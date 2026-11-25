"""Image regressions for the public CLI. Build Release x64 and install Pillow first."""
import math
from pathlib import Path
import subprocess
import tempfile
import unittest

from PIL import Image, ImageChops, ImageStat

PROJECT = Path(__file__).resolve().parents[1] / "SoftRenderer"
EXE = PROJECT / "bin/x64/Release/SoftRenderer.exe"


class RenderTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.folder = Path(self.temp.name)

    def frame(self, *args, cwd=None, size="161x127"):
        target = self.folder / "frame.tga"
        subprocess.run([str(EXE), *args, "--size", size, "--output", str(target)],
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

    def screen_mesh(self, name, points, faces, color, distance=2.0):
        # Coordinates are in the 161x127 bottom-up viewport of the --model camera.
        eye = (0.55, 0.15, 2.9)
        length = math.sqrt(sum(v*v for v in eye))
        back = tuple(v / length for v in eye)
        h = math.hypot(back[0], back[2])
        right = (back[2]/h, 0, -back[0]/h)
        up = (back[1]*right[2], back[2]*right[0]-back[0]*right[2], -back[1]*right[0])
        scale = 2 * distance * math.tan(math.radians(22.5)) / 127
        rows = []
        for x, y in points:
            world = [eye[i] - distance * back[i] + scale * ((x-80.5)*right[i] + (y-63.5)*up[i])
                     for i in range(3)]
            rows.append("v " + " ".join(map(str, world)))
        rows.extend("f " + " ".join(map(str, f)) for f in faces)
        (self.folder / (name + ".obj")).write_text("\n".join(rows))
        Image.new("RGB", (1, 1), color).save(self.folder / (name + "_diffuse.tga"))
        return name + ".obj"

    def test_shared_edge_has_exactly_one_owner(self):
        for samples, bottom in (("1", 30), ("4", 30.75)):
            a, b, c, d = (40, bottom), (100, bottom), (100, bottom+60), (40, bottom+60)
            lower = self.screen_mesh("lower", [a,b,c], [(1,2,3)], (240,0,0))
            upper = self.screen_mesh("upper", [a,c,d], [(1,2,3)], (0,240,0))
            args = ("--mode", "albedo", "--msaa", samples)
            first = self.frame(*args, "--model", lower, "--model", upper, cwd=self.folder)
            second = self.frame(*args, "--model", upper, "--model", lower, cwd=self.folder)
            self.assertEqual(first.tobytes(), second.tobytes())
            for x in range(41, 99):
                for y in range(32, 89):
                    r, g, b = first.getpixel((x, 126-y))
                    self.assertEqual((r+g, b), (240, 0))
            # In a Y-up raster the lower triangle owns its left (diagonal) edge.
            self.assertEqual(first.getpixel((80, 126-70)), (240, 0, 0))

    def test_subpixel_triangle_hits_only_covered_sample(self):
        mesh = self.screen_mesh("thin", [(80.30,63.07),(80.46,63.07),(80.375,63.23)],
                                [(1,2,3)], (241,0,0))
        args = ("--mode", "albedo", "--model", mesh)
        single = self.frame(*args, cwd=self.folder)
        self.assertEqual(single.getpixel((80,63)), (209,195,177))
        for traversal in ("bbox", "scanline"):
            multi = self.frame(*args, "--msaa", "4", "--raster", traversal, cwd=self.folder)
            self.assertEqual(multi.getpixel((80,63)), (217,146,133))
        tiny = self.screen_mesh("center", [(80.44,63.44),(80.56,63.44),(80.5,63.58)],
                                [(1,2,3)], (241,0,0))
        self.assertEqual(self.frame("--mode", "albedo", "--model", tiny, cwd=self.folder)
                         .getpixel((80,63)), (241,0,0))

    def test_msaa_depth_and_resolve_are_per_sample(self):
        faces = [(1,2,3), (1,3,4)]
        near = self.screen_mesh("near", [(50,40),(80.5,40),(80.5,90),(50,90)], faces, (240,0,0))
        far = self.screen_mesh("far", [(45,35),(110,35),(110,95),(45,95)], faces, (0,0,240), 3.0)
        for first, second in ((near,far), (far,near)):
            args = ("--msaa", "4", "--model", first, "--model", second)
            im = self.frame(*args, "--mode", "albedo", cwd=self.folder)
            self.assertEqual(im.getpixel((80,63)), (120,0,120))
            depth = self.frame(*args, "--mode", "depth", cwd=self.folder)
            self.assertAlmostEqual(depth.getpixel((80,63))[0], 128, delta=1)

    def test_msaa_shading_point_independent_of_occluder_order(self):
        self.filter_triangle(lambda x, y, q: (0.5 + 0.12*x/q, 0.5))
        ramp = Image.new("L", (2, 1))
        ramp.putdata([0, 240])
        ramp.save(self.folder / "plane_diffuse.tga")
        near = self.screen_mesh("occluder", [(50,40),(80.5,40),(80.5,90),(50,90)],
                                [(1,2,3), (1,3,4)], (20,20,20), distance=0.6)
        args = ("--msaa", "4", "--mode", "albedo", "--filter", "bilinear")
        for first, second in ((near, "plane.obj"), ("plane.obj", near)):
            frame = self.frame(*args, "--model", first, "--model", second, cwd=self.folder)
            # Two samples see the foreground; the other two share the full
            # background primitive's centroid shading value (120).
            self.assertAlmostEqual(frame.getpixel((80, 63))[0], 70, delta=1)

    def test_msaa_silhouette_against_supersampled_reference(self):
        mesh = self.screen_mesh("slant", [(13.2,16.7),(139.6,35.1),(77.1,118.4)], [(1,2,3)], (5,10,20))
        args = ("--mode", "albedo", "--model", mesh)
        single = self.frame(*args, cwd=self.folder)
        multi = self.frame(*args, "--msaa", "4", cwd=self.folder)
        ref = self.frame(*args, cwd=self.folder, size="1288x1016").resize(single.size, Image.Resampling.BOX)
        def error(image):
            return sum(ImageStat.Stat(ImageChops.difference(image, ref)).sum)
        self.assertLess(error(multi), 0.5 * error(single))
        scanline = self.frame(*args, "--msaa", "4", "--raster", "scanline", cwd=self.folder)
        self.assertEqual(multi.tobytes(), scanline.tobytes())

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

    def test_shadow_filter_preserves_planes_and_filters_cast_edges(self):
        # A gray receiver and a red square half a unit closer to the directional light.
        # The receiver extends beyond the shadow frustum; its slope in light space is nonzero.
        for name, lo, hi, z, color in (("receiver", -3, 3, 0, (200, 200, 200)),
                                       ("caster", -.2, .6, .5, (150, 0, 0))):
            rows = [f"v {x} {y} {z}" for x, y in ((lo, lo), (hi, lo), (hi, hi), (lo, hi))]
            (self.folder / (name + ".obj")).write_text("\n".join(rows + ["f 1 2 3", "f 1 3 4"]))
            Image.new("RGB", (1, 1), color).save(self.folder / (name + "_diffuse.tga"))
            Image.new("L", (1, 1), 0).save(self.folder / (name + "_spec.tga"))
        def render(mode, *options):
            return self.frame("--mode", mode, "--model", "receiver.obj", *options,
                              size="321x241", cwd=self.folder)
        lit = render("lit")
        for side in ("1", "31", "256"):
            filtered = render("shadow", "--shadow-filter", "pcf", "--shadow-size", side)
            self.assertEqual(lit.tobytes(), filtered.tobytes(), side)

        def receiver_levels(side, filtering):
            frame = render("shadow", "--model", "caster.obj", "--shadow-size", side,
                           "--shadow-filter", filtering)
            return {r: count for count, (r, g, b) in frame.getcolors(321 * 241) if r == g == b}
        # Only direct diffuse light varies. Each blocked tap removes 1/9 of its shadowable part.
        levels = {int(200 * (0.4 + (1 - 0.7 * blocked / 9) / math.sqrt(3))) for blocked in range(10)}
        ends = {min(levels), max(levels)}
        self.assertEqual(set(receiver_levels("256", "hard")), ends)
        coarse = receiver_levels("64", "pcf")
        fine = receiver_levels("256", "pcf")
        self.assertTrue(ends <= coarse.keys() <= levels)
        self.assertTrue(ends <= fine.keys() <= levels)
        self.assertGreaterEqual(len(coarse), 8)
        self.assertGreater(sum(n for v, n in coarse.items() if v not in ends),
                           sum(n for v, n in fine.items() if v not in ends))

    def test_shadow_map_size_and_defaults(self):
        args = ("--mode", "shadow")
        default = self.frame(*args, size="151x151")
        for side in ("auto", "604"):
            explicit = self.frame(*args, "--shadow-size", side, "--shadow-filter", "hard", size="151x151")
            self.assertEqual(default.tobytes(), explicit.tobytes())
        coarse = self.frame(*args, "--shadow-size", "63", size="151x151")
        self.assertIsNotNone(ImageChops.difference(default, coarse).getbbox())
        for key, value in (("--shadow-size", "0"), ("--shadow-size", "8193"), ("--shadow-size", "32x32"),
                           ("--shadow-size", "-1"), ("--shadow-filter", "linear")):
            result = subprocess.run([str(EXE), key, value, "--output", str(self.folder / "bad.tga")],
                                    cwd=PROJECT, capture_output=True)
            self.assertNotEqual(result.returncode, 0, (key, value))

    def test_pcf_and_camera_msaa_share_the_same_ao_map_as_hard_shadows(self):
        visibility = self.folder / "ao.tga"
        args = ("--mode", "shadow", "--msaa", "4", "--ao", "ssao", "--ao-map", str(visibility))
        hard = self.frame(*args, "--shadow-size", "512")
        with Image.open(visibility) as image:
            ao = image.tobytes()
        pcf = self.frame(*args, "--shadow-size", "127", "--shadow-filter", "pcf")
        with Image.open(visibility) as image:
            self.assertEqual(ao, image.tobytes())
        self.assertIsNotNone(ImageChops.difference(hard, pcf).getbbox())

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
