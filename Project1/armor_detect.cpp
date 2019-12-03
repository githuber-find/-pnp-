#include "armor_detect.h"
#define ARMOUR_ANGLE//装甲板角度要求
#define LIGHT_ANGLE//灯条角度要求

void drawbox::drawBoxBlue(RotatedRect box, Mat& img)
{
	Point2f pt[4];
	int i;
	for (i = 0; i < 4; i++)
	{
		pt[i].x = 0;
		pt[i].y = 0;
	}
	box.points(pt); //计算二维盒子顶点 
	line(img, pt[0], pt[1], CV_RGB(0, 0, 255), 2, 8, 0);
	line(img, pt[1], pt[2], CV_RGB(0, 0, 255), 2, 8, 0);
	line(img, pt[2], pt[3], CV_RGB(0, 0, 255), 2, 8, 0);
	line(img, pt[3], pt[0], CV_RGB(0, 0, 255), 2, 8, 0);
}

void drawbox::drawBoxGreen(RotatedRect box, Mat& img)
{
	Point2f pt[4];
	int i;
	for (i = 0; i < 4; i++)
	{
		pt[i].x = 0;
		pt[i].y = 0;
	}
	box.points(pt); //计算二维盒子顶点 
	line(img, pt[0], pt[1], CV_RGB(0, 255, 0), 2, 8, 0);
	line(img, pt[1], pt[2], CV_RGB(0, 255, 0), 2, 8, 0);
	line(img, pt[2], pt[3], CV_RGB(0, 255, 0), 2, 8, 0);
	line(img, pt[3], pt[0], CV_RGB(0, 255, 0), 2, 8, 0);
}

void drawbox::drawBoxRed(RotatedRect box, Mat& img)
{
	Point2f pt[4];
	int i;
	for (i = 0; i < 4; i++)
	{
		pt[i].x = 0;
		pt[i].y = 0;
	}
	box.points(pt); //计算二维盒子顶点 
	line(img, pt[0], pt[1], CV_RGB(255, 0, 0), 2, 8, 0);
	line(img, pt[1], pt[2], CV_RGB(255, 0, 0), 2, 8, 0);
	line(img, pt[2], pt[3], CV_RGB(255, 0, 0), 2, 8, 0);
	line(img, pt[3], pt[0], CV_RGB(255, 0, 0), 2, 8, 0);
}


void armor_color::blue_armor(Mat& image, Mat& dst, int threshValue)
{
	for (int i = 0; i < image.rows; i++)
	{
		uchar* data_bgr = image.ptr<uchar>(i);

		for (int j = 0; j < image.cols; j++)
		{
			//蓝色减去红色
			dst.ptr<uchar>(i)[j] = saturate_cast<uchar>(*data_bgr - *(data_bgr + 2));

			//B-R后，蓝色灯条是黑色，B-R的黑色部分让灰度图

			//阈值化
			if (dst.ptr<uchar>(i)[j] < threshValue)
			{
				dst.ptr<uchar>(i)[j] = 0;
			}
			else
			{
				dst.ptr<uchar>(i)[j] = 255;
			}
			
			data_bgr = data_bgr + 3;
		}
	}
}

void armor_color::red_armor(Mat& image, Mat& dst, int threshValue)
{
	for (int i = 0; i < image.rows; i++)
	{
		uchar* data_bgr = image.ptr<uchar>(i);
		data_bgr = data_bgr + 2;

		for (int j = 0; j < image.cols; j++)
		{
			dst.ptr<uchar>(i)[j] = saturate_cast<uchar>(*data_bgr - *(data_bgr - 2));

			//阈值化
			if (dst.ptr<uchar>(i)[j] < threshValue)
			{
				dst.ptr<uchar>(i)[j] = 0;
			}
			else
			{
				dst.ptr<uchar>(i)[j] = 255;
			}
			
			data_bgr = data_bgr + 3;
		}
	}
}

