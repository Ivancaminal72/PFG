#include "opencv2/opencv.hpp"
#include <cmath>
#include "opencv/cv.h"
#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <string>
#include <dirent.h>
#include <math.h>/* atan2 */
#include <boost/filesystem.hpp>

using namespace cv;
using namespace std;
namespace bf = boost::filesystem;

#define _USE_MATH_DEFINES

struct TrayPoint{
	int frame;
	Point pos;
	float angle;
};
vector<TrayPoint> rutas;

bool verifyDir(bf::path dir, bool doCreation){
	char op;
	cout<<endl<<dir<<endl;
	if(bf::exists(dir)){
			if(bf::is_directory(dir)){
				cout<<"Correct directory!"<<endl;
				return true;
			}else{
				cout<<"It is not a directory"<<endl;
				return false;
			}
	}else if(doCreation){
		cout<<"It doesn't exist"<<endl;
		cout<<"Do you want to create it? [y/n]"<<endl;
		cin>>op;
		if(op == 'y'){
			if(bf::create_directories(dir)){
				cout<<"Directory created!"<<endl;
				return true;
			}else{
				cout<<"Error creating directory"<<endl;
				return false;
			} 
		}else return false;
	}
	cout<<"It doesn't exist"<<endl;
	return false;
}

bool loadRoutes(bf::path loadRoutesPath){
	int num_routes=-1;
	int countFrames=0;
	cout<<endl<<"Loading Routes ";
	FileStorage fs_routes;
	cout<<loadRoutesPath.native()<<endl;
	if(fs_routes.open(loadRoutesPath.native(), FileStorage::READ)){
		FileNode rootNode = fs_routes.root(0);
		if(rootNode.empty()){
			cout<<"ERROR: Empty file"<<endl<<endl;
			return false;
		}else{
			TrayPoint ptray;
			Point actPoint;
			if(fs_routes["ultimaRuta"].isNone()){
				cout<<"ERROR: Node ultimaRuta doesn't exist"<<endl;
				return false;
			}
			fs_routes["ultimaRuta"]>>num_routes;
			FileNode actRutaNode;
			FileNodeIterator it_frame;
			for(int i=0; i<num_routes+1; i++){
				actRutaNode = fs_routes["Ruta"+to_string(i)];
				it_frame = actRutaNode.begin(); 
				for(;it_frame != actRutaNode.end(); it_frame++){
					countFrames+=1;
					ptray.frame=(int)(*it_frame)["frame"];
					FileNode nodePoint = (*it_frame)["Punto"];
					actPoint.x=(int) nodePoint[0];
					actPoint.y=(int) nodePoint[1];
					ptray.pos=actPoint;
					ptray.angle=(float)(*it_frame)["angle"];
					rutas.push_back(ptray);
				}
			}
		}
		fs_routes.release();
		cout<<"OK!"<<endl;
		return true;
	}else{
		cout<<"ERROR: Can't open for read"<<endl<<endl;
		return false;
	}
}

std::vector <float> matrixCoeficients(Point p1, Point p2){
	CvPoint2D32f vec;
	vec.x=p1.x-p2.x;
	vec.y=p1.y-p2.y;
	vector <float> coef(3);
	if(vec.x == 0){
		coef.at(0)=1;
		coef.at(1)=0;
		coef.at(2)=p2.x;
	}else{
		coef.at(0)=-(vec.y/vec.x);
		coef.at(1)=1;
		coef.at(2)=p2.y-(vec.y/vec.x*p2.x);
	}
	return coef;
}

bool findIntersection(Point& act, Point prev, Point pline1, Point pline2){
	vector <float> c1=matrixCoeficients(act, prev);
	vector <float> c2=matrixCoeficients(pline1, pline2);
	cv::Mat mA = (cv::Mat_<float>(2,2)<<c1.at(0),c1.at(1),c2.at(0),c2.at(1));
	cv::Mat mB = (cv::Mat_<float>(2,1)<<c1.at(2),c2.at(2));
	cv::Mat mInt;
	if(!cv::solve(mA,mB,mInt)) return false;
	act.x=round(mInt.at<float>(0,0));
	act.y=round(mInt.at<float>(0,1));
	return true;
}

bool getWallPosition(int& wpos, Point p, cv::Size imgSize){
	if(p.y==0){
		if(p.x<0 or p.x>imgSize.width-1) return false;
		else {wpos=p.x; return true;}
	}else if(p.x==imgSize.width-1){
		if(p.y<1 or p.y>imgSize.height-1) return false;
		else {wpos=p.x+p.y; return true;}
	}else if(p.y==imgSize.height-1){
		if(p.x<0 or p.x>imgSize.width-2) return false;
		else {wpos=imgSize.width*2+imgSize.height-p.x-3; return true;} 
	}else{
		if(p.y<1 or p.y>imgSize.height-2) return false;
		else {wpos=imgSize.width*2+imgSize.height*2-p.y-4; return true;}
	}
}

