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

#define POS_MW_WIDTH 560
#define POS_MW_HEIGHT 350
#define CONTROLBAR_HEIGHT 60

#define BORDER_SPACE_X 26 //поправка на границу окна
#define BORDER_SPACE_Y 5 //поправка на границу окна

#define DEVICE_WIDTH 480//720//480
#define DEVICE_HEIGHT 854//1280//854
int deviceWidth = DEVICE_WIDTH;
int deviceHeight = DEVICE_HEIGHT;
float deviceAspectRatio = (float)deviceHeight / deviceWidth;

#define SWIPE_MIN_LIMIT 80

#define HMENU_CHECK_ORIENTATION 8001
#define HMENU_CHECK_CLICK 8002
#define HMENU_BTN_SETHV 8003
#define HMENU_BTN_LOAD 8004
#define HMENU_CHECK_AUTO 8005
#define HMENU_BTN_BACK 8006
#define HMENU_BTN_HOME 8007
#define HMENU_BTN_RECENT 8008
#define HMENU_BTN_SAVE 8009
#define HMENU_CHECK_LOAD 8010
#define HMENU_CHECK_FORCE_INVERT 8011
#define HMENU_CHECK_SCREENXY 8012

int mwWidth = POS_MW_WIDTH;
int mwHeight = POS_MW_HEIGHT;

HWND hWnd = NULL;
HWND hControlBar = NULL;
HWND hMirror = NULL; //окно просмотра
HWND hCheckOrientation = NULL;
HWND hCheckRemoteControl = NULL;
HWND hCV = NULL;
HWND hCkClick = NULL;
HWND hDeviceChooser = NULL;
HWND hBtnLoadScreen = NULL;
HWND hCkAutoLoad = NULL;
HWND hBtnBack = NULL;
HWND hBtnHome = NULL;
HWND hBtnRecent = NULL;
HWND hBtnSave = NULL;
HWND hCkScreenDownload = NULL;
HWND hCkForceInvert = NULL;
HWND hCkScreenXY = NULL;

const WCHAR g_szClassName[] = L"TitanBotMirrorClass";
const WCHAR szControlBarClass[] = L"TitanBotMirrorControlWindowClass";
const WCHAR szMirrorWindowClass[] = L"TitanBotMirrorWindowClass";

using namespace std;
wstring workDir = L"";
wstring adbDir = L"C:\\tools";

const WCHAR mwCaption[] = L"TitanBot Mirror v1.2beta";

bool isLoaded = false;
int lBtnDownLastPosX = 0;
int lBtnDownLastPosY = 0;

bool isScrLoader = true;

bool screenIsLandscape = false;

using namespace cv;

Mat mirror(deviceWidth, deviceHeight, CV_8UC3);
Mat scaled(deviceWidth, deviceHeight, CV_8UC3);
Mat emptyMat = Mat::zeros(deviceWidth, deviceHeight, CV_8UC3);
cv::Point* point;
cv::Size mirrorWindowSize(mwWidth, mwHeight);

vector<int> compression_params;

void addDeviceNumberIfChoosen(wstring*);

char androidVersion = 0;

std::string exec(const char* cmd) {
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
	if (!pipe) {
		throw std::runtime_error("_popen() failed!");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}
	return result;
}

void chechErrorsMatrix(void) {
	if (mirror.size() != emptyMat.size() || mirror.cols != deviceWidth || emptyMat.cols != deviceWidth ||
		mirror.rows != deviceHeight || emptyMat.rows != deviceHeight) {
		cv::Size size(deviceWidth, deviceHeight);
		resize(mirror, mirror, size);
		resize(emptyMat, emptyMat, size);
	}
}

bool chechSpaceMatrix(int length) {
	int currentLen = (mirror.size()).width * (mirror.size()).height;
	if (length == currentLen * 3) return true;
	else return false;
}

