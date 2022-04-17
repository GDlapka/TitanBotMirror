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

#define DEVICE_WIDTH 480//720//480
#define DEVICE_HEIGHT 854//1280//854

using namespace cv;
using namespace std;

vector<int> compression_params;

Mat shot;

Scalar ScalarHSV2BGR(uchar H, uchar S, uchar V) {
	Mat rgb;
	Mat hsv(1, 1, CV_8UC3, Scalar(H, S, V));
	cvtColor(hsv, rgb, CV_HSV2BGR);
	return Scalar(rgb.data[0], rgb.data[1], rgb.data[2]);
}

void saveShot(void) {
	time_t now = time(0);
	char buf[30] = "";
	_itoa_s(now, buf, 30, 10);
	string filename = (string)"shots\\" + buf + ".bmp";
	imwrite(filename.c_str(), shot, compression_params);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	return 0;
}
