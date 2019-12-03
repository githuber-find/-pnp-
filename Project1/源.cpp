#include"PNPSolver.h"
#include"armor_detect.h"

bool pause = false;
bool flag = false;

int g_color = 0;
int g_threshValue = 150;//ͨ����������ֵ

vector<RotatedRect> light; //������ת���ε����������ڴ洢���ֵ�Ŀ������
vector<RotatedRect> target;//�洢װ�װ�

Mat g_image, g_image1, g_frame, g_dst;

void on_Color(int, void*);

int main()
{
	int thresh = 170;//�Ҷ�ͼ��ֵ
	int exposure = -7;//�ع��

	Mat gray;
	Mat dst, gray_and_bgr;
	drawbox box;
	armor_detect get_armour;
	PNPSolver pnp;
	Point2f point2D[4];

	VideoCapture capture(1);

	pnp.read_xml("test.xml");

	namedWindow("��ɫͨ�����");
	namedWindow("�Ҷ�ͼ��ֵ��");

	createTrackbar("��ɫ", "��ɫͨ�����", &g_color, 1, on_Color);//ѡ��ʶ����ɫ
	createTrackbar("��ɫthreshold", "��ɫͨ�����", &g_threshValue, 255, on_Color);//��ֵ����
	createTrackbar("�Ҷ�threshold", "�Ҷ�ͼ��ֵ��", &thresh, 255, 0);

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

		cvtColor(g_image1, gray, COLOR_BGR2GRAY);//�õ��Ҷ�ͼ
		on_Color(g_color, 0);//��ɫͨ�����		B-R���ڻҶ�ͼ��ֻʶ��B-R��Ķ�ֵͼ�ϵİ�ɫ����
		on_Color(g_threshValue, 0);//��ֵ��

		//����ɫͨ�������Ķ�ֵͼ����ȥ�봦��
		medianBlur(g_dst, g_dst, 5);
		dilate(g_dst, g_dst, elementDilate);
		erode(g_dst, g_dst, elementErode);

		//����ɫͨ��������ֺ�����������Ҷ�ͼ����������
		dst.create(g_image1.rows, g_image1.cols, CV_8UC1);
		dst = gray & g_dst;

		//gray_and_bgr = dst.clone();//����ͼ��

		//�Ը���ͼ����ж�ֵ����ȥ��
		threshold(dst, dst, thresh, 255, 0);

		dilate(dst, dst, elementDilate2);
		erode(dst, dst, elementErode2);

		//�ҵ�������
		get_armour.getLightContours(dst, g_image1, light);
		if (!light.empty())
		{
			for (int i = 0; i < light.size(); i++)
			{
				box.drawBoxGreen(light[i], g_image);
			}
		}

		char c = waitKey(1);
		//���е�����ԣ���ȡװ�װ�
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
			
			//����Ŀ�����������
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

		imshow("��ɫͨ�����", g_dst);
		imshow("�Ҷ�ͼ��ֵ��", dst);
		imshow("ԭͼ��", g_image);
		//imshow("����ͼ��", gray_and_bgr);

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

void on_Color(int, void*)//���˼�����ɫ
{
	armor_color color;
	if (g_color == 0)
	{
		//B-R����ɫ�����Ǻ�ɫ��B-R�ĺ�ɫ�����ûҶ�ͼ
		color.blue_armor(g_image1, g_dst, g_threshValue);
	}
	else
	{
		color.red_armor(g_image1, g_dst, g_threshValue);
	}
}