void getDeviceParams(void) {
	wstring wcommand = adbDir + L"\\adb ";
	addDeviceNumberIfChoosen(&wcommand);
	string appeal = wide2str(wcommand);
	string command = appeal + "shell \"dumpsys input \| grep \'SurfaceOrientation\'; getprop ro.build.version.release; wm size\"";
	string output = exec(command.c_str());

	output.erase(std::remove(output.begin(), output.end(), '\r'), output.end());

	size_t pos = output.rfind('\n');
	if (output.empty()) return;
	if (pos != 0) output.erase(pos);
	pos = output.rfind(' ');
	string buf;
	if (pos + 1 != output.npos) buf = output.substr(pos+1);
	if (pos != 0) output.erase(pos);
	pos = output.rfind('\n');
	if (pos != 0) output.erase(pos);
	if (!buf.empty()) {
		size_t pos2 = buf.find('x');
		if (pos2 != 0 && pos2 < buf.npos - 1) {
			int x = atoi((buf.substr(pos2 + 1)).c_str());
			int y = atoi((buf.substr(0, pos2)).c_str());
			if (x > y) {
				deviceWidth = x; deviceHeight = y;
			}
			else {
				deviceWidth = y; deviceHeight = x;
			}
			deviceAspectRatio = (float)deviceWidth / deviceHeight;
		}
	}
	pos = output.rfind('\n');
	if (pos + 1 != output.npos) buf = output.substr(pos + 1);
	if (!buf.empty()) androidVersion = buf[0];
	pos = output.rfind('\n');
	if (pos != 0) output.erase(pos);
	if (!output.empty()) screenIsLandscape = (output[output.size() - 1] == '1');
}

void str2Mat(string* str, Mat* mat) {
	chechErrorsMatrix();
	if (!chechSpaceMatrix(str->size())) {
		MessageBox(NULL, L"Matrix space unmatch!", L"Error", MB_OK);
		return;
	}
	if (androidVersion == '5') {
		int i = 0;
		for (int x = mat->cols - 1; x >= 0; x--) {
			for (int y = 0; y < mat->rows; y++) {
				cv::Vec3b* ptr = mat->ptr<cv::Vec3b>(y);
				unsigned char red, green, blue;
				red = str->at(i * 3);
				green = str->at(i * 3 + 1);
				blue = str->at(i * 3 + 2);
				ptr[x] = cv::Vec3b(blue, green, red);
				i++;
			}
		}
		cv::flip(*mat, *mat, -1);
	}
	else {
		int i = 0;
		if (screenIsLandscape) {
			for (int y = 0; y < mat->rows; y++) {
				//for (int x = 0; x < mat->cols; x++) {
				//for (int y = mat->rows - 1; y >= 0; y--) {
				for (int x = mat->cols - 1; x >= 0; x--) {
					cv::Vec3b* ptr = mat->ptr<cv::Vec3b>(y);
					unsigned char red, green, blue;
					red = str->at(i * 3);
					green = str->at(i * 3 + 1);
					blue = str->at(i * 3 + 2);
					ptr[x] = cv::Vec3b(blue, green, red);
					i++;
				}
			}
			cv::flip(*mat, *mat, 1);
		}
		else {
			for (int x = mat->cols - 1; x >= 0; x--) {
				//for (int x = 0; x < mat->cols; x++) {
				//for (int y = mat->rows - 1; y >= 0; y--) {
				for (int y = 0; y < mat->rows; y++) {
					cv::Vec3b* ptr = mat->ptr<cv::Vec3b>(y);
					unsigned char red, green, blue;
					red = str->at(i * 3);
					green = str->at(i * 3 + 1);
					blue = str->at(i * 3 + 2);
					ptr[x] = cv::Vec3b(blue, green, red);
					i++;
				}
			}
			cv::flip(*mat, *mat, -1);
		}
	}
}

void getDevicesFile(void) {
	wifstream in(L"devices.txt"); // окрываем файл для чтения
	if (in.is_open()) {
		WCHAR buf;
		wstring line = L"";
		while (getline(in, line)) ComboBox_AddString(hDeviceChooser, line.c_str());
		in.close();
	}
}

