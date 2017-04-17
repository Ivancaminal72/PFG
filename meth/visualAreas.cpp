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

std::vector <double> matrixCoeficients(Point2d p1, Point2d p2){
	Point2d vec;
	vec.x=p1.x-p2.x;
	vec.y=p1.y-p2.y;
	vector <double> coef(3);
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

bool findIntersection(Point2d& pInt, Point2d p1r1, Point2d p2r1, Point2d p1r2, Point2d p2r2){
	vector <double> c1=matrixCoeficients(p1r1, p2r1);
	vector <double> c2=matrixCoeficients(p1r2, p2r2);
	cv::Mat mA = (cv::Mat_<double>(2,2)<<c1.at(0),c1.at(1),c2.at(0),c2.at(1));
	cv::Mat mB = (cv::Mat_<double>(2,1)<<c1.at(2),c2.at(2));
	cv::Mat mInt;
	if(!cv::solve(mA,mB,mInt)) return false;
	pInt.x=mInt.at<double>(0,0);
	pInt.y=mInt.at<double>(0,1);
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

void replaceWeights(Mat& m, double wNew, double wOld){
	for(int i = 0; i < m.rows; i++){
		for(int j = 0; j < m.cols; j++){
			if(m.at<double>(i,j) == wOld) m.at<double>(i,j)=wNew;
		}
	}
}

int main( int argc, char* argv[] ) {
	double hAngle = 20*M_PI/180;//mean horizontal visualCamp
	string save_file,routes_name;
	bf::path routes_path, save_dir = "./generatedVisualAreas/";

	//if (argc < 2 || argc > 5) {//wrong arguments
	if (argc < 1 || argc > 5) {
		cout<<"USAGE:"<<endl<< "./visualAreas rotues_path [horizontal_angle] [save_file] [dir_save/]"<<endl<<endl;
		cout<<"DEFAULT [horizontal_angle] = "<<hAngle<<endl;
		cout<<"DEFAULT [dir_save/] = "<<save_dir<<endl;
		cout<<"DEFAULT [save_file] = routes_name+horizontal_angle.yml"<<endl;
		return -1;
	}else{
		//routes_path = argv[1];
		routes_path = "/home/ivan/videos/sequences/pack3/routes.yml";
		if(!bf::exists(routes_path) or !routes_path.has_filename()) {cout<<"Wrong routes_path"<<endl; return -1;}
		if(argc > 2) hAngle = atof(argv[2])*M_PI/180;
		if(argc > 3) {
			save_file = argv[3];
			size_t found = save_file.find(".yml");
			if(found == string::npos){
				cout<<"Wrong save_file"<<endl;
				return -1;
			}
		}else{
			save_file=routes_path.filename().native()+to_string(hAngle)+".yml";
		}
		if(argc == 5) save_dir = argv[4];
	}

	//Verify and create correct directories
	if(!verifyDir(save_dir,true)) return -1;

	cv::Size imgSize;
	imgSize.width = 8;//640; //640 8
	imgSize.height = 6;//480; //480 6
	double persHeight = 3;//208;//1.6*130 3
	double wallsHeight = 6; //390; ~3*130
	int mWLength = imgSize.width*2+(imgSize.height-2)*2;

	TrayPoint t;
	t.angle=315;
	t.pos.x=round(imgSize.width/2);
	t.pos.y=round(imgSize.height/2);

	int lim1,lim2,cor1,cor2,cor3,cor4,markPxFloor,markPxWalls;
	double tAngle, dist, wFloor, wWalls, wCeiling, aFloor, aWalls, aCeiling;
	Point2d tpos, ori, prev, act, plim1, plim2, plim3, pcor1, pcor2, pcor3, pcor4; //2 dimensions <double> precission
	vector<Point> vpts, vpts2;
	vector<Point2d> vplim3;
	Mat mR, mOri, mAct, mWalls, mFloor, mCeiling;
	pcor1.x=0; pcor1.y=0;
	pcor2.x=imgSize.width-1; pcor2.y=0;
	pcor3.x=imgSize.width-1; pcor3.y=imgSize.height-1;
	pcor4.x=0; pcor4.y=imgSize.height-1;
	act.x=-1; act.y=-1;
	ori.x=0;
	tAngle = t.angle*M_PI/180;
	tpos = t.pos;
	vpts.push_back(t.pos);
	bool correctedAct=false;
	bool firstFunctionDone=false;

	/*loadRoutes(routes_path);
	vector<TrayPoint>::iterator it=rutas.begin();
	for(; it<rutas.end(); it++){
		//Point prespective correction
		cout<<(it->pos).x<<endl;
	}*/

	//0-Correction tpos sensor projection

	//1-Prove tpos is inside the image bounds

	//2-Generate function points
	while(1){

		//--------------compute new function point--------//
		prev.x=act.x;
		prev.y=act.y;
		ori.y=tan(hAngle)*sqrt(pow(persHeight,2)+pow(ori.x,2));
		if(firstFunctionDone) ori.y*=-1.0;
		mR = (Mat_<double>(2,2)<<cos(tAngle),-sin(tAngle),sin(tAngle),cos(tAngle));
		mOri = (Mat_<double>(2,1)<<ori.x,ori.y);
		mAct = mR*mOri;
		//cout<<mAct<<endl; //Logging
		act.x=mAct.at<double>(0,0)+tpos.x;
		act.y=-1*mAct.at<double>(1,0)+tpos.y;

		//--------------verify out of bounds--------------//
		if(act.y<=0 or act.x>=imgSize.width-1 or act.y>=imgSize.height-1 or act.x<=0){//act point is in the limit or out of bounds
			correctedAct=false;
			while(act.y<0 or act.x>imgSize.width-1 or act.y>imgSize.height-1 or act.x<0 or !correctedAct){
				if(act.y<0){
					if(!findIntersection(act, Point2d(act), prev, pcor1, pcor2)) {cout<<"Error intersection: "<<act<<prev<<" type1"<<endl; return -1;}
					//act.y=0;
				}
				if(act.x>imgSize.width-1){
					if(!findIntersection(act, Point2d(act), prev, pcor2, pcor3)) {cout<<"Error intersection: "<<act<<prev<<" type2"<<endl; return -1;}
					//act.x=imgSize.width-1;
				}
				if(act.y>imgSize.height-1){
					if(!findIntersection(act, Point2d(act), prev, pcor3, pcor4)) {cout<<"Error intersection: "<<act<<prev<<" type4"<<endl; return -1;}
					//act.y=imgSize.height-1;
				}
				if(act.x<0){
					if(!findIntersection(act, Point2d(act), prev, pcor4, pcor1)) {cout<<"Error intersection: "<<act<<prev<<" type3"<<endl; return -1;}
					//act.x=0;
				}
				correctedAct=true;
			}
			ori.x=0;
			if(firstFunctionDone){
				if(prev.x != act.x or prev.y != act.x) {vpts2.push_back(Point(round(act.x),round(act.y)));}
				plim2=act;
				break;
			}else {
				if(prev.x != act.x or prev.y != act.x) {vpts.push_back(Point(round(act.x),round(act.y)));}
				plim1=act;
				firstFunctionDone=true;
			}
		}

		else {//act point is inside bounds
			ori.x+=1;
			if(firstFunctionDone){
				if(prev.x != act.x or prev.y != act.x) {vpts2.push_back(Point(round(act.x),round(act.y)));}
			}else{
				if(prev.x != act.x or prev.y != act.x) {vpts.push_back(Point(round(act.x),round(act.y)));}
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

	/* //Logging
	cout<<"Point lim1: "<<plim1<<" -> "<<lim1<<endl;
	cout<<"Point lim2: "<<plim2<<" -> "<<lim2<<endl;
	cout<<"Point cor1: "<<pcor1<<" -> "<<cor1<<endl;
	cout<<"Point cor2: "<<pcor2<<" -> "<<cor2<<endl;
	cout<<"Point cor3: "<<pcor3<<" -> "<<cor3<<endl;
	cout<<"Point cor4: "<<pcor4<<" -> "<<cor4<<endl;
	*/

	//4-Add corner area points
	if(lim2<lim1){
		if(lim1<cor2) vpts.push_back(Point(round(pcor2.x),round(pcor2.y)));
		if(lim1<cor3) vpts.push_back(Point(round(pcor3.x),round(pcor3.y)));
		if(lim1<cor4) vpts.push_back(Point(round(pcor4.x),round(pcor4.y)));
		if(lim2!=cor1) vpts.push_back(Point(round(pcor1.x),round(pcor1.y)));
		if(lim2>cor2) vpts.push_back(Point(round(pcor2.x),round(pcor2.y)));
		if(lim2>cor3) vpts.push_back(Point(round(pcor3.x),round(pcor3.y)));
		if(lim2>cor4) vpts.push_back(Point(round(pcor4.x),round(pcor4.y)));
	}else{
		if(lim1<cor2 and lim2>cor2) vpts.push_back(Point(round(pcor2.x),round(pcor2.y)));
		if(lim1<cor3 and lim2>cor3) vpts.push_back(Point(round(pcor3.x),round(pcor3.y)));
		if(lim1<cor4 and lim2>cor4) vpts.push_back(Point(round(pcor4.x),round(pcor4.y)));
	}

	vector<Point>::reverse_iterator rit=vpts2.rbegin();
	for(; rit!=vpts2.rend(); rit++){
		vpts.push_back(*rit);
	}
	/* //Logging
	vector<Point>::iterator ite = vpts.begin();
	for(; ite<vpts.end(); ite++){
		cout<<*ite<<endl;
	}*/

	//5-Draw mFloor and mWall
	mWalls = Mat::zeros(1, mWLength, CV_64F);
	if(lim2<lim1){ 

		for(int i = lim1; i<mWLength; i++){
			mWalls.at<double>(0,i) = 1;	
		}

		for(int i = 0; i<=lim2; i++){
			mWalls.at<double>(0,i) = 1;	
		}
	}else{
		for(int i = lim1; i<=lim2; i++){
			mWalls.at<double>(0,i) = 1;	
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
	mCeiling=mFloor.clone();

	/* //Logging
	imshow("suuh", mFloor);
	waitKey(0);*/

	//6-Normalize mFoor and mWalls
	markPxFloor = 0;
	markPxWalls = 0;

	for(int j = 0; j < mWalls.cols; j++){
		if(mWalls.at<double>(0,j) != 0 and mWalls.at<double>(0,j) != 1) {cout<<"Error: wrong mWalls value"<<endl; return -1;}
		if(mWalls.at<double>(0,j) != 0) markPxWalls+=1;
	}

	for(int i = 0; i < mFloor.rows; i++){
		for(int j = 0; j < mFloor.cols; j++){
			if(mFloor.at<double>(i,j) != 0 and mFloor.at<double>(i,j) != 1) {cout<<"Error: wrong mFloor value"<<endl; return -1;}
			if(mFloor.at<double>(i,j) != 0) markPxFloor+=1;
		}
	} 
	/* //Logging
	cout<<markPxFloor<<" "<<markPxWalls<<endl;*/

	//--------------find tpos-bounds intersections--------------//
	cout<<"tAngle: "<<tAngle<<endl;
	if(findIntersection(plim3, tpos, Point2d(tpos.x+1,tpos.y-tan(tAngle)), pcor1, pcor2)) vplim3.push_back(plim3);
	if(findIntersection(plim3, tpos, Point2d(tpos.x+1,tpos.y-tan(tAngle)), pcor2, pcor3)) vplim3.push_back(plim3);
	if(findIntersection(plim3, tpos, Point2d(tpos.x+1,tpos.y-tan(tAngle)), pcor3, pcor4)) vplim3.push_back(plim3);
	if(findIntersection(plim3, tpos, Point2d(tpos.x+1,tpos.y-tan(tAngle)), pcor4, pcor1)) vplim3.push_back(plim3);

	//--------------check correct intersection--------------//
	vector<Point2d>::iterator it3 = vplim3.begin();

	for(; it3!=vplim3.end();){
		if(round((*it3).y) == 0 or round((*it3).y) == imgSize.height-1){
			if((*it3).x < 0 or (*it3).x > imgSize.width-1) vplim3.erase(it3);
			else it3++;

		}else if(round((*it3).x) == 0 or round((*it3).x) == imgSize.width-1){
			if((*it3).y < 0 or (*it3).y > imgSize.height-1) vplim3.erase(it3);
			else it3++;

		}else {
			cout<<"Error intersection vplim point"<<endl;
		}
	}

	it3 = vplim3.begin();

	cout<<"vplim: ";
	for(; it3!=vplim3.end(); it3++){
		cout<<(*it3)<<" ,";
	}
	cout<<endl;
	it3 = vplim3.begin();


	if(vplim3.size() != 0){
		if(sin(tAngle) == 1){ //90 deg
			for(; it3!=vplim3.end(); it3++){
				if(it3 == vplim3.begin()) plim3=(*it3);
				else if((*it3).y < plim3.y) plim3=(*it3);
			}
		}else if(sin(tAngle == -1)){ //270 deg
			for(; it3!=vplim3.end(); it3++){
				if(it3 == vplim3.begin()) plim3=(*it3);
				else if((*it3).y > plim3.y) plim3=(*it3);
			}
		}else if(cos(tAngle) > 0){ //  >90 and <270 deg
			for(; it3!=vplim3.end(); it3++){
				if(it3 == vplim3.begin()) plim3=(*it3);
				else if((*it3).x > plim3.x) plim3=(*it3);
			}
		}else if(cos(tAngle) < 0){ // >270 and <90 deg
			for(; it3!=vplim3.end(); it3++){
				if(it3 == vplim3.begin()) plim3=(*it3);
				else if((*it3).x < plim3.x) plim3=(*it3);
			}
		}else cout<<"Error impossible angle"<<endl;
	}else cout<<"Error plim3 intersection not found"<<endl;
	
	//--------------calculate ecliudean distance--------------//
	cout<<"plim3: "<<plim3<<" tpos: "<<tpos<<endl;
	dist = sqrt(pow(plim3.x-tpos.x,2)+pow(plim3.y-tpos.y,2));

	cout<<"Dist: "<<dist<<endl;
	//--------------calculate weights-------------------------//
	aFloor = atan(persHeight/dist);
	aCeiling = atan(wallsHeight-persHeight/dist);
	aWalls = aFloor+aCeiling-180;

	wFloor = aFloor/180;
	wCeiling = aCeiling/180;
	wWalls = aWalls/180;

	cout<<wFloor<<" "<<wCeiling<<" "<<wWalls<<endl;

	//--------------replace weights---------------------------//
	replaceWeights(mFloor, wFloor, 1);
	replaceWeights(mWalls, wWalls, 1);
	replaceWeights(mWalls, wCeiling, 1);

	/*//Logging
	imshow("suuh", mFloor);
	waitKey(0);*/

	//7-Add to de total

}

