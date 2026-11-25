"""Image regressions for the public CLI. Build Release x64 and install Pillow first."""
import math
from pathlib import Path
import subprocess
import tempfile
import unittest

from PIL import Image, ImageChops, ImageStat

PROJECT = Path(__file__).resolve().parents[1] / "SoftRenderer"
EXE = PROJECT / "bin/x64/Release/SoftRenderer.exe"


def srgb_decode(byte):
    s = byte / 255
    return s / 12.92 if s <= 0.04045 else ((s + 0.055) / 1.055)**2.4


def display_byte(radiance, ev=0):
    exposed = radiance * 2**ev
    mapped = exposed / (1 + exposed)
    srgb = 12.92 * mapped if mapped <= 0.0031308 else 1.055 * mapped**(1/2.4) - 0.055
    return round(255 * srgb)


def unit(v):
    length = math.sqrt(sum(x*x for x in v))
    return tuple(x/length for x in v)


def write_environment(path, radiance, width=32, height=16):
    # Small uncompressed Radiance RGBE panoramas; keep the test oracle independent of the decoder.
    pixels = bytearray()
    for y in range(height):
        theta = math.pi * (y + 0.5) / height
        for x in range(width):
            phi = 2 * math.pi * ((x + 0.5) / width - 0.5)
            direction = (math.sin(theta)*math.cos(phi), math.cos(theta), math.sin(theta)*math.sin(phi))
            rgb = radiance(x, y, direction)
            if max(rgb) == 0:
                pixels.extend((0, 0, 0, 0))
            else:
                _, exponent = math.frexp(max(rgb))
                pixels.extend(int(c * 2**(8-exponent)) for c in rgb)
                pixels.append(exponent + 128)
    header = f"#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y {height} +X {width}\n"
    path.write_bytes(header.encode("ascii") + pixels)


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

    def sample_filter(self, name, raster="bbox", *options):
        return self.frame("--model", "plane.obj", "--mode", "albedo", "--filter", name,
                          "--raster", raster, *options, cwd=self.folder)

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

    def test_anisotropy_limits_and_rotated_footprints(self):
        texture = Image.new("L", (64, 32))
        texture.putdata([255 if y == 16 else 0 for y in range(32) for x in range(64)])
        texture.save(self.folder / "plane_diffuse.tga")
        # Rotate the same 8x1 texel footprint in screen space. At 45 degrees
        # neither screen derivative is parallel to the long axis.
        for angle in (0, math.pi/4, math.pi/2):
            c, s = math.cos(angle), math.sin(angle)
            self.filter_triangle(lambda x, y, q: (
                .5 + .8*8*(c*x+s*y)/(64*q), 16.5/32 + .8*(-s*x+c*y)/(32*q)))
            for limit, expected in ((2, 40), (4, 96), (8, 255)):
                frame = self.sample_filter("trilinear", "bbox", "--anisotropy", str(limit))
                self.assertAlmostEqual(frame.getpixel((80, 63))[0], expected, delta=2, msg=(angle, limit))
            self.assertEqual(frame.tobytes(),
                             self.sample_filter("trilinear", "scanline", "--anisotropy", "8").tobytes())
        ordinary = self.sample_filter("trilinear")
        self.assertEqual(ordinary.tobytes(), self.sample_filter("trilinear", "bbox", "--anisotropy", "1").tobytes())

    def test_anisotropy_preserves_four_texel_stripes_at_grazing_angles(self):
        texture = Image.new("L", (64, 64))
        texture.putdata([255 if (y // 4) % 2 else 0 for y in range(64) for x in range(64)])
        texture.save(self.folder / "plane_diffuse.tga")
        for row, expected in ((33, 0), (37, 255)):
            self.filter_triangle(lambda x, y, q: (.5 + .8*8*x/(64*q), (row+.5)/64 + .8*y/(64*q)))
            ordinary = self.sample_filter("trilinear")
            eight = self.sample_filter("trilinear", "bbox", "--anisotropy", "8")
            self.assertAlmostEqual(ordinary.getpixel((80, 63))[0], 128, delta=1)
            self.assertEqual(eight.getpixel((80, 63))[0], expected)

    def test_anisotropy_follows_a_diagonal_in_texture_space(self):
        texture = Image.new("L", (64, 64))
        texture.putdata([255 if (y-x) % 16 < 8 else 0 for y in range(64) for x in range(64)])
        texture.save(self.folder / "plane_diffuse.tga")
        for offset, expected in ((3.5, 255), (11.5, 0)):
            self.filter_triangle(lambda x, y, q: (
                .5 + .8*(16*x-y)/(math.sqrt(2)*64*q),
                .5 + offset/64 + .8*(16*x+y)/(math.sqrt(2)*64*q)))
            aa = self.sample_filter("trilinear", "bbox", "--anisotropy", "8")
            self.assertAlmostEqual(aa.getpixel((80, 63))[0], expected, delta=6)
            self.assertAlmostEqual(self.sample_filter("trilinear").getpixel((80, 63))[0], 128, delta=2)

    def test_anisotropic_taps_wrap_and_filter_opacity_as_data(self):
        # Eight centers straddle the repeat seam. Six land on white texels.
        texture = Image.new("RGBA", (64, 1))
        texture.putdata([(255, 255, 255, 255) if x >= 61 or 2 <= x <= 4 else (0, 0, 0, 0)
                         for x in range(64)])
        texture.save(self.folder / "plane_diffuse.tga")
        for tile in (-1, 0, 1):
            self.filter_triangle(lambda x, y, q: (tile + 1/64 + .8*8*x/(64*q), .5))
            self.material("plane", "map_d plane_diffuse.tga", "alpha_mode mask")
            args = ("--model", "plane.obj", "--mode", "albedo", "--filter", "trilinear", "--anisotropy", "8")
            opaque = self.frame(*args, "--alpha", "opaque", cwd=self.folder)
            hdr = self.frame(*args, "--hdr", "--alpha-cutoff", ".73", cwd=self.folder)
            hole = self.frame(*args, "--alpha-cutoff", ".77", cwd=self.folder)
            self.assertAlmostEqual(opaque.getpixel((80, 63))[0], 191, delta=1)
            self.assertAlmostEqual(hdr.getpixel((80, 63))[0], display_byte(.75), delta=1)
            self.assertEqual(hole.getpixel((80, 63)), (209, 195, 177))

    def test_anisotropy_rejects_invalid_cli_options(self):
        for mode, level in (("trilinear", "0"), ("trilinear", "3"), ("trilinear", "16"),
                            ("trilinear", "4x"), ("bilinear", "4"), ("nearest", "2"), ("mipmap", "8")):
            result = subprocess.run([str(EXE), "--filter", mode, "--anisotropy", level, "--output",
                                     str(self.folder / "bad.tga")], cwd=PROJECT, capture_output=True)
            self.assertNotEqual(result.returncode, 0, (mode, level))

    def test_hdr_recovers_emission_above_display_white(self):
        self.screen_mesh("lamp", [(40, 30), (120, 30), (80, 100)], [(1, 2, 3)], (0, 0, 0))
        Image.new("L", (1, 1), 0).save(self.folder / "lamp_spec.tga")
        glow = (255, 128, 10)
        Image.new("RGB", (1, 1), glow).save(self.folder / "lamp_glow.tga")
        for mode in ("emission", "lit", "phong", "shadow", "pbr"):
            for scale, ev in ((1, 0), (8, 0), (8, -3)):
                with self.subTest(mode=mode, scale=scale, ev=ev):
                    frame = self.frame("--model", "lamp.obj", "--mode", mode, "--emission-strength",
                                       str(scale), "--exposure", str(ev), cwd=self.folder)
                    expected = tuple(display_byte(scale * srgb_decode(c), ev) for c in glow)
                    pixel = frame.getpixel((80, 63))
                    self.assertTrue(all(abs(a-b) <= 1 for a, b in zip(expected, pixel)), (expected, pixel))

    def test_hdr_color_filtering_happens_in_linear_light(self):
        texture = Image.new("L", (2, 1))
        texture.putdata([0, 255])
        texture.save(self.folder / "plane_diffuse.tga")
        for mode, rate in (("bilinear", 0), ("mipmap", 10), ("trilinear", 10)):
            self.filter_triangle(lambda x, y, q: (0.5 + rate * x/q, 0.5))
            frame = self.frame("--model", "plane.obj", "--mode", "albedo", "--hdr",
                               "--filter", mode, cwd=self.folder)
            self.assertAlmostEqual(frame.getpixel((80, 63))[0], display_byte(0.5), delta=1)

    def test_hdr_keeps_specular_texture_as_data(self):
        # At the center the normal bisects the view and light directions, giving a unit highlight.
        eye = (0.55, 0.15, 2.9)
        h = [v / math.sqrt(sum(c*c for c in eye)) + 1/math.sqrt(3) for v in eye]
        n = " ".join(str(v / math.sqrt(sum(c*c for c in h))) for v in h)
        (self.folder / "gloss.obj").write_text(
            "v -1 -1 0\nv 1 -1 0\nv 0 1 0\nvn " + n + "\nf 1//1 2//1 3//1\n")
        Image.new("L", (1, 1), 0).save(self.folder / "gloss_diffuse.tga")
        Image.new("L", (1, 1), 128).save(self.folder / "gloss_spec.tga")
        pixel = self.frame("--model", "gloss.obj", "--mode", "lit", "--hdr",
                           cwd=self.folder).getpixel((80, 63))
        self.assertAlmostEqual(pixel[0], display_byte(128/255), delta=1)

    def material(self, mesh, *lines):
        path = self.folder / (mesh + ".obj")
        geometry = path.read_text()
        if not geometry.startswith("mtllib "):
            path.write_text(f"mtllib {mesh}.mtl\nusemtl surface\n" + geometry)
        (self.folder / (mesh + ".mtl")).write_text("\n".join(
            ["newmtl surface", f"map_Kd {mesh}_diffuse.tga", *lines]) + "\n")

    def test_alpha_blend_order_opaque_depth_and_msaa(self):
        quad = [(1, 2, 3), (1, 3, 4)]
        for name, color, distance, right in (("black", (0, 0, 0), 4, 125),
                                             ("red", (255, 0, 0), 3, 120),
                                             ("blue", (0, 0, 255), 2, 80.5),
                                             ("green", (0, 255, 0), 1.5, 70)):
            self.screen_mesh(name, [(35, 25), (right, 25), (right, 96), (35, 96)], quad, color, distance)
        self.material("red", "d 0.5")
        self.material("blue", "Tr 0.75")
        models = ["black.obj", "blue.obj", "green.obj", "red.obj"]
        for samples in ("1", "4"):
            def render(order, raster):
                args = [token for mesh in order for token in ("--model", mesh)]
                return self.frame("--mode", "albedo", "--msaa", samples, "--raster", raster, *args,
                                  cwd=self.folder)
            image = render(models, "bbox")
            self.assertEqual(image.tobytes(), render(reversed(models), "scanline").tobytes())
            self.assertEqual(image.getpixel((55, 63)), (0, display_byte(1), 0))
            self.assertEqual(image.getpixel((75, 63)), (display_byte(.375), 0, display_byte(.25)))
            self.assertEqual(image.getpixel((95, 63)), (display_byte(.5), 0, 0))
            if samples == "4":
                self.assertEqual(image.getpixel((80, 63)), (display_byte(.4375), 0, display_byte(.125)))

    def test_alpha_sources_are_linear_and_shared_alpha_applies_once(self):
        self.filter_triangle(lambda x, y, q: (.5, .5))
        Image.new("RGBA", (1, 1), (255, 255, 255, 128)).save(self.folder / "plane_diffuse.tga")
        Image.new("L", (1, 1), 64).save(self.folder / "gray.png")
        Image.new("RGB", (1, 1), (0, 32, 255)).save(self.folder / "rgb.png")
        self.screen_mesh("black", [(20, 10), (140, 10), (80, 115)], [(1, 2, 3)], (0, 0, 0), 3)
        for setting, opacity in (([], 128/255), (["map_d plane_diffuse.tga"], 128/255),
                                 (["map_d gray.png"], 64/255), (["map_d -imfchan g rgb.png"], 32/255)):
            self.material("plane", "d 0.5", *setting)
            im = self.frame("--mode", "albedo", "--model", "plane.obj", "--model", "black.obj",
                            cwd=self.folder)
            self.assertEqual(im.getpixel((80, 63)), (display_byte(.5*opacity),)*3, setting)

    def test_filtered_alpha_after_clipping_and_per_sample_cutoff(self):
        alpha = Image.new("L", (8, 1))
        alpha.putdata([0, 0, 0, 240, 0, 0, 0, 0])
        alpha.save(self.folder / "opacity.png")
        Image.new("RGB", (1, 1), "white").save(self.folder / "plane_diffuse.tga")
        rate = .8 * 2**1.25 / 8
        self.filter_triangle(lambda x, y, q: (.4375 + rate*y/q, .5))
        self.material("plane", "map_d opacity.png", "alpha_mode mask")
        for mode, expected in (("nearest", 240), ("bilinear", 240), ("mipmap", 90), ("trilinear", 76.875)):
            for delta, pixel in ((-2, (255, 255, 255)), (2, (209, 195, 177))):
                im = self.frame("--mode", "albedo", "--model", "plane.obj", "--filter", mode,
                                "--alpha-cutoff", str((expected+delta)/255), cwd=self.folder)
                self.assertEqual(im.getpixel((80, 63)), pixel, (mode, delta))
        alpha = Image.new("L", (2, 1))
        alpha.putdata([0, 255])
        alpha.save(self.folder / "opacity.png")
        self.filter_triangle(lambda x, y, q: (.5 + .12*x/q, .5))
        self.material("plane", "map_d opacity.png")
        self.screen_mesh("black", [(20, 10), (140, 10), (80, 115)], [(1, 2, 3)], (0, 0, 0), 3)
        for raster in ("bbox", "scanline"):
            im = self.frame("--mode", "albedo", "--model", "plane.obj", "--model", "black.obj",
                            "--filter", "bilinear", "--msaa", "4", "--raster", raster, cwd=self.folder)
            self.assertEqual(im.getpixel((80, 63)), (128, 128, 128))

    def test_masked_shadow_matches_geometric_opening(self):
        def panel(name, x0, y0, x1, y1, z, color):
            xy = [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]
            rows = [f"v {x} {y} {z}" for x, y in xy] + ["vt 0 0", "vt 1 0", "vt 1 1", "vt 0 1",
                                                         "f 1/1 2/2 3/3", "f 1/1 3/3 4/4"]
            (self.folder / (name+".obj")).write_text("\n".join(rows))
            Image.new("RGB", (1, 1), color).save(self.folder / (name+"_diffuse.tga"))
            self.material(name, "map_Ks black.png")
        Image.new("L", (1, 1), 0).save(self.folder / "black.png")
        panel("receiver", -3, -3, 3, 3, 0, (200, 200, 200))
        panel("masked", -.2, -.2, .6, .6, .5, (180, 0, 0))
        panel("half", -.2, -.2, .2, .6, .5, (180, 0, 0))
        mask = Image.new("L", (2, 1))
        mask.putdata([255, 0])
        mask.save(self.folder / "cut.png")
        self.material("masked", "map_d cut.png", "map_Ks black.png")
        def render(name, mode, *options):
            return self.frame("--mode", mode, "--model", name+".obj", "--model", "receiver.obj",
                              "--shadow-size", "512", *options, size="321x241", cwd=self.folder)
        for filtering in ("hard", "pcf"):
            expected = render("half", "shadow", "--shadow-filter", filtering)
            actual = render("masked", "shadow", "--shadow-filter", filtering)
            shadow = ImageChops.difference(expected, render("half", "lit")).getchannel("G")
            self.assertGreater(sum(shadow.histogram()[1:]), 100)
            diff = ImageChops.difference(actual, expected)
            errors = sum(count for count, color in diff.getcolors(diff.width * diff.height) if any(color))
            self.assertLess(errors, 20, (filtering, errors))
        for ao in ("ssao", "brute"):
            target = self.folder / "ao.tga"
            def visibility(name):
                render(name, "shadow", "--ao", ao, "--ao-map", str(target))
                with Image.open(target) as im:
                    return im.copy()
            diff = ImageChops.difference(visibility("masked"), visibility("half"))
            self.assertLess(ImageStat.Stat(diff).mean[0], .2)

    def test_blended_material_preserves_depth_shadows_and_ao(self):
        # The cutout fixture has a receiver and a floating card. Keep the same geometry
        # and camera for all runs, varying only the opacity of the card.
        assets = PROJECT / "resource/models/alpha"
        for filename in ("cutout.obj", "tiles.png", "screen.png", "holes.png"):
            (self.folder / filename).write_bytes((assets / filename).read_bytes())
        mtl = self.folder / "panels.mtl"
        materials = (assets / "panels.mtl").read_text().split("newmtl Blue")[0]
        ao_path = self.folder / "ao.tga"
        def render(dissolve, mode, *options):
            mtl.write_text(materials.replace("map_d holes.png", f"d {dissolve}\nalpha_mode blend"))
            return self.frame("--model", "cutout.obj", "--mode", mode, "--hdr", "--msaa", "4",
                              *options, cwd=self.folder)
        self.assertEqual(render(0, "depth").tobytes(), render(.6, "depth").tobytes())
        for ao in ("ssao", "brute"):
            args = ("--ao", ao, "--ao-map", str(ao_path), "--shadow-size", "256")
            bare = render(0, "shadow", *args)
            with Image.open(ao_path) as im:
                ao_without_card = im.tobytes()
            glass = render(.6, "shadow", *args)
            with Image.open(ao_path) as im:
                self.assertEqual(ao_without_card, im.tobytes())
            self.assertNotEqual(glass.tobytes(), bare.tobytes())
            lit_change = ImageChops.difference(render(0, "lit"), render(.6, "lit"))
            # Color may change only on the card, including with AO and shadows enabled.
            shadow_change = ImageChops.difference(bare, glass)
            self.assertEqual(shadow_change.getbbox(), lit_change.getbbox())
            coverage = lit_change.convert("L").point(lambda value: 255 if value else 0)
            self.assertIsNone(Image.composite(Image.new("RGB", bare.size), shadow_change, coverage).getbbox())

    def test_alpha_rejects_invalid_controls(self):
        for option, value in (("--alpha", "glass"), ("--alpha-cutoff", "-0.01"),
                              ("--alpha-cutoff", "1.1"), ("--alpha-cutoff", "nan")):
            result = subprocess.run([str(EXE), option, value, "--output", str(self.folder / "bad.tga")],
                                    cwd=PROJECT, capture_output=True)
            self.assertNotEqual(result.returncode, 0, (option, value))

    def ibl_material(self, normal, orm, albedo=(255, 255, 255)):
        # The center camera ray hits the origin. Assign one known shading normal to the triangle.
        (self.folder / "ibl.obj").write_text(
            "mtllib ibl.mtl\nv -1 -1 0\nv 1 -1 0\nv 0 1 0\nvn "
            + " ".join(map(str, normal)) + "\nusemtl sample\nf 1//1 2//1 3//1\n")
        (self.folder / "ibl.mtl").write_text("newmtl sample\nmap_Kd base.tga\nmap_Ka orm.tga\n")
        Image.new("RGB", (1, 1), albedo).save(self.folder / "base.tga")
        Image.new("RGB", (1, 1), orm).save(self.folder / "orm.tga")

    def ibl_pixel(self, *options):
        return self.frame("--model", "ibl.obj", "--mode", "pbr", "--env", "probe.hdr",
                          *options, cwd=self.folder).getpixel((80, 63))

    def test_ibl_constant_hdr_and_split_sum(self):
        radiance = (4, 1, 0.25)
        write_environment(self.folder / "probe.hdr", lambda x,y,d: radiance, 10, 5)
        for roughness, response in ((0, 1), (255, 1 - math.log(2))):
            # White metal at normal incidence. Unit roughness has an analytic Smith-GGX integral.
            self.ibl_material(unit((.55, .15, 2.9)), (255, roughness, 255))
            direct = self.ibl_pixel("--env-strength", "0")
            for strength, angle in ((0.5, "0"), (1, "90"), (1, "-270")):
                actual = self.ibl_pixel("--env-strength", str(strength), "--env-rotation", angle)
                for c, d, value in zip(radiance, direct, actual):
                    base = srgb_decode(d) / (1 - srgb_decode(d))
                    self.assertAlmostEqual(value, display_byte(base + strength*response*c), delta=1)

    def test_ibl_diffuse_convolution_rotation_and_ao(self):
        # For L(w) = 1 + 0.75*w.x, normalized Lambert convolution is 1 + 0.5*N.x.
        write_environment(self.folder / "probe.hdr", lambda x,y,d: (1 + 0.75*d[0],)*3, 64, 32)
        n = unit((.55, .15, 2.9))
        self.ibl_material(n, (255, 0, 0))
        d = srgb_decode(self.ibl_pixel("--env-strength", "0")[0])
        direct = d/(1-d)
        for angle, nx in ((0, n[0]), (90, -n[2]), (450, -n[2])):
            actual = self.ibl_pixel("--env-rotation", str(angle))[0]
            expected = direct + 0.96*(1 + 0.5*nx) + 0.04*(1 + 0.75*nx)
            self.assertAlmostEqual(actual, display_byte(expected), delta=1)
        self.ibl_material(n, (0, 0, 0))
        self.assertEqual(self.ibl_pixel("--env-rotation", "90"), self.ibl_pixel("--env-strength", "0"))

    def test_ibl_glossy_reflection_spreads_with_roughness(self):
        v = unit((.55, .15, 2.9))
        def light(x, y, direction):
            inside = sum(a*b for a,b in zip(v, direction)) > math.cos(math.radians(13))
            return (8, 8, 8) if inside else (0, 0, 0)
        write_environment(self.folder / "probe.hdr", light, 64, 32)
        response = []
        for roughness in (0, 153):
            self.ibl_material(v, (255, roughness, 255))
            off = self.ibl_pixel("--env-strength", "0")[0]
            response.append([self.ibl_pixel("--env-rotation", angle)[0]-off for angle in ("0", "40")])
        smooth, rough = response
        self.assertGreater(smooth[0], rough[0] + 30)
        self.assertEqual(smooth[1], 0)
        self.assertGreater(rough[1], 10)

    def test_ibl_wraps_longitude_and_clamps_poles(self):
        def stripes(x, y, d):
            if y == 0: return (0, 2, 0)
            if y == 7: return (2, 0, 0)
            return (1, 0, 0) if x == 0 else ((0, 0, 1) if x == 15 else (0, 0, 0))
        write_environment(self.folder / "probe.hdr", stripes, 16, 8)
        v = unit((.55, .15, 2.9))
        for reflected, env in (((-1,0,0), (.5,0,.5)), ((0,1,0), (0,2,0)), ((0,-1,0), (2,0,0))):
            self.ibl_material(unit(tuple(a+b for a,b in zip(v, reflected))), (255, 0, 255))
            direct = self.ibl_pixel("--env-strength", "0")
            for actual, d, target in zip(self.ibl_pixel(), direct, env):
                base = srgb_decode(d)/(1-srgb_decode(d))
                self.assertAlmostEqual(actual, display_byte(base+target), delta=1)

    def test_ibl_selects_pbr_hdr_and_preserves_direct_light(self):
        self.ibl_material(unit((.55, .15, 2.9)), (0, 130, 80))
        write_environment(self.folder / "probe.hdr", lambda x,y,d: (1,1,1), 10, 5)
        args = ("--model", "ibl.obj", "--env", "probe.hdr")
        implicit = self.frame(*args, cwd=self.folder)
        explicit = self.frame(*args, "--mode", "pbr", "--hdr", cwd=self.folder)
        direct = self.frame("--model", "ibl.obj", "--mode", "pbr", "--hdr", cwd=self.folder)
        self.assertEqual(implicit.tobytes(), explicit.tobytes())
        self.assertEqual(implicit.tobytes(), direct.tobytes())  # zero material AO removes only indirect light

    def test_ibl_rejects_invalid_environment_options(self):
        Image.new("RGB", (8, 4)).save(self.folder / "ldr.png")
        write_environment(self.folder / "square.hdr", lambda x,y,d: (1,1,1), 4, 4)
        for args in (("--env", "missing.hdr"), ("--env", "ldr.png"), ("--env", "square.hdr"),
                     ("--env-rotation", "90"), ("--env", "studio", "--env-strength", "-1"),
                     ("--env", "studio", "--env-rotation", "nan"), ("--env", "studio", "--mode", "lit")):
            target = self.folder / "bad.tga"
            result = subprocess.run([str(EXE), "--mode", "pbr", *args, "--output", str(target)],
                                    cwd=self.folder, capture_output=True)
            self.assertNotEqual(result.returncode, 0, args)
            self.assertFalse(target.exists())

    def test_hdr_pbr_exposes_linear_material_radiance_once(self):
        # The shading normal faces away from all four point lights; only the PBR ambient term remains.
        (self.folder / "mat.obj").write_text(
            "mtllib mat.mtl\nv -1 -1 0\nv 1 -1 0\nv 0 1 0\nvn 0 0 -1\nusemtl cloth\nf 1//1 2//1 3//1\n")
        (self.folder / "mat.mtl").write_text("newmtl cloth\nmap_Kd color.tga\nmap_Ka orm.tga\n")
        albedo = (230, 90, 8)
        Image.new("RGB", (1, 1), albedo).save(self.folder / "color.tga")
        Image.new("RGB", (1, 1), (128, 160, 0)).save(self.folder / "orm.tga")
        pixel = self.frame("--model", "mat.obj", "--mode", "pbr", "--exposure", "4",
                           cwd=self.folder).getpixel((80, 63))
        expected = tuple(display_byte(0.03 * (128/255) * srgb_decode(c), 4) for c in albedo)
        self.assertEqual(pixel, expected)

    def test_hdr_msaa_resolves_radiance_before_tone_mapping(self):
        mesh = self.screen_mesh("lamp", [(80.30,63.07), (80.46,63.07), (80.375,63.23)],
                                [(1,2,3)], (0, 0, 0))
        Image.new("RGB", (1, 1), (255, 0, 0)).save(self.folder / "lamp_glow.tga")
        frame = self.frame("--mode", "emission", "--model", mesh, "--msaa", "4",
                           "--emission-strength", "8", cwd=self.folder)
        # One bright red sample and three uncovered samples of the beige clear color.
        expected = tuple(display_byte((3 * srgb_decode(bg) + light) / 4)
                         for bg, light in zip((209, 195, 177), (8, 0, 0)))
        self.assertEqual(frame.getpixel((80, 63)), expected)

    def test_bloom_only_spreads_excess_scene_luminance(self):
        mesh = self.screen_mesh("lamp", [(78,60), (84,60), (84,66), (78,66)],
                                [(1,2,3), (1,3,4)], (0, 0, 0))
        Image.new("RGB", (1, 1), (255, 0, 0)).save(self.folder / "lamp_glow.tga")
        def render(strength, bloom, ev):
            return self.frame("--model", mesh, "--mode", "emission", "--emission-strength",
                              str(strength), "--bloom", str(bloom), "--exposure", str(ev), cwd=self.folder)
        # Pure red radiance 4 has luminance 0.8504, even if camera exposure is high.
        self.assertEqual(render(4, 0, 2).tobytes(), render(4, 1, 2).tobytes())
        base, bloom = render(32, 0, -1), render(32, 1, -1)
        inside, near, far = (80, 63), (85, 63), (10, 63)
        self.assertGreater(bloom.getpixel(inside)[0], base.getpixel(inside)[0])
        self.assertGreater(bloom.getpixel(near)[0], base.getpixel(near)[0])
        self.assertEqual(bloom.getpixel(near)[1:], base.getpixel(near)[1:])
        self.assertEqual(bloom.getpixel(far), base.getpixel(far))

    def test_occluded_emission_cannot_contribute_to_bloom(self):
        faces = [(1,2,3), (1,3,4)]
        lamp = self.screen_mesh("lamp", [(76,58), (84,58), (84,66), (76,66)], faces, (0,0,0), 3)
        lid = self.screen_mesh("cover", [(60,45), (100,45), (100,80), (60,80)], faces, (0,0,0), 2)
        Image.new("L", (1, 1), 255).save(self.folder / "lamp_glow.tga")
        for name in ("lamp", "cover"):
            Image.new("L", (1, 1), 0).save(self.folder / (name + "_spec.tga"))
        args = ("--mode", "shadow", "--shadow-size", "128", "--msaa", "4", "--ao", "ssao",
                "--emission-strength", "32", "--bloom", "1")
        reference = self.frame(*args, "--model", lid, cwd=self.folder)
        for first, second in ((lamp, lid), (lid, lamp)):
            frame = self.frame(*args, "--model", first, "--model", second, cwd=self.folder)
            self.assertEqual(reference.tobytes(), frame.tobytes())

    def test_display_post_process_reads_finished_hdr_frame(self):
        args = ("--mode", "shadow", "--exposure", "-1", "--emission-strength", "64", "--bloom", "2")
        image = self.frame(*args)
        gray = self.frame(*args, "--post", "gray")
        self.assertEqual(gray.getchannel("R").tobytes(), gray.getchannel("B").tobytes())
        diff = ImageChops.difference(gray.getchannel("R"), image.convert("L"))
        self.assertLessEqual(diff.getextrema()[1], 1)

    def test_hdr_diagnostics_and_invalid_controls(self):
        for mode in ("normal", "depth", "wire", "barycentric"):
            plain = self.frame("--mode", mode)
            hdr = self.frame("--mode", mode, "--exposure", "4", "--bloom", "1")
            self.assertEqual(plain.tobytes(), hdr.tobytes(), mode)
        for option, value in (("--exposure", "nan"), ("--exposure", "17"), ("--bloom", "-1"),
                              ("--bloom", "2abc"), ("--emission-strength", "inf")):
            result = subprocess.run([str(EXE), option, value, "--output", str(self.folder / "bad.tga")],
                                    cwd=PROJECT, capture_output=True)
            self.assertNotEqual(result.returncode, 0, (option, value))

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