void openDump(Mat* outImage) {
	ifstream fin;
	string dumpFile = wide2str(workDir + L"\\screen.png");

	if (GetFileAttributesA(dumpFile.c_str()) == INVALID_FILE_ATTRIBUTES) return;
	isLoaded = true;
	*outImage = imread(dumpFile.c_str());
	if (screenIsLandscape) {
		//cv::transpose(*outImage, *outImage);
		//cv::flip(*outImage, *outImage, 0);
	} else {
		cv::transpose(*outImage, *outImage);
		cv::flip(*outImage, *outImage, 0);
	}
	/*fin.open(dumpFile.c_str(), ios_base::binary);
	if (!fin.is_open()) isLoaded = false;
	else {
		//conversion
		string inStr, outStr;
		char buf;
		while (true)
		{
			fin.read(&buf, sizeof(char));
			if ((fin.rdstate() & std::ifstream::eofbit) != 0)
				break;
			else inStr.push_back(buf);
		}

		fin.close();

		int siz = inStr.size();



		//алгоритм
		if (androidVersion == '9') inStr.erase(0, 16);
		else inStr.erase(0, 12);
		siz = outStr.size();
		siz = 0;
		int i = 0;
		for (string::iterator it = inStr.begin(); it < inStr.end(); ++it) {
			i++;
			if (i % 4 == 0)	continue;
			siz++;
			outStr.push_back(*it);
		}
		siz = outStr.size();
		str2Mat(&outStr, outImage);
	}*/
}

void paintHCV(RECT* mwRect) {

	RECT rect = { mwRect->left, mwRect->top + 45, mwRect->right, mwRect->bottom - CONTROLBAR_HEIGHT};
	
	int width = rect.right - rect.left - BORDER_SPACE_X;
	int height = rect.bottom - rect.top - BORDER_SPACE_Y;
	float currentAspectRatio;
	if (Button_GetCheck(hCheckOrientation)) {
		//Landscape
		currentAspectRatio = (float)width / height;
		if (currentAspectRatio >= deviceAspectRatio) {
			mirrorWindowSize.height = height;
			mirrorWindowSize.width = round(((float)height) * deviceAspectRatio);
		}
		else {
			mirrorWindowSize.width = width;
			mirrorWindowSize.height = round(((float)width) / deviceAspectRatio);
		}
	}
	else {
		//Portrait
		currentAspectRatio = (float)height / width;
		if (currentAspectRatio < deviceAspectRatio) {
			mirrorWindowSize.height = round(((float)height) / deviceAspectRatio);
			mirrorWindowSize.width = height;
		}
		else {
			mirrorWindowSize.width = round(((float)width) * deviceAspectRatio);
			mirrorWindowSize.height = width;
		}
	}
	//currentAspectRatio = (float)size.width / size.height;
	if (isLoaded) resize(mirror, scaled, mirrorWindowSize);
	else resize(emptyMat, scaled, mirrorWindowSize);

	if (hCheckOrientation != NULL) {
		if (!Button_GetCheck(hCheckOrientation)) {
			transpose(scaled, scaled);
			cv::flip(scaled, scaled, 1);
		}
	}

	imshow("TitanBotCV", scaled);
			
	SetWindowPos(hCV, NULL, (mwRect->right - mwRect->left - scaled.cols - BORDER_SPACE_X) / 2, 0, rect.right - rect.left, rect.bottom - rect.top, NULL);
}

void addDeviceNumberIfChoosen(wstring* command) {
	if (ComboBox_GetCurSel(hDeviceChooser) > 0) {
		//Not one device
		int index = ComboBox_GetCurSel(hDeviceChooser);
		WCHAR line[100] = L"";
		ComboBox_GetLBText(hDeviceChooser, index, line);
		wstring serial = line;
		size_t pos = serial.rfind(' ');
		if (pos > 1) serial.erase(0, pos);
		*command += (wstring)L"-s" + serial + L" ";
	}
}

