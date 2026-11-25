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
    source = ["--scene", scene] if scene else []
    subprocess.run(
        [str(EXE), *source, "--mode", mode, "--size", "2160x1560",
         "--output", str(path), *options],
        cwd=PROJECT, check=True,
    )
    with Image.open(path) as frame:
        frame.save(GALLERY / (name + ".png"))


def detail_grid(name, rows, box, gap=0, scale=6):
    # Enlarge exactly the same source pixels in every panel.
    width, height = (box[2] - box[0]) * scale, (box[3] - box[1]) * scale
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


def hdr_comparison(directory):
    options = ("--hdr", "--shadow-size", "2048", "--shadow-filter", "pcf")
    for name, strength, ev, bloom in (("hdr-emission-1", 1, 0, 0), ("hdr-emission-8", 8, 0, 0),
                                      ("hdr-emission-64", 64, 0, 0), ("hdr-bloom", 64, 0, 1),
                                      ("hdr-exposure-minus2", 64, -2, 0)):
        render(directory, name, "diablo", "shadow", *options, "--emission-strength", str(strength),
               "--exposure", str(ev), "--bloom", str(bloom))
    render(directory, "helmet-hdr", "helmet", "pbr", "--alpha", "opaque", "--hdr")
    sequence = [("hdr-emission-1", "hdr-emission-8", "hdr-emission-64", "hdr-bloom")]
    detail_grid("hdr-bloom-detail", sequence, (978, 285, 1166, 473), gap=6, scale=2)


def environment_comparison(directory):
    names = []
    for angle in (0, 90, 180):
        name = f"helmet-ibl-{angle}"
        render(directory, name, "helmet", "pbr", "--alpha", "opaque", "--env", "studio", "--env-rotation", str(angle))
        names.append(name)
    detail_grid("helmet-ibl-leather-detail", [names], (880, 68, 1290, 278), gap=6, scale=2)
    detail_grid("helmet-ibl-metal-detail", [names], (760, 1150, 1070, 1370), gap=6, scale=2)


def alpha_comparison(directory):
    render(directory, "helmet-alpha-blend", "helmet", "pbr", "--env", "studio")
    detail_grid("helmet-alpha-detail", [("helmet-ibl-0", "helmet-alpha-blend")],
                (789, 255, 1179, 495), gap=6, scale=2)
    screen = ("--model", "resource/models/alpha/cutout.obj", "--shadow-size", "2048",
              "--shadow-filter", "pcf", "--filter", "bilinear", "--msaa", "4")
    for mode in ("opaque", "mask"):
        render(directory, "cutout-" + mode, None, "shadow", *screen, "--alpha", mode)
    render(directory, "panes-blend", None, "albedo", "--model", "resource/models/alpha/panes.obj",
           "--filter", "bilinear", "--msaa", "4")


def anisotropy_comparison(directory):
    names = []
    for limit in (1, 4, 8):
        name = f"checker-aniso-{limit}"
        render(directory, name, None, "albedo", "--model", "resource/models/filtering/checker.obj",
               "--filter", "trilinear", "--anisotropy", str(limit))
        names.append(name)
    detail_grid("checker-aniso-detail", [names], (1310, 316, 1470, 396), gap=6, scale=4)


def main():
    GALLERY.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        directory = Path(tmp)
        for mode in ("wire", "barycentric", "flat", "gouraud", "smooth"):
            render(directory, mode, "head", mode)
        for space in ("vertex", "object", "tangent"):
            render(directory, "normal-" + space, "head", "normal", "--normals", space)
        render(directory, "phong", "head", "phong", "--normals", "object")
        render(directory, "helmet", "helmet", "pbr", "--alpha", "opaque")
        render(directory, "helmet-msaa4", "helmet", "pbr", "--alpha", "opaque", "--msaa", "4")
        detail_grid("helmet-msaa-detail", [("helmet", "helmet-msaa4")], (908, 64, 1004, 128))
        for mode in ("albedo", "emission", "lit", "shadow", "depth", "toon"):
            render(directory, mode, "diablo", mode)
        shadow_comparison(directory)
        hdr_comparison(directory)
        environment_comparison(directory)
        alpha_comparison(directory)
        anisotropy_comparison(directory)
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
