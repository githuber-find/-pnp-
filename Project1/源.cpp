#include"PNPSolver.h"
#include"armor_detect.h"

bool pause = false;
bool flag = false;

int g_color = 0;
int g_threshValue = 150;//通道相减后的阈值

vector<RotatedRect> light; //定以旋转矩形的向量，用于存储发现的目标区域
vector<RotatedRect> target;//存储装甲板

Mat g_image, g_image1, g_frame, g_dst;

void on_Color(int, void*);

int main()
{
	int thresh = 170;//灰度图阈值
	int exposure = -7;//曝光度

	Mat gray;
	Mat dst, gray_and_bgr;
	drawbox box;
	armor_detect get_armour;
	PNPSolver pnp;
	Point2f point2D[4];

	VideoCapture capture(1);

	pnp.read_xml("test.xml");

	namedWindow("颜色通道相减");
	namedWindow("灰度图二值化");

	createTrackbar("颜色", "颜色通道相减", &g_color, 1, on_Color);//选择识别颜色
	createTrackbar("颜色threshold", "颜色通道相减", &g_threshValue, 255, on_Color);//阈值调节
	createTrackbar("灰度threshold", "灰度图二值化", &thresh, 255, 0);

	capture.set(CAP_PROP_EXPOSURE, exposure);

	capture >> g_frame;

	while (true)
	{
		if (!pause)
		{
			capture >> g_frame;

			if (g_frame.empty())
			{
				break;
			}
		}

		g_image = g_frame.clone();
#ifdef SHRINK
		resize(g_image, g_image, Size(0, 0), 0.4, 0.4);
#endif // SHRINK
		g_image1 = g_image.clone();
		g_dst.create(g_image1.rows, g_image1.cols, CV_8UC1);

		Mat elementDilate = getStructuringElement(MORPH_ELLIPSE, Size(15, 15));
		Mat elementErode = getStructuringElement(MORPH_ELLIPSE, Size(15, 15));
		Mat elementDilate2 = getStructuringElement(MORPH_ELLIPSE, Size(7, 7));
		Mat elementErode2 = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));

		cvtColor(g_image1, gray, COLOR_BGR2GRAY);//得到灰度图
		on_Color(g_color, 0);//颜色通道相减		B-R后，在灰度图上只识别B-R后的二值图上的白色区域
		on_Color(g_threshValue, 0);//二值化

		//对颜色通道相减后的二值图进行去噪处理
		medianBlur(g_dst, g_dst, 5);
		dilate(g_dst, g_dst, elementDilate);
		erode(g_dst, g_dst, elementErode);

		//将颜色通道相减区分红蓝场，再与灰度图进行且运算
		dst.create(g_image1.rows, g_image1.cols, CV_8UC1);
		dst = gray & g_dst;

		//gray_and_bgr = dst.clone();//复合图像

		//对复合图像进行二值化并去噪
		threshold(dst, dst, thresh, 255, 0);

		dilate(dst, dst, elementDilate2);
		erode(dst, dst, elementErode2);

		//找灯条轮廓
		get_armour.getLightContours(dst, g_image1, light);
		if (!light.empty())
		{
			for (int i = 0; i < light.size(); i++)
			{
				box.drawBoxGreen(light[i], g_image);
			}
		}

		char c = waitKey(1);
		//进行灯条配对，提取装甲板
		get_armour.getArmour(light, target);
		if (!target.empty())
		{
			if (g_color == 1)
			{
				for (int i = 0; i < target.size(); i++)
				{
					box.drawBoxBlue(target[i], g_image);
				}
			}
			else
			{
				for (int i = 0; i < target.size(); i++)
				{
					box.drawBoxRed(target[i], g_image);
					target[i].points(point2D);
				}
			}
			
			//输入目标的象素坐标
			pnp.Points2D.push_back(point2D[0]);
			pnp.Points2D.push_back(point2D[1]);
			pnp.Points2D.push_back(point2D[2]);
			pnp.Points2D.push_back(point2D[3]);

			if (c == 'g')
			{
				pnp.get_distance(pnp.Points2D);
			}
					
		}

		if (c == 27)
		{
			break;
		}
		if (c == 'p')
		{
			pause = !pause;
		}

		imshow("颜色通道相减", g_dst);
		imshow("灰度图二值化", dst);
		imshow("原图像", g_image);
		//imshow("复合图像", gray_and_bgr);

		light.clear();
		target.clear();
		pnp.Points2D.clear();
	}

//Mat image = imread("1.jpg");
//pnp.Points2D.push_back(Point2f(930, 159));
//pnp.Points2D.push_back(Point2f(1262, 171));
//pnp.Points2D.push_back(Point2f(1251, 377));
//pnp.Points2D.push_back(Point2f(916, 359));
//
//pnp.get_distance(pnp.Points2D);
//
//cin.get();
}

void on_Color(int, void*)//过滤己方颜色
{
	armor_color color;
	if (g_color == 0)
	{
		//B-R后，蓝色灯条是黑色，B-R的黑色部分让灰度图
		color.blue_armor(g_image1, g_dst, g_threshValue);
	}
	else
	{
		color.red_armor(g_image1, g_dst, g_threshValue);
	}
}