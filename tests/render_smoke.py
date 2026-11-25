"""Image regressions for the public CLI. Build Release x64 and install Pillow first."""
import math
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

    def filter_triangle(self, uv_at):
        # Prescribe screen offsets and reciprocal clip w. The rasterizer must clip
        # this triangle and reconstruct the same rational UV field in every piece.
        eye = (0.55, 0.15, 2.9)
        back = tuple(v / math.sqrt(sum(c*c for c in eye)) for v in eye)
        norm = math.hypot(back[0], back[2])
        right = (back[2]/norm, 0, -back[0]/norm)
        up = (back[1]*right[2], back[2]*right[0]-back[0]*right[2], -back[1]*right[0])
        rows = []
        for x, y in ((-180, -115), (170, -115), (0, 115)):
            q = 0.8 + 0.0019*x + 0.00227*y
            scale = 2 * math.tan(math.radians(22.5)) / 127
            world = [eye[i] + (scale*(right[i]*x + up[i]*y) - back[i])/q for i in range(3)]
            u, v = uv_at(x, y, q)
            rows.extend(["v " + " ".join(map(str, world)), f"vt {u} {1-v}"])
        (self.folder / "plane.obj").write_text("\n".join(rows + ["f 1/1 2/2 3/3"]))

    def sample_filter(self, name, raster="bbox"):
        return self.frame("--model", "plane.obj", "--mode", "albedo", "--filter", name,
                          "--raster", raster, cwd=self.folder)

    def test_bilinear_centers_and_repeat_seams(self):
        image = Image.new("RGB", (2, 2))
        image.putdata([(240, 0, 0), (0, 240, 0), (0, 0, 240), (240, 240, 240)])
        image.save(self.folder / "plane_diffuse.tga")
        for uv, expected in (((0.25, 0.25), (240, 0, 0)),
                             ((0.375, 0.625), (90, 60, 180)),
                             ((-1.0, 2.0), (120, 120, 120))):
            self.filter_triangle(lambda x, y, q: uv)
            for mode in ("bilinear", "mipmap", "trilinear"):
                actual = self.sample_filter(mode).getpixel((80, 63))
                self.assertTrue(all(abs(a-b) <= 1 for a,b in zip(actual, expected)), (uv, mode, actual))

    def test_mips_include_odd_edges_and_stop_at_one(self):
        for width, height in ((5, 3), (1, 7), (7, 1), (1, 1)):
            image = Image.new("RGB", (width, height))
            image.putdata([(240*(x == width-1), 240*(y == height-1), 50)
                           for y in range(height) for x in range(width)])
            image.save(self.folder / "plane_diffuse.tga")
            self.filter_triangle(lambda x, y, q: (10*x/q, 10*y/q))
            expected = (240/width, 240/height, 50)
            for mode in ("mipmap", "trilinear"):
                actual = self.sample_filter(mode).getpixel((80, 63))
                self.assertTrue(all(abs(a-b) <= 1 for a,b in zip(actual, expected)), (image.size, actual))

    def test_filtered_opposing_normals_fall_back_to_surface(self):
        normals = Image.new("RGB", (2, 1))
        normals.putdata([(255, 128, 128), (0, 127, 127)])
        normals.save(self.folder / "plane_nm.tga")
        self.filter_triangle(lambda x, y, q: (0.5, 0.5))
        args = ("--model", "plane.obj", "--mode", "normal", "--filter", "bilinear")
        mapped = self.frame(*args, "--normals", "object", cwd=self.folder)
        geometric = self.frame(*args, "--normals", "vertex", cwd=self.folder)
        self.assertEqual(mapped.getpixel((80, 63)), geometric.getpixel((80, 63)))

    def test_fractional_lod_after_perspective_clipping(self):
        # At the center, the larger footprint lies along screen Y. LOD = 1.25.
        # The 8x1 texture gives distinct bilinear samples at levels 0, 1 and 2.
        image = Image.new("L", (8, 1))
        image.putdata([0, 0, 0, 240, 0, 0, 0, 0])
        image.save(self.folder / "plane_diffuse.tga")
        rate = 0.8 * 2**1.25 / 8
        self.filter_triangle(lambda x, y, q: (0.4375 + rate*y/q, 0.5))
        for mode, expected in (("bilinear", 240), ("mipmap", 90), ("trilinear", 76.875)):
            frame = self.sample_filter(mode)
            self.assertAlmostEqual(frame.getpixel((80, 63))[0], expected, delta=1.1)
            self.assertEqual(frame.tobytes(), self.sample_filter(mode, "scanline").tobytes())

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
