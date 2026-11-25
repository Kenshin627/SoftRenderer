"""Re-render the README gallery with the Release x64 executable and Pillow."""
from pathlib import Path
import subprocess
import tempfile

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / "SoftRenderer"
EXE = PROJECT / "bin/x64/Release/SoftRenderer.exe"
GALLERY = ROOT / "docs/screenshots"


def render(directory, name, scene, mode, *options):
    path = directory / (name + ".tga")
    subprocess.run(
        [str(EXE), "--scene", scene, "--mode", mode, "--size", "2160x1560",
         "--output", str(path), *options],
        cwd=PROJECT, check=True,
    )
    with Image.open(path) as frame:
        frame.save(GALLERY / (name + ".png"))


def detail_grid(name, rows, box, gap=0):
    # Enlarge exactly the same source pixels in every panel.
    width, height = (box[2] - box[0]) * 6, (box[3] - box[1]) * 6
    cols = len(rows[0])
    comparison = Image.new("RGB", (cols * width + (cols - 1) * gap,
                                   len(rows) * height + (len(rows) - 1) * gap), (48, 48, 48))
    for y, row in enumerate(rows):
        for x, source in enumerate(row):
            with Image.open(GALLERY / (source + ".png")) as frame:
                crop = frame.crop(box).resize((width, height), Image.Resampling.NEAREST)
                comparison.paste(crop, (x * (width + gap), y * (height + gap)))
    comparison.save(GALLERY / (name + ".png"))


def shadow_comparison(directory):
    rows = []
    for side in ("2048", "4096"):
        row = []
        for filtering in ("hard", "pcf"):
            name = f"shadow-{side}-{filtering}"
            render(directory, name, "diablo", "shadow", "--shadow-size", side, "--shadow-filter", filtering)
            row.append(name)
        rows.append(row)
    detail_grid("shadow-filter-detail", rows, (518, 941, 614, 1005), gap=6)
    contact = [("shadow", "shadow-2048-pcf", "shadow-4096-pcf")]
    detail_grid("shadow-contact-detail", contact, (864, 1064, 960, 1128), gap=6)


def main():
    GALLERY.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        directory = Path(tmp)
        for mode in ("wire", "barycentric", "flat", "gouraud", "smooth"):
            render(directory, mode, "head", mode)
        for space in ("vertex", "object", "tangent"):
            render(directory, "normal-" + space, "head", "normal", "--normals", space)
        render(directory, "phong", "head", "phong", "--normals", "object")
        render(directory, "helmet", "helmet", "pbr")
        render(directory, "helmet-msaa4", "helmet", "pbr", "--msaa", "4")
        detail_grid("helmet-msaa-detail", [("helmet", "helmet-msaa4")], (908, 64, 1004, 128))
        for mode in ("albedo", "emission", "lit", "shadow", "depth", "toon"):
            render(directory, mode, "diablo", mode)
        shadow_comparison(directory)
        for filtering in ("bilinear", "mipmap", "trilinear"):
            render(directory, "albedo-" + filtering, "diablo", "albedo", "--filter", filtering)
        render(directory, "bands", "diablo", "toon", "--post", "none")
        render(directory, "mosaic", "diablo", "shadow", "--post", "mosaic")
        for ao in ("ssao", "brute"):
            visibility = directory / (ao + "-map.tga")
            render(directory, ao, "diablo", "shadow", "--ao", ao, "--ao-map", str(visibility))
            with Image.open(visibility) as frame:
                frame.save(GALLERY / (ao + "-map.png"))


if __name__ == "__main__":
    main()