void loadScreen(void) {
	wstring dumpFile = workDir + L"\\screen.png";
	if (GetFileAttributes(dumpFile.c_str()) != INVALID_FILE_ATTRIBUTES) _wremove(dumpFile.c_str());
	wstring command = adbDir + L"\\adb ";
	addDeviceNumberIfChoosen(&command);

	WCHAR* tmp_name = _wtempnam(NULL, NULL);
	auto cmd_name = std::wstring(tmp_name) + L".cmd";
	{
		std::wofstream f(cmd_name);
		f << (command + L"shell screencap \/sdcard\/scripts\/_out_scr.png" + L"\n").c_str();
		f << (command + L"pull \/sdcard\/scripts\/_out_scr.png " + dumpFile + L"\n").c_str();
		f << "Exit \/B\n";
	}
	int result = _wsystem(cmd_name.c_str());
	_wremove(cmd_name.c_str());
	free(tmp_name);

	getDeviceParams();
	if (Button_GetCheck(hCkForceInvert)) screenIsLandscape = !(screenIsLandscape);
	openDump(&mirror);

	RECT rect; 
	GetWindowRect(hWnd, &rect);
	paintHCV(&rect);
}

Scalar ScalarHSV2BGR(uchar H, uchar S, uchar V) {
	Mat rgb;
	Mat hsv(1, 1, CV_8UC3, Scalar(H, S, V));
	cvtColor(hsv, rgb, CV_HSV2BGR);
	return Scalar(rgb.data[0], rgb.data[1], rgb.data[2]);
}

void sendTap(int x, int y) {
	if (Button_GetCheck(hCkClick)) {
		wstring command;
		wstring coords;
		WCHAR buf[20] = L"";
		_itow_s(x, buf, 20, 10);
		coords = (wstring)buf + L" ";
		_itow_s(y, buf, 20, 10);
		coords += buf;
		command = adbDir + L"\\adb ";
		addDeviceNumberIfChoosen(&command);
		command += L"shell input tap " + coords;
		_wsystem(command.c_str());
		if (isScrLoader) {
			Sleep(1000);
			loadScreen();
		}
	}
}

void sendSwipe(int x, int y) {
	if (Button_GetCheck(hCkClick)) {
		wstring command;
		wstring coords;
		WCHAR buf[20] = L"";
		_itow_s(lBtnDownLastPosX, buf, 20, 10);
		coords = (wstring)buf + L" ";
		_itow_s(lBtnDownLastPosY, buf, 20, 10);
		coords += (wstring)buf + L" ";
		_itow_s(x, buf, 20, 10);
		coords += (wstring)buf + L" ";
		_itow_s(y, buf, 20, 10);
		coords += (wstring)buf + L" 300";

		command = adbDir + L"\\adb ";
		addDeviceNumberIfChoosen(&command);
		command += L"shell input swipe " + coords;
		_wsystem(command.c_str());
		if (isScrLoader) {
			Sleep(1000);
			loadScreen();
		}
	}
}

void sendHold(int x, int y) {
	if (Button_GetCheck(hCkClick)) {
		wstring command;
		wstring coords;
		WCHAR buf[20] = L"";
		_itow_s(x, buf, 20, 10);
		coords = (wstring)buf + L" ";
		_itow_s(y, buf, 20, 10);
		coords += (wstring)buf + L" ";
		_itow_s(x, buf, 20, 10);
		coords += (wstring)buf + L" ";
		_itow_s(y, buf, 20, 10);
		coords += (wstring)buf + L" 2000";

		command = adbDir + L"\\adb ";
		addDeviceNumberIfChoosen(&command);
		command += L"shell input swipe " + coords;
		_wsystem(command.c_str());
		if (isScrLoader) {
			Sleep(1000);
			loadScreen();
		}
	}
}

void convertCoordsToDevice(int* inX, int* inY, int* outX, int* outY) {
	if (Button_GetCheck(hCheckOrientation)) {
		//Landscape
		*outX = round(*inX * (deviceHeight - 1) / (mirrorWindowSize.height - 1));
		*outY = round(*inY * (deviceWidth - 1) / (mirrorWindowSize.width - 1));
	}
	else {
		//Portrait
		*outX = round(*inY * (deviceWidth - 1) / (mirrorWindowSize.width - 1));
		*outY = deviceHeight - round(*inX * (deviceHeight - 1) / (mirrorWindowSize.height - 1)) - 1;
	}
}

