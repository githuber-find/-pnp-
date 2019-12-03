#pragma once
#include <opencv2/opencv.hpp>
#include <omp.h>
using namespace cv;
using namespace std;

class armor_detect
{
public:
	void getArmour(vector<RotatedRect>& light, vector<RotatedRect>& target);
	void getLightContours(Mat& dst, Mat& img, vector<RotatedRect>& light);
	
};


class armor_color
{
public:
	void red_armor(Mat& image, Mat& dst, int threshValue);
	void blue_armor(Mat& image, Mat& dst, int threshValue);
};


class drawbox
{
public:
	void drawBoxBlue(RotatedRect box, Mat& img);
	void drawBoxGreen(RotatedRect box, Mat& img);
	void drawBoxRed(RotatedRect box, Mat& img);
};