int main( int argc, char* argv[] ) {
	float horAngle = 20*M_PI/180;
	cv::Size imgSize;
	string save_file,routes_name;
	bf::path routes_path, save_dir = "./generatedVisualAreas/";

	//if (argc < 2 || argc > 5) {//wrong arguments
	if (argc < 1 || argc > 5) {
		cout<<"USAGE:"<<endl<< "./visualAreas rotues_path [horizontal_angle] [save_file] [dir_save/]"<<endl<<endl;
		cout<<"DEFAULT [horizontal_angle] = "<<horAngle<<endl;
		cout<<"DEFAULT [dir_save/] = "<<save_dir<<endl;
		cout<<"DEFAULT [save_file] = routes_name+horizontal_angle.yml"<<endl;
		return -1;
	}else{
		//routes_path = argv[1];
		routes_path = "/home/ivan/videos/sequences/pack3/routes.yml";
		if(!bf::exists(routes_path) or !routes_path.has_filename()) {cout<<"Wrong routes_path"<<endl; return -1;}
		if(argc > 2) horAngle = atoi(argv[2]);
		if(argc > 3) {
			save_file = argv[3];
			size_t found = save_file.find(".yml");
			if(found == string::npos){
				cout<<"Wrong save_file"<<endl;
				return -1;
			}
		}else{
			save_file=routes_path.filename().native()+to_string(horAngle)+".yml";
		}
		if(argc == 5) save_dir = argv[4];
	}

	//Verify and create correct directories
	if(!verifyDir(save_dir,true)) return -1;

	imgSize.width = 640; //640 8
	imgSize.height = 480; //480 6
	float persHeight = 208;//208 3

	/*loadRoutes(routes_path);
	vector<TrayPoint>::iterator it=rutas.begin();
	for(; it<rutas.end(); it++){
		//Point prespective correction
		cout<<(it->pos).x<<endl;
	}*/

	TrayPoint t;
	t.angle=315;
	t.pos.x=round(imgSize.width/2);
	t.pos.y=round(imgSize.height/2);

	//1-Prove t.pos is inside ImageSize

	//2-Generate positive function points
	int lim1=0,lim2=0,cor1=0,cor2=0,cor3=0,cor4=0;
	CvPoint2D32f ori;
	Point prev, act, plim1, plim2;
	Point pcor1(0,0), pcor2(imgSize.width-1,0); 
	Point pcor3(imgSize.width-1,imgSize.height-1), pcor4(0,imgSize.height-1);
	vector<Point> vpts, vpts2;
	Mat mR, mOri, mAct, mWalls, mFloor;
	act.x=-1;
	act.y=-1;
	ori.x=0;
	float rAng = t.angle*M_PI/180;
	vpts.push_back(t.pos);
	bool correctedAct=false;
	bool firstFunctionDone=false;  
	
	while(1){

		//--------------compute new function point--------//
		prev.x=act.x;
		prev.y=act.y;
		ori.y=tan(horAngle)*sqrt(pow(persHeight,2)+pow(ori.x,2));
		if(firstFunctionDone) ori.y*=-1.0;
		mR = (Mat_<float>(2,2)<<cos(rAng),-sin(rAng),sin(rAng),cos(rAng));
		mOri = (Mat_<float>(2,1)<<ori.x,ori.y);
		mAct = mR*mOri;
		//cout<<mAct<<endl;
		act.x=round(mAct.at<float>(0,0)+t.pos.x);
		act.y=round(-1*mAct.at<float>(1,0)+t.pos.y);

		//--------------verify out of bounds--------------//
		if(act.x<=0 or act.x>=imgSize.width-1 or act.y<=0 or act.y>=imgSize.height-1){//act point is in the limit or out of bounds
			correctedAct=false;
			while(act.x<0 or act.x>imgSize.width-1 or act.y<0 or act.y>imgSize.height-1 or !correctedAct){
				if(act.x<0){
					if(!findIntersection(act, prev, Point(0,0), Point(0,1))) {
						cout<<"Error intersection: "<<act<<prev<<" type1"<<endl; 
						return -1;
					}
					//act.x=0;
				}
				if(act.x>imgSize.width-1){
					if(!findIntersection(act, prev, Point(imgSize.width-1,0), Point(imgSize.width-1,1))) {
						cout<<"Error intersection: "<<act<<prev<<" type2"<<endl; 
						return -1;
					}
					//act.x=imgSize.width-1;
				}
				if(act.y<0){
					if(!findIntersection(act, prev, Point(0,0), Point(1,0))) {
						cout<<"Error intersection: "<<act<<prev<<" type3"<<endl; 
						return -1;
					}
					//act.y=0;
				}
				if(act.y>imgSize.height-1){
					if(!findIntersection(act, prev, Point(0,imgSize.height-1), Point(1,imgSize.height-1))) {
						cout<<"Error intersection: "<<act<<prev<<" type4"<<endl; 
						return -1;
					}
					//act.y=imgSize.height-1;
				}
				correctedAct=true;
			}
			ori.x=0;
			if(firstFunctionDone){
				if(prev.x != act.x or prev.y != act.x) {vpts2.push_back(act);}
				plim2=act;
				break;
			}else {
				if(prev.x != act.x or prev.y != act.x) {vpts.push_back(act);}
				plim1=act;
				firstFunctionDone=true;
			}
		}

		else {//act point is inside bounds
			ori.x+=1;
			if(firstFunctionDone){
				if(prev.x != act.x or prev.y != act.x) {vpts2.push_back(act);}
			}else{
				if(prev.x != act.x or prev.y != act.x) {vpts.push_back(act);}
			}
			
		}
	}

	//3-Get mWalls positions
	if(!getWallPosition(lim1,plim1,imgSize)) {cout<<"Error wrong Wall Position lim1 "<<plim1<<endl; return -1;}
	if(!getWallPosition(lim2,plim2,imgSize)) {cout<<"Error wrong Wall Position lim2 "<<plim2<<endl; return -1;}
	cor1 = 0;
	if(!getWallPosition(cor2,pcor2,imgSize)) {cout<<"Error wrong Wall Position cor2"<<endl; return -1;}
	if(!getWallPosition(cor3,pcor3,imgSize)) {cout<<"Error wrong Wall Position cor3"<<endl; return -1;}
	if(!getWallPosition(cor4,pcor4,imgSize)) {cout<<"Error wrong Wall Position cor4"<<endl; return -1;}

	/* Logging
	cout<<"Point lim1: "<<plim1<<" -> "<<lim1<<endl;
	cout<<"Point lim2: "<<plim2<<" -> "<<lim2<<endl;
	cout<<"Point cor1: "<<pcor1<<" -> "<<cor1<<endl;
	cout<<"Point cor2: "<<pcor2<<" -> "<<cor2<<endl;
	cout<<"Point cor3: "<<pcor3<<" -> "<<cor3<<endl;
	cout<<"Point cor4: "<<pcor4<<" -> "<<cor4<<endl;
	*/

	//4-Add corner area points
	if(lim2<lim1){
		if(lim1<cor2) vpts.push_back(pcor2);
		if(lim1<cor3) vpts.push_back(pcor3);
		if(lim1<cor4) vpts.push_back(pcor4);
		if(lim2!=cor1) vpts.push_back(pcor1);
		if(lim2>cor2) vpts.push_back(pcor2);
		if(lim2>cor3) vpts.push_back(pcor3);
		if(lim2>cor4) vpts.push_back(pcor4);
	}else{
		if(lim1<cor2 and lim2>cor2) vpts.push_back(pcor2);
		if(lim1<cor3 and lim2>cor3) vpts.push_back(pcor3);
		if(lim1<cor4 and lim2>cor4) vpts.push_back(pcor4);
	}

	vector<Point>::reverse_iterator rit=vpts2.rbegin();
	for(; rit!=vpts2.rend(); rit++){
		vpts.push_back(*rit);
	}

	//5-Draw mFloor and mWall
	int mWSize = imgSize.width*2+(imgSize.height-2)*2;
	mWalls = Mat::zeros(1, mWSize, CV_64F);
	if(lim2<lim1){ 

		for(int i = lim1; i<mWSize; i++){
			mWalls.at<double>(0,i) += 1;	
		}

		for(int i = 0; i<=lim2; i++){
			mWalls.at<double>(0,i) += 1;	
		}
	}else{
		for(int i = lim1; i<=lim2; i++){
			mWalls.at<double>(0,i) += 1;	
		}
	}

	mFloor = Mat::zeros(imgSize, CV_64F);
	Point pts[vpts.size()];
	vector<Point>::iterator it=vpts.begin();
	int posPts=0;
	for(; it!=vpts.end(); it++){
		pts[posPts]=(*it);
		posPts+=1;
	}
	cv::fillConvexPoly(mFloor, pts, vpts.size(),cv::Scalar(1));
	imshow("suuh", mFloor);
	waitKey(0);
	//6-Normalize polygons and line 

	//7-Add to de total

	vector<Point>::iterator ite = vpts.begin();
	for(; ite<vpts.end(); ite++){
		cout<<*ite<<endl;
	}
/*
	cv::Mat r1A = (cv::Mat_<float>(2,2)<<0.5,1,1,0);
	cv::Mat B = (cv::Mat_<float>(2,1)<<0.5-2,0);
	cv::Mat pp;

	bool dd = cv::solve(r1A,B,pp);

	cout<<dd<<"Result "<<pp<<endl;*/
}