void armor_detect::getLightContours(Mat& dst, Mat& img, vector<RotatedRect>& light)
{
	vector<vector<Point>> lightContours;
	findContours(dst, lightContours, RETR_EXTERNAL, CHAIN_APPROX_NONE);//寻找所有外轮廓

	for (int i = 0; i < lightContours.size(); i++)
	{
		if (lightContours[i].size() > 4)//判断每个轮廓的象素个数
		{
			RotatedRect lightRect;   //定义旋转矩形

			lightRect = fitEllipse(lightContours[i]);//用椭圆拟合轮廓得到旋转矩形

#ifdef LIGHT_ANGLE
			//灯条的角度
			double light_angle = 0;
			light_angle = lightRect.angle;

			//printf("light_angle = %f\n", light_angle);
			if (light_angle > 45 && light_angle < 135)
			{
				continue;
			}
#endif // LIGHT_ANGLE

			//要保证灯条要足够细
			if (lightRect.size.height / lightRect.size.width > 2 || lightRect.size.width / lightRect.size.height > 2)
			{
				light.push_back(lightRect);//保存目标矩形
			}
		}
	}
}

void armor_detect::getArmour(vector<RotatedRect>& light, vector<RotatedRect>& target)
{
	if (light.size() < 2)
	{
		return;
	}

	float dangle;

	for (int i = 0; i < light.size() - 1; i++)
	{
		for (int j = i + 1; j < light.size(); j++)//将每个矩形与除自己以外的所有矩形进行配对
		{
			//判断灯条是否在同一直线上
			//if (abs(light[i].center.y - light[j].center.y) > 50)
			//{
			//	continue;
			//}

			//if (abs(light[i].center.x - light[j].center.x) < 10)
			//{
			//	continue;
			//}

			//判断两个灯条是否平行
			dangle = abs(light[i].angle - light[j].angle);//得到角度差
			//printf("%f\n", dangle);
			if (dangle < 30 || dangle > 150)
			{


				RotatedRect armour;
				float armourLong, armourWidth;
				float light1_height, light2_height;

				if (light[i].size.width > light[i].size.height)
				{
					light1_height = light[i].size.width;
				}
				else
				{
					light1_height = light[i].size.height;
				}

				if (light[j].size.width > light[j].size.height)
				{
					light2_height = light[j].size.width;
				}
				else
				{
					light2_height = light[j].size.height;
				}

				//判断两个灯条的宽的比值相差是否很大
				if (light1_height / light2_height > 2 || light2_height / light1_height > 2)
				{
					continue;
				}

				//短边
				armourWidth = (light1_height + light2_height) / 2;
				//长边
				armourLong = sqrt((light[i].center.x - light[j].center.x)*(light[i].center.x - light[j].center.x) +
					(light[i].center.y - light[j].center.y)*(light[i].center.y - light[j].center.y));

				//判断两灯条中心点的距离与装甲板的长是否相近
				if (armourLong / (light[i].center.x - light[j].center.x) > 1.5)
				{
					continue;
				}

				//矫正装甲板的角度
				float angle1 = 0, angle2 = 0;

				angle1 = light[i].angle;
				angle2 = light[j].angle;

				armour.angle = (angle1 + angle2) / 2 ;

				//装甲板的角度应该与灯条角度垂直
				if (dangle < 90 && angle1 < 90)//两灯条同时偏向右侧
				{
					armour.angle = armour.angle + 90;
				}

				if (dangle < 90 && angle1 > 90)//两灯条同时偏向左侧
				{
					armour.angle = armour.angle - 90;
				}
#ifdef ARMOUR_ANGLE
				//对 角度、长/宽进行限制
				if (armour.angle < 60 || armour.angle > 120)
				{
					continue;
				}
#endif // ARMOUR_ANGLE

				if (armourLong / armourWidth < 4 && armourLong / armourWidth > 0)
				{
					armour.size.width = armourWidth;
					armour.size.height = armourLong;

					armour.center.x = (light[i].center.x + light[j].center.x) / 2;
					armour.center.y = (light[i].center.y + light[j].center.y) / 2;

					//printf("armour_angle = %f\n", armour.angle);
					target.push_back(armour);//将找出的装甲的旋转矩形保存到target
				}
			}
		}
	}
}