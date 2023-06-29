#include "tgaimage/tgaimage.h"
#include "utils/Line.h"
#include "renderer/model.h"

int main()
{
	//TGAImage image(100, 100, TGAImage::RGB);
	//TGAColor red = TGAColor(255, 0, 0, 255);
	//TGAColor green = TGAColor(0, 255, 0, 255);
	//TGAColor blue = TGAColor(0, 0, 255, 255);
	////DrawLine2(20, 20, 80, 80, image, green);
	////DrawLine2(20, 20, 40, 80, image, red);
	////DrawLine3(20, 20, 30, 80, image, blue);
	////DrawLine3(20, 20, 80, 30, image, blue);
	//DrawLine3(25, 25, 35, 85, image, green);
	//DrawLine3(25, 25, 85, 35, image, green);

	//DrawLine4(20, 20, 80, 30, image, red);
	//DrawLine4(20, 20, 30, 80, image, red);
	//image.flip_vertically();
	//image.write_tga_file("output.tga");
	uint32_t width = 1000;
	uint32_t height = 1000;
	TGAImage image(width, height, TGAImage::RGB);
	TGAColor white = { 255, 255, 255, 255, 4 * 8 };
	Model model{ "source/models/diablo/diablo3_pose.obj" };
	for (int i = 0; i < model.nfaces(); i++) {
		for (int j = 0; j < 3; j++) {
			vec3 v0 = model.vert(i, j);
			vec3 v1 = model.vert(i, (j + 1) % 3);
			/*Vec3f v0 = model->vert(face[j]);
			Vec3f v1 = model->vert(face[(j + 1) % 3]);*/
			int x0 = (v0.x + 1.) * width / 2.;
			int y0 = (v0.y + 1.) * height / 2.;
			int x1 = (v1.x + 1.) * width / 2.;
			int y1 = (v1.y + 1.) * height / 2.;
			DrawLine4(x0, y0, x1, y1, image, white);
		}
	}
	image.write_tga_file("output.tga");
}