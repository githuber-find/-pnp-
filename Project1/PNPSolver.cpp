#include "PNPSolver.h"

// 本类用于快速解决PNP问题，顺带解决空间绕轴旋转以及图像系、相机系、世界系三系坐标投影问题
// 调用顺序：
// 1.初始化本类
// 2.调用SetCameraMatrix(),SetDistortionCoefficients()设置好相机内参数与镜头畸变参数
// 3.向Points3D，Points2D中添加一一对应的特征点对
// 4.调用Solve()方法运行计算
// 5.从RoteM, TransM, W2CTheta等属性中提出结果
//
// 原理参见：http://www.cnblogs.com/singlex/category/911880.html
// Author：VShawn
// Ver:2016.11.26.0
void convert1(vector <Point> point2i, vector <Point2f>& point2f);
void convert2(vector <Point2f> point2i, vector <Point>& point2f);
PNPSolver::PNPSolver()
{
	//初始化输出矩阵
	vector<double> rv(3), tv(3);
	Mat rvec(rv), tvec(tv);
}
PNPSolver::PNPSolver(Mat CameraMatrix, Mat DistortionCoefficients)
{
	//初始化输出矩阵
	vector<double> rv(3), tv(3);
	Mat rvec(rv), tvec(tv);
	camera_matrix = CameraMatrix;
	distortion_coefficients = DistortionCoefficients;
}

PNPSolver::~PNPSolver()
{
}

int PNPSolver::Solve(int method)
{
	//数据校验
	if (camera_matrix.cols == 0 || distortion_coefficients.cols == 0)
	{
		printf("ErrCode:-1,相机内参数或畸变参数未设置！\r\n");
		return -1;
	}

	if (Points3D.size() != Points2D.size())
	{
		cout << "Points3D.size() = " << Points3D.size() << "," 
			<< "Points2D.size()" << Points2D.size() << endl;

		printf("ErrCode:-2,1111r\n");
		return -2;
	}
	if (method == CV_P3P || method == CV_ITERATIVE)
	{
		if (Points3D.size() != 4)
		{
			printf("ErrCode:-2,2222r\n");
			return -2;
		}
	}
	else
	{
		if (Points3D.size() < 4)
		{
			printf("ErrCode:-2,3333r\n");
			return -2;
		}
	}

	////TODO::检验是否是共面的四点
	//if ((method == METHOD::CV_ITERATIVE || method == METHOD::CV_EPNP) && Points2D.size() == 4)
	//{
	//	//通过向量两两叉乘获得法向量，看法向量是否平行
	//}






	/*******************解决PNP问题*********************/
	//有三种方法求解
	solvePnP(Points3D, Points2D, camera_matrix, distortion_coefficients, rvec, tvec, true, method);
	//实测迭代法似乎只能用共面特征点求位置
	//solvePnP(Points3D, Points2D, camera_matrix, distortion_coefficients, rvec, tvec, false, CV_ITERATIVE);	//实测迭代法似乎只能用共面特征点求位置
	//solvePnP(Points3D, Points2D, camera_matrix, distortion_coefficients, rvec, tvec, false, CV_P3P);		//Gao的方法可以使用任意四个特征点
	//solvePnP(Points3D, Points2D, camera_matrix, distortion_coefficients, rvec, tvec, false, CV_EPNP);


	/*******************提取旋转矩阵*********************/
	Rodrigues(rvec, RoteM);  //罗技,把旋转向量变成旋转矩阵
	double r11 = RoteM.ptr<double>(0)[0];
	double r12 = RoteM.ptr<double>(0)[1];
	double r13 = RoteM.ptr<double>(0)[2];
	double r21 = RoteM.ptr<double>(1)[0];
	double r22 = RoteM.ptr<double>(1)[1];
	double r23 = RoteM.ptr<double>(1)[2];
	double r31 = RoteM.ptr<double>(2)[0];
	double r32 = RoteM.ptr<double>(2)[1];
	double r33 = RoteM.ptr<double>(2)[2];
	TransM = tvec;

	//计算出相机坐标系的三轴旋转欧拉角，旋转后可以转出世界坐标系。
	//旋转顺序为z、y、x
	double thetaz = atan2(r21, r11) / CV_PI * 180;
	double thetay = atan2(-1 * r31, sqrt(r32*r32 + r33*r33)) / CV_PI * 180;
	double thetax = atan2(r32, r33) / CV_PI * 180;

	//相机系到世界系的三轴旋转欧拉角，相机坐标系照此旋转后可以与世界坐标系完全平行。
	//旋转顺序为z、y、x
	Theta_C2W.z = thetaz;
	Theta_C2W.y = thetay;
	Theta_C2W.x = thetax;

	//计算出世界系到相机系的三轴旋转欧拉角，世界系照此旋转后可以转出相机坐标系。
	//旋转顺序为x、y、z
	Theta_W2C.x = -1 * thetax;
	Theta_W2C.y = -1 * thetay;
	Theta_W2C.z = -1 * thetaz;


	/*************************************此处计算出相机坐标系原点Oc在世界坐标系中的位置**********************************************/

	/***********************************************************************************/
	/* 当原始坐标系经过旋转z、y、x三次旋转后，与世界坐标系平行，向量OcOw会跟着旋转 */
	/* 而我们想知道的是两个坐标系完全平行时，OcOw的值 */
	/* 因此，原始坐标系每次旋转完成后，对向量OcOw进行一次反相旋转，最终可以得到两个坐标系完全平行时的OcOw */
	/* 该向量乘以-1就是世界坐标系下相机的坐标 */
	/***********************************************************************************/

	//提出平移矩阵，表示从相机坐标系原点，跟着向量(x,y,z)走，就到了世界坐标系原点
	double tx = tvec.ptr<double>(0)[0];
	double ty = tvec.ptr<double>(0)[1];
	double tz = tvec.ptr<double>(0)[2];

	//x y z 为唯一向量在相机原始坐标系下的向量值
	//也就是向量OcOw在相机坐标系下的值
	double x = tx, y = ty, z = tz;
	Position_OwInC.x = x;
	Position_OwInC.y = y;
	Position_OwInC.z = z;
	//进行三次反向旋转
	CodeRotateByZ(x, y, -1 * thetaz, x, y);
	CodeRotateByY(x, z, -1 * thetay, x, z);
	CodeRotateByX(y, z, -1 * thetax, y, z);


	//获得相机在世界坐标系下的位置坐标
	//即向量OcOw在世界坐标系下的值
	Position_OcInW.x = x*-1;
	Position_OcInW.y = y*-1;
	Position_OcInW.z = z*-1;

	return 0;
}


