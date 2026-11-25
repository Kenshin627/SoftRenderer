"""Image regressions for the public CLI. Build Release x64 and install Pillow first."""
from pathlib import Path
import subprocess
import tempfile
import unittest

from PIL import Image, ImageChops

PROJECT = Path(__file__).resolve().parents[1] / "SoftRenderer"
EXE = PROJECT / "bin/x64/Release/SoftRenderer.exe"


class RenderTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.folder = Path(self.temp.name)

    def frame(self, *args, cwd=None):
        target = self.folder / "frame.tga"
        subprocess.run([str(EXE), *args, "--size", "161x127", "--output", str(target)],
                       cwd=cwd or PROJECT, check=True)
        with Image.open(target) as image:
            return image.convert("RGB")

    def test_scanline_and_bbox_match(self):
        for scene in ("head", "diablo"):
            with self.subTest(scene=scene):
                args = ("--scene", scene, "--mode", "barycentric")
                bbox = self.frame(*args)
                self.assertGreater(len(bbox.getcolors(161 * 127)), 100)
                self.assertEqual(bbox.tobytes(), self.frame(*args, "--raster", "scanline").tobytes())

    def test_normals_and_emission(self):
        # No vn: the loader must derive +Z from this face. UV axes match +X/+Y.
        (self.folder / "tri.obj").write_text(
            "v -1 -1 0\nv 1 -1 0\nv 0 1 0\nvt 0 0\nvt 1 0\nvt .5 1\nf 1/1 2/2 3/3\n")
        color = (128, 204, 230)
        for suffix in ("_nm", "_nm_tangent"):
            Image.new("RGB", (1, 1), color).save(self.folder / ("tri" + suffix + ".tga"))
        for suffix in ("_diff", "_spec"):
            Image.new("L", (1, 1), 0).save(self.folder / ("tri" + suffix + ".tga"))
        glow = (27, 91, 163)
        Image.new("RGB", (1, 1), glow).save(self.folder / "tri_glow.tga")

        def pixel(*args):
            return self.frame("--model", "tri.obj", *args, cwd=self.folder).getpixel((80, 63))

        for source in ("object", "tangent", "auto"):
            observed = pixel("--mode", "normal", "--normals", source)
            self.assertTrue(all(abs(a-b) <= 2 for a, b in zip(observed, color)), (source, observed))
        self.assertEqual(pixel("--mode", "normal", "--normals", "vertex"), (127, 127, 255))
        self.assertEqual(pixel("--mode", "shadow", "--ao", "ssao"), glow)

    def test_near_plane_clipping(self):
        # Build a triangle in the custom-model camera frame. One corner is
        # behind the eye; clipping at distance 0.1 yields the reference quad.
        import math
        eye = (0.55, 0.15, 2.9)
        def unit(v):
            return tuple(x / math.sqrt(sum(y*y for y in v)) for x in v)
        def cross(a, b):
            return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
        back = unit(eye)
        right = unit(cross((0, 1, 0), back))
        up = cross(back, right)
        def write_mesh(name, points, faces):
            rows = []
            for x, y, depth in points:
                world = [eye[i] + right[i]*x + up[i]*y - back[i]*depth for i in range(3)]
                rows.append("v " + " ".join(str(c) for c in world))
            (self.folder / name).write_text("\n".join(rows + faces))
        a, b, c = (-0.17, -0.14, 0.65), (0.17, -0.14, 0.65), (0, 0.08, -0.05)
        def at_near(p):
            t = (0.1 - c[2]) / (p[2] - c[2])
            return tuple(u + t*(v-u) for u, v in zip(c, p))
        write_mesh("crossing.obj", [a, b, c], ["f 1 2 3"])
        write_mesh("clipped.obj", [a, b, at_near(b), at_near(a)], ["f 1 2 3", "f 1 3 4"])
        actual = self.frame("--model", "crossing.obj", "--mode", "albedo", cwd=self.folder)
        reference = self.frame("--model", "clipped.obj", "--mode", "albedo", cwd=self.folder)
        different = sum(a != b for a, b in zip(actual.tobytes(), reference.tobytes()))
        self.assertLessEqual(different, 3)  # at most one boundary pixel of rounding
        self.assertEqual(actual.getpixel((80, 63)), (255, 255, 255))

    def test_depth_is_independent_of_model_order(self):
        models = []
        for name, z, red in (("near", 0.3, 210), ("far", -0.3, 40)):
            (self.folder / (name + ".obj")).write_text(
                f"v -1 -1 {z}\nv 1 -1 {z}\nv 0 1 {z}\nf 1 2 3\n")
            Image.new("L", (1, 1), red).save(self.folder / (name + "_diffuse.tga"))
            models.append(name + ".obj")
        def render(first, second):
            return self.frame("--model", first, "--model", second, "--mode", "albedo", cwd=self.folder)
        forward, reverse = render(*models), render(*reversed(models))
        self.assertEqual(forward.tobytes(), reverse.tobytes())
        self.assertEqual(forward.getpixel((80, 63)), (210, 210, 210))

    def test_occlusion_on_first_frame(self):
        lit = self.frame("--mode", "lit")
        shadow = self.frame("--mode", "shadow")
        self.assertIsNotNone(ImageChops.difference(lit, shadow).getbbox())
        for ao in ("ssao", "brute"):
            path = self.folder / (ao + ".tga")
            occluded = self.frame("--mode", "shadow", "--ao", ao, "--ao-map", str(path))
            self.assertIsNotNone(ImageChops.difference(occluded, shadow).getbbox())
            with Image.open(path) as visibility:
                self.assertEqual(visibility.size, shadow.size)
                low, high = visibility.getextrema()
                self.assertLess(low, high)
                self.assertEqual(high, 255)

    def test_toon_outline_and_flat_normals(self):
        bands = self.frame("--mode", "toon", "--post", "none")
        ink = self.frame("--mode", "toon")
        self.assertGreater(sum(n for n, c in ink.getcolors(20) if c == (0, 0, 0)), 0)
        self.assertIsNotNone(ImageChops.difference(ink, bands).getbbox())
        flat = self.frame("--scene", "head", "--mode", "flat")
        self.assertGreater(len(flat.getcolors(161 * 127)), 50)


if __name__ == "__main__":
    unittest.main()
