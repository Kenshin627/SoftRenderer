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


def helmet_edge_comparison():
    # Show the same 96x64 pixels of the helmet crown, without smoothing the enlargement.
    box = (908, 64, 1004, 128)
    comparison = Image.new("RGB", (96 * 6 * 2, 64 * 6))
    for i, name in enumerate(("helmet", "helmet-msaa4")):
        with Image.open(GALLERY / (name + ".png")) as frame:
            crop = frame.crop(box).resize((96 * 6, 64 * 6), Image.Resampling.NEAREST)
            comparison.paste(crop, (i * crop.width, 0))
    comparison.save(GALLERY / "helmet-msaa-detail.png")


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
        helmet_edge_comparison()
        for mode in ("albedo", "emission", "lit", "shadow", "depth", "toon"):
            render(directory, mode, "diablo", mode)
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
