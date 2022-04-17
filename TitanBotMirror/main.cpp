#include <windows.h>
#include <windowsx.h>
#include <time.h>

#include <string>
#include <fstream>
#include <iostream>
#include <opencv2\opencv.hpp>
#include <opencv2\imgcodecs.hpp>
#include <opencv2\highgui\\highgui.hpp>
#include <vector>
#include "GDString2.h"

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>

#define DEVICE_WIDTH 720//720//480
#define DEVICE_HEIGHT 1280//1280//854

using namespace cv;
using namespace std;

vector<int> compression_params;

Scalar ScalarHSV2BGR(uchar H, uchar S, uchar V) {
	Mat rgb;
	Mat hsv(1, 1, CV_8UC3, Scalar(H, S, V));
	cvtColor(hsv, rgb, CV_HSV2BGR);
	return Scalar(rgb.data[0], rgb.data[1], rgb.data[2]);
}

void saveShot(Mat shot) {
	time_t now = time(0);
	char buf[30] = "";
	_itoa_s(now, buf, 30, 10);
	string filename = (string)buf + ".bmp";
	imwrite(filename.c_str(), shot, compression_params);
}

struct PixelRGB
{
	unsigned char r, g, b;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	Mat cv_rgb_image(DEVICE_HEIGHT, DEVICE_WIDTH, CV_8UC3);
	for (unsigned i = 0; i < cv_rgb_image.rows; i++)
	{
		for (unsigned j = 0, k = 0; j < cv_rgb_image.step; j += 3, k++)
		{
			cv_rgb_image.data[DEVICE_WIDTH * 3 * i + j] = (char)((i >> 8) << 4) | (k >> 8);;
			cv_rgb_image.data[DEVICE_WIDTH * 3 * i + j + 1] = (char) i;
			cv_rgb_image.data[DEVICE_WIDTH * 3 * i + j + 2] = (char) k;
		}
	}
	saveShot(cv_rgb_image);
	return 0;
}