void convertCoordsToDevice(int* inX, int* inY, int* outX, int* outY, int* tapX, int* tapY) {
	convertCoordsToDevice(inX, inY, outX, outY);
	if (Button_GetCheck(hCkScreenXY)) {
		*tapX = *outX;
		*tapY = *outY;
	}
	else {
		if (screenIsLandscape) {
			*tapX = *outX;
			*tapY = *outY;
		}
		else {
			//Portrait
			*tapX = deviceHeight - 1 - *outY;
			*tapY = *outX;
		}
	}
}

void onMouse(int evt, int x, int y, int flags, void* param) {
	int deviceX;
	int deviceY;
	int tapX;
	int tapY;
	if (evt == CV_EVENT_MOUSEMOVE) {
		convertCoordsToDevice(&x, &y, &deviceX, &deviceY, &tapX, &tapY);

		//cv::Vec3b v = mirror.at<cv::Vec3b>(deviceX, deviceY);
		//cv::Point point(deviceX, deviceY);
		Vec3b color; color[0] = 0; color[1] = 0; color[2] = 0;
		try { 
			color = mirror.at<Vec3b>(deviceY, deviceX);
		}
		catch (Exception e) {};
		WCHAR wBuf[100] = L"";
		wstring text = L"";

		_itow_s(tapX, wBuf, 100, 10);
		text = (wstring)mwCaption + L" | tapX:" + wBuf + L" tapY:";
		_itow_s(tapY, wBuf, 100, 10);
		text += (wstring)wBuf + L" | ";

		_itow_s(deviceX, wBuf, 100, 10);
		text += (wstring)L" x:" + wBuf + L" y:";
		_itow_s(deviceY, wBuf, 100, 10);
		text += (wstring)wBuf + L" ";

		text += (wstring)L"#" + intToHexWide(color[2]); 
		text += (wstring)intToHexWide(color[1]);
		text += (wstring)intToHexWide(color[0]);

		_itow_s(deviceWidth, wBuf, 100, 10);
		text += (wstring)L" [" + wBuf + L"x";
		_itow_s(deviceHeight, wBuf, 100, 10);
		text += (wstring)wBuf + L"]";

		SetWindowText(hWnd, text.c_str());
	}
	if (evt == CV_EVENT_LBUTTONDOWN) {
		convertCoordsToDevice(&x, &y, &deviceX, &deviceY, &tapX, &tapY);
		lBtnDownLastPosX = tapX;
		lBtnDownLastPosY = tapY;
	}
	if (evt == CV_EVENT_LBUTTONUP) {
		convertCoordsToDevice(&x, &y, &deviceX, &deviceY, &tapX, &tapY);
		if (abs(tapX - lBtnDownLastPosX) + abs(tapY - lBtnDownLastPosY) > SWIPE_MIN_LIMIT) {
			//swipe
			sendSwipe(tapX, tapY);
		}
		else {
			//tap
			sendTap(tapX, tapY);
		}
	}
	if (evt == CV_EVENT_RBUTTONUP) {
		convertCoordsToDevice(&x, &y, &deviceX, &deviceY, &tapX, &tapY);
		sendHold(tapX, tapY);
	}
}

void sendKeyevent(wstring keyevent) {
	wstring command;
	if (Button_GetCheck(hCkClick)) {
		command = adbDir + L"\\adb ";
		addDeviceNumberIfChoosen(&command);
		command += L"shell input keyevent " + keyevent;
		_wsystem(command.c_str());
		if (isScrLoader) {
			Sleep(1000);
			loadScreen();
		}
	}
}

void saveShot(void) {
	time_t now = time(0);
	char buf[30] = "";
	_itoa_s(now, buf, 30, 10);
	string filename = (string)"shots\\" + buf + ".bmp";
	imwrite(filename.c_str(), mirror, compression_params);
}