//根据计算出的结果将世界坐标重投影到图像，返回像素坐标点集
//输入为世界坐标系的点坐标集合
//输出为点投影到图像上的图像坐标集合
vector<Point2f> PNPSolver::WordFrame2ImageFrame(vector<Point3f> WorldPoints)
{
	vector<Point2f> projectedPoints;
	projectPoints(WorldPoints, rvec, tvec, camera_matrix, distortion_coefficients, projectedPoints);
	return projectedPoints;
}



//根据输入的参数将图像坐标转换到相机坐标中
//使用前需要先用Solve()解出相机位姿
//输入为图像上的点坐标
//double F为镜头焦距
//输出为点在焦距=F时的相机坐标系坐标
Point3f PNPSolver::ImageFrame2CameraFrame(Point2f p, double F)
{
	double fx;
	double fy;
	double u0;
	double v0;

	fx = camera_matrix.ptr<double>(0)[0];
	u0 = camera_matrix.ptr<double>(0)[2];
	fy = camera_matrix.ptr<double>(1)[1];
	v0 = camera_matrix.ptr<double>(1)[2];
	double zc = F;
	double xc = (p.x - u0)*F / fx;
	double yc = (p.y - v0)*F / fy;
	return Point3f(xc, yc, zc);
}

void PNPSolver::read_xml(string xml, bool left_eye)
{
	const char *IntrinsicsPath = xml.c_str();
	FileStorage readfs;

	if (!readfs.open(IntrinsicsPath, FileStorage::READ))
	{
		cout << "Cannot open the file" << endl;
		exit(0);
	}
	if (left_eye)
	{
		readfs["cameraMatrixL"] >> camera_matrix;
		readfs["distCoeffL"] >> distortion_coefficients;
	}
	else
	{
		readfs["cameraMatrixR"] >> camera_matrix;
		readfs["distCoeffR"] >> distortion_coefficients;
	}

	//  distortion_coefficients
	//cout << camera_matrix << endl << distortion_coefficients << endl;
	readfs.release();
}

void PNPSolver::read_xml(string xml)
{
	const char *IntrinsicsPath = xml.c_str();
	FileStorage readfs;

	if (!readfs.open(IntrinsicsPath, FileStorage::READ))
	{
		cout << "Cannot open the file" << endl;
		exit(0);
	}
	readfs["cameraMatrix"] >> camera_matrix;
	readfs["distCoeff"] >> distortion_coefficients;
	//  distortion_coefficients
	//cout << camera_matrix << endl << distortion_coefficients << endl;
	readfs.release();
}

Point3f PNPSolver::get_distance(vector <Point2f> points)
{
	Point3f point;
	vector<Point >point2i;
	Points2D = points;
	convert2(points, point2i);
	convert1(point2i, Points2D);

	//小装甲板的3D坐标
	Points3D.push_back(Point3f(0, 0, 0));
	Points3D.push_back(Point3f(135, 0, 0));
	Points3D.push_back(Point3f(135, 55, 0));
	Points3D.push_back(Point3f(0, 55, 0));
	
	//标定板的3D坐标
	//Points3D.push_back(Point3f(0, 0, 0));
	//Points3D.push_back(Point3f(200, 0, 0));
	//Points3D.push_back(Point3f(200, 125, 0));
	//Points3D.push_back(Point3f(0, 125, 0));

	cout << "特征点的相机坐标" << endl;
	for (size_t i = 0; i < 4; i++)
	{
		cout << Points2D[i] << endl;
	}
	cout << endl;

	cout << "特征点的世界坐标" << endl;
	for (size_t i = 0; i < 4; i++)
	{
		cout << Points3D[i] << endl;
	}
	cout << endl;

	Solve(CV_P3P);//CV_P3P  CV_EPNP  CV_ITERATIVE
	Points3D.clear();
	cout << endl;

	cout << "相机的世界坐标" << endl;
	cout << "point = " << Position_OcInW << endl;
	//cout << "point2 = " << Position_OwInC << endl;
	//out2file(pnpsolver.Position_OwInC.z);

	return Position_OcInW;
}

void convert1(vector <Point> point2i, vector <Point2f>& point2f)
{
	Mat src = Mat(point2i);
	Mat dst = Mat(point2f);
	Mat tmp;
	src.convertTo(tmp, dst.type());
	point2f = (vector<Point2f>)tmp;
}

void convert2(vector <Point2f> point2i, vector <Point>& point2f)
{
	Mat src = Mat(point2i);
	Mat dst = Mat(point2f);
	Mat tmp;
	src.convertTo(tmp, dst.type());
	point2f = (vector<Point>)tmp;
}