// Step 4: the Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WORD wmId = LOWORD(wParam);
	WORD wmEvent = HIWORD(wParam);
	//WORD lLo = LOWORD(lParam);
	//WORD lHi = HIWORD(lParam);
	RECT rect;

	switch (msg)
	{
	case WM_MOUSEMOVE:
		SetWindowText(hWnd, mwCaption);
		break;
	case WM_WINDOWPOSCHANGED:
		GetWindowRect(hWnd, &rect);
		
		SetWindowPos(hControlBar, NULL, 0, 0, rect.right - rect.left, CONTROLBAR_HEIGHT, SWP_NOMOVE);
		SetWindowPos(hMirror, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top - CONTROLBAR_HEIGHT, SWP_NOMOVE);
		if (hCV != NULL) paintHCV(&rect);
		break;
	case WM_COMMAND:
		if (wmEvent == CBN_SELCHANGE) {
			int sel = ComboBox_GetCurSel(hDeviceChooser);
			if (sel != 0 && sel != -1 && isScrLoader) loadScreen();
		}
		switch (wmId)
		{
		case HMENU_BTN_BACK:
			sendKeyevent(L"4");
			break;
		case HMENU_BTN_HOME:
			sendKeyevent(L"3");
			break;
		case HMENU_BTN_RECENT:
			sendKeyevent(L"187");
			break;
		case HMENU_BTN_LOAD:
			loadScreen();
			break;
		case HMENU_BTN_SAVE:
			saveShot();
			break;
		case HMENU_CHECK_ORIENTATION:
			if (Button_GetCheck(hCheckOrientation)) {
				//MessageBox(NULL, L"x", L"", MB_OK); 
				//Langscape
				SetWindowPos(hWnd, NULL, 0, 0, POS_MW_WIDTH, POS_MW_HEIGHT, SWP_NOMOVE);
				SetWindowPos(hControlBar, NULL, 0, 0, POS_MW_WIDTH, CONTROLBAR_HEIGHT, SWP_NOMOVE);
				SetWindowPos(hMirror, NULL, 0, 0, POS_MW_WIDTH, POS_MW_HEIGHT, SWP_NOMOVE);
			}
			else {
				//MessageBox(NULL, L"o", L"", MB_OK); 
				//Portrait
				SetWindowPos(hWnd, NULL, 0, 0, POS_MW_WIDTH, POS_MW_WIDTH, SWP_NOMOVE);
				SetWindowPos(hControlBar, NULL, 0, 0, POS_MW_WIDTH, CONTROLBAR_HEIGHT, SWP_NOMOVE);
				SetWindowPos(hMirror, NULL, 0, 0, POS_MW_WIDTH, POS_MW_WIDTH - CONTROLBAR_HEIGHT, SWP_NOMOVE);
			}
			break;
		case HMENU_CHECK_LOAD:
			isScrLoader = Button_GetCheck(hCkScreenDownload);
			break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
		break;
	case WM_CLOSE:
		if (hCV != NULL) {
			DestroyWindow(hCV);
			hCV = NULL;
		}
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wc;
	MSG Msg;

	//Step 1: Registering the Window Class
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = g_szClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Window Registration Failed!", L"Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wc.lpszClassName = szControlBarClass;

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Window Registration Failed!", L"Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	
	wc.lpszClassName = szMirrorWindowClass;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Mirror Window Registration Failed!", L"Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	// Step 2: Creating the Window
	hWnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		g_szClassName,
		mwCaption,
		WS_OVERLAPPEDWINDOW,
		//WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, POS_MW_WIDTH, POS_MW_HEIGHT + CONTROLBAR_HEIGHT,
		NULL, NULL, hInstance, NULL);

	if (hWnd == NULL)
	{
		MessageBox(NULL, L"Window Creation Failed!", L"Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	WCHAR dir[MAX_PATH] = L"";
	GetModuleFileName(NULL, dir, MAX_PATH);
	workDir = dir;
	size_t pos = workDir.rfind('\\');
	if (pos != 0) workDir.erase(pos);

	SetWindowPos(hWnd, NULL, 0, 0, POS_MW_WIDTH, POS_MW_HEIGHT, SWP_NOMOVE);

	hControlBar = CreateWindowEx(0, szControlBarClass, L"", WS_CHILDWINDOW | WS_VISIBLE/* | WS_SIZEBOX*/, 0, 0, POS_MW_WIDTH, CONTROLBAR_HEIGHT, hWnd, NULL, hInstance, NULL);
	hMirror = CreateWindowEx(0, szMirrorWindowClass, L"TitanBotMirrorWindow", WS_CHILDWINDOW | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL, 0, CONTROLBAR_HEIGHT, POS_MW_WIDTH, POS_MW_HEIGHT - CONTROLBAR_HEIGHT, hWnd, NULL, hInstance, NULL);

	hCheckOrientation = CreateWindow(L"BUTTON", L"Landscape", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 5, 5, 90, 25, hControlBar, (HMENU)HMENU_CHECK_ORIENTATION, NULL, NULL);
	Button_SetCheck(hCheckOrientation, BST_CHECKED);

	hCkClick = CreateWindow(L"BUTTON", L"Control", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 100, 5, 90, 25, hControlBar, (HMENU)HMENU_CHECK_CLICK, NULL, NULL);
	hDeviceChooser = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_DISABLENOSCROLL | WS_VSCROLL, 5, 32, 180, 120, hControlBar, NULL, NULL, NULL, NULL);
	ComboBox_AddString(hDeviceChooser, L"----[одно устройство]----");
	getDevicesFile();
	ComboBox_SetCurSel(hDeviceChooser, 0);
	hBtnLoadScreen = CreateWindow(L"BUTTON", L"Load Screen", WS_VISIBLE | WS_CHILD, 190, 32, 180, 25, hControlBar, (HMENU)HMENU_BTN_LOAD, NULL, NULL);
	hBtnBack = CreateWindow(L"BUTTON", L"<", WS_VISIBLE | WS_CHILD, 190, 5, 50, 25, hControlBar, (HMENU)HMENU_BTN_BACK, NULL, NULL);
	hBtnHome = CreateWindow(L"BUTTON", L"^", WS_VISIBLE | WS_CHILD, 255, 5, 50, 25, hControlBar, (HMENU)HMENU_BTN_HOME, NULL, NULL);
	hBtnRecent = CreateWindow(L"BUTTON", L"o", WS_VISIBLE | WS_CHILD, 320, 5, 50, 25, hControlBar, (HMENU)HMENU_BTN_RECENT, NULL, NULL);
	hBtnSave = CreateWindow(L"BUTTON", L"Save Shot", WS_VISIBLE | WS_CHILD, 380, 32, 90, 25, hControlBar, (HMENU)HMENU_BTN_SAVE, NULL, NULL);
	hCkScreenDownload = CreateWindow(L"BUTTON", L"ScrLoader", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 380, 5, 90, 25, hControlBar, (HMENU)HMENU_CHECK_LOAD, NULL, NULL);
	Button_SetCheck(hCkScreenDownload, BST_CHECKED);
	hCkForceInvert = CreateWindow(L"BUTTON", L"Invert", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 475, 5, 90, 25, hControlBar, (HMENU)HMENU_CHECK_FORCE_INVERT, NULL, NULL);
	hCkScreenXY = CreateWindow(L"BUTTON", L"ScrXY", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 475, 31, 90, 25, hControlBar, (HMENU)HMENU_CHECK_SCREENXY, NULL, NULL);

	namedWindow("TitanBotCV");

	cv::setMouseCallback("TitanBotCV", onMouse, (void*)&point);

	hCV = FindWindowA(NULL, "TitanBotCV");

	LONG cvStyle = WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP;// GetWindowLong(hCV, GWL_STYLE);
	SetWindowLong(hCV, GWL_STYLE, cvStyle | WS_CHILD);
	SetParent(hCV, hMirror);

	ShowWindow(hWnd, SW_MAXIMIZE);
	
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// Step 3: The Message Loop
	while (GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return Msg.wParam;
}