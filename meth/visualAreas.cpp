#include "opencv2/opencv.hpp"
#include <cmath>
#include "opencv/cv.h"
#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <string>
#include <dirent.h>
#include <math.h>
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

bool getWallPosition(int& wpos, Point p, cv::Size roomSize){
	if(p.y==0){
		if(p.x<0 or p.x>roomSize.width-1) return false;
		else {wpos=p.x; return true;}
	}else if(p.x==roomSize.width-1){
		if(p.y<1 or p.y>roomSize.height-1) return false;
		else {wpos=p.x+p.y; return true;}
	}else if(p.y==roomSize.height-1){
		if(p.x<0 or p.x>roomSize.width-2) return false;
		else {wpos=roomSize.width*2+roomSize.height-p.x-3; return true;} 
	}else{
		if(p.y<1 or p.y>roomSize.height-2) return false;
		else {wpos=roomSize.width*2+roomSize.height*2-p.y-4; return true;}
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
	double RPM = 130;
	string save_file,routes_name;
	bf::path routes_path, save_dir = "./generatedVisualAreas/";

	if (argc < 2 || argc > 5) {
	//if (argc < 1 || argc > 5) {
		cout<<"USAGE:"<<endl<< "./visualAreas rotues_path [horizontal_angle] [save_file] [dir_save/]"<<endl<<endl;
		cout<<"DEFAULT [horizontal_angle] = 20"<<endl;
		cout<<"DEFAULT [dir_save/] = "<<save_dir.native()<<endl;
		cout<<"DEFAULT [save_file] = routes_name+horizontal_angle.yml"<<endl;
		return -1;
	}else{
		routes_path = argv[1];
		//routes_path = "/home/ivan/videos/sequences/pack3/routes.yml";
		//routes_path = "/home/ivan/videos/sequences/pack1/routes/sonia_gasco_original.yml";
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
			
			save_file=to_string((int)(hAngle*180/M_PI))+"_"+routes_path.filename().native();
		}
		if(argc == 5) save_dir = argv[4];
	}

	//Verify and create correct directories
	if(!verifyDir(save_dir,true)) return -1;

	cv::Size roomSize, imgSize;
	roomSize.width = 1101; //1101 8
	roomSize.height = 828; //828 6
	imgSize.width = 640; //640 5
	imgSize.height = 480; //480 4 
	double persHeight = 1.6*RPM;//1.6*RPM 3
	double sensorHeight = 3.07*RPM; //3.07*RPM 4
	Point camCoor(round(5.35*RPM-2.46*RPM),round(3.77*RPM-1.845*RPM));//Point camCoor(3,2);
	int mWLength = roomSize.width*2+(roomSize.height-2)*2;

	//Declare variables
	int lim1,lim2,cor1,cor2,cor3,cor4,markPxFloor,markPxWalls;
	double tAngle, dist, wFloor, wWalls, wCeiling, aFloor, aWalls, aCeiling;
	Point2d tpos, ori, prev, act, plim1, plim2, plim3;  //2 dimensions <double> precission
	Point pver1, pver2, pver3, pver4;
	vector<Point> vpts, vpts2;
	vector<Point2d> vplim3;
	Mat mR, mOri, mAct, mWalls, mFloor, mCeiling, mTotalWalls, mTotalFloor, mTotalCeiling; 
	bool correctedAct,firstFunctionDone;

	//Define room vertices
	pver1.x=0; pver1.y=0;
	pver2.x=roomSize.width-1; pver2.y=0;
	pver3.x=roomSize.width-1; pver3.y=roomSize.height-1;
	pver4.x=0; pver4.y=roomSize.height-1;

	//Inicialize mTotals
	mTotalFloor = Mat::zeros(roomSize, CV_64F);
	mTotalWalls = Mat::zeros(1, mWLength, CV_64F);
	mTotalCeiling = Mat::zeros(roomSize, CV_64F);

	loadRoutes(routes_path);
	vector<TrayPoint>::iterator it=rutas.begin();
	for(; it<rutas.end(); it++){
		tAngle = (it->angle)*M_PI/180;
		tpos = (it->pos);
		act.x=-1; act.y=-1;
		ori.x=0;
		correctedAct=false;
		firstFunctionDone=false;
		if(vplim3.size()!=0) vplim3.clear();
		if(vpts2.size()!=0) vpts2.clear();
		if(vpts.size()!=0) vpts.clear();

		//0-Prove tpos is inside the image bounds
		if(tpos.x<0 or tpos.x>imgSize.width-1 or tpos.y<0 or tpos.y>imgSize.height-1){
			cout<<"Error tpos is out of image bounds"<<endl;
			return -1;
		}
		cout<<"it->pos: "<<tpos<<endl;

		//1-Correction of tpos real position
		tpos.x-= imgSize.width/2;
		tpos.y-= imgSize.height/2;
		tpos-= persHeight/sensorHeight*tpos;
		tpos.x+= imgSize.width/2;
		tpos.y+= imgSize.height/2;

		//2-Move tpos from sensor to room coordinates
		cout<<"camCoor: "<<camCoor<<" it->pos: "<<tpos<<endl;
		tpos.x += camCoor.x;
		tpos.y += camCoor.y;
		cout<<"tpos: "<<tpos<<endl;
		vpts.push_back(Point(round(tpos.x),round(tpos.y)));
		mR = (Mat_<double>(2,2)<<cos(tAngle),-sin(tAngle),sin(tAngle),cos(tAngle));

		//3-Generate function points
		while(1){

			//--------------compute new function point--------//
			prev=act;
			ori.y=tan(hAngle)*sqrt(pow(persHeight,2)+pow(ori.x,2));
			if(firstFunctionDone) ori.y*=-1.0;
			mOri = (Mat_<double>(2,1)<<ori.x,ori.y);
			mAct = mR*mOri;
			//cout<<mAct<<endl; //Logging
			act.x=mAct.at<double>(0,0)+tpos.x;
			act.y=-1*mAct.at<double>(1,0)+tpos.y;

			//--------------verify out of bounds--------------//
			if(act.y<=0 or act.x>=roomSize.width-1 or act.y>=roomSize.height-1 or act.x<=0){//act point is in the limit or out of bounds
				correctedAct=false;
				while(round(act.y)<0 or round(act.x)>roomSize.width-1 or round(act.y)>roomSize.height-1 or round(act.x)<0 or !correctedAct){
					if(act.y<0){
						if(!findIntersection(act, Point2d(act), prev, pver1, pver2)) {
							cout<<"Error intersection: "<<act<<prev<<" type1"<<endl; 
							return -1;
						}
					}
					if(act.x>roomSize.width-1){
						if(!findIntersection(act, Point2d(act), prev, pver2, pver3)) {
							cout<<"Error intersection: "<<act<<prev<<" type2"<<endl; 
							return -1;
						}
					}
					if(act.y>roomSize.height-1){
						if(!findIntersection(act, Point2d(act), prev, pver3, pver4)) {
							cout<<"Error intersection: "<<act<<prev<<" type4"<<endl; 
							return -1;
						}
					}
					if(act.x<0){
						if(!findIntersection(act, Point2d(act), prev, pver4, pver1)) {
							cout<<"Error intersection: "<<act<<prev<<" type3"<<endl; 
							return -1;
						}
					}
					correctedAct=true;
				}
				ori.x=0;
				if(firstFunctionDone){
					if(round(prev.x) != round(act.x) or round(prev.y) != round(act.y)) {
						vpts2.push_back(Point(round(act.x),round(act.y)));
					}
					plim2=act;
					break;
				}else {
					if(round(prev.x) != round(act.x) or round(prev.y) != round(act.y)) {
						vpts.push_back(Point(round(act.x),round(act.y)));
					}
					plim1=act;
					firstFunctionDone=true;
				}
			}

			else {//act point is inside bounds
				ori.x+=1;
				if(firstFunctionDone){
					if(round(prev.x) != round(act.x) or round(prev.y) != round(act.y)) {
						vpts2.push_back(Point(round(act.x),round(act.y)));
					}
				}else{
					if(round(prev.x) != round(act.x) or round(prev.y) != round(act.y)) {
						vpts.push_back(Point(round(act.x),round(act.y)));
					}
				}
				
			}
		}

		//4-Get mWalls positions
		if(!getWallPosition(lim1,plim1,roomSize)) {cout<<"Error wrong Wall Position lim1 "<<plim1<<endl; return -1;}
		if(!getWallPosition(lim2,plim2,roomSize)) {cout<<"Error wrong Wall Position lim2 "<<plim2<<endl; return -1;}
		cor1 = 0;
		if(!getWallPosition(cor2,pver2,roomSize)) {cout<<"Error wrong Wall Position cor2"<<endl; return -1;}
		if(!getWallPosition(cor3,pver3,roomSize)) {cout<<"Error wrong Wall Position cor3"<<endl; return -1;}
		if(!getWallPosition(cor4,pver4,roomSize)) {cout<<"Error wrong Wall Position cor4"<<endl; return -1;}

		/*//Logging
		cout<<"Point lim1: "<<plim1<<" -> "<<lim1<<endl;
		cout<<"Point lim2: "<<plim2<<" -> "<<lim2<<endl;
		cout<<"Point cor1: "<<pver1<<" -> "<<cor1<<endl;
		cout<<"Point cor2: "<<pver2<<" -> "<<cor2<<endl;
		cout<<"Point cor3: "<<pver3<<" -> "<<cor3<<endl;
		cout<<"Point cor4: "<<pver4<<" -> "<<cor4<<endl;*/
		

		//5-Add corner area points
		if(lim2<lim1){
			if(lim1<cor2) vpts.push_back(pver2);
			if(lim1<cor3) vpts.push_back(pver3);
			if(lim1<cor4) vpts.push_back(pver4);
			if(lim2!=cor1) vpts.push_back(pver1);
			if(lim2>cor2) vpts.push_back(pver2);
			if(lim2>cor3) vpts.push_back(pver3);
			if(lim2>cor4) vpts.push_back(pver4);
		}else{
			if(lim1<cor2 and lim2>cor2) vpts.push_back(Point(round(pver2.x),round(pver2.y)));
			if(lim1<cor3 and lim2>cor3) vpts.push_back(Point(round(pver3.x),round(pver3.y)));
			if(lim1<cor4 and lim2>cor4) vpts.push_back(Point(round(pver4.x),round(pver4.y)));
		}

		vector<Point>::reverse_iterator rit=vpts2.rbegin();
		for(; rit!=vpts2.rend(); rit++){
			vpts.push_back(*rit);
		}
		/*//Logging
		vector<Point>::iterator ite = vpts.begin();
		for(; ite<vpts.end(); ite++){
			cout<<*ite<<endl;
		}*/

		//6-Draw mFloor and mWall
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

		mFloor = Mat::zeros(roomSize, CV_64F);
		Point pts[1][vpts.size()];
		vector<Point>::iterator it2=vpts.begin();
		int posPts=0;
		for(; it2!=vpts.end(); it2++){
			pts[0][posPts]=(*it2);
			posPts+=1;
		}

		//cv::fillConvexPoly(mFloor, pts, vpts.size(),cv::Scalar(1)); //Leaves some areas not drawed
		int npt[] = {(int) vpts.size()};
		const Point* ppt[1] = {pts[0]}; 
		cv::fillPoly(mFloor, ppt, npt, 1, cv::Scalar(1),8);
		mCeiling=mFloor.clone();

		//7-Normalize mFoor and mWalls
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
		if(findIntersection(plim3, tpos, Point2d(tpos.x+1,tpos.y-tan(tAngle)), pver1, pver2)) vplim3.push_back(plim3);
		if(findIntersection(plim3, tpos, Point2d(tpos.x+1,tpos.y-tan(tAngle)), pver2, pver3)) vplim3.push_back(plim3);
		if(findIntersection(plim3, tpos, Point2d(tpos.x+1,tpos.y-tan(tAngle)), pver3, pver4)) vplim3.push_back(plim3);
		if(findIntersection(plim3, tpos, Point2d(tpos.x+1,tpos.y-tan(tAngle)), pver4, pver1)) vplim3.push_back(plim3);

		//--------------check correct intersection--------------//
		vector<Point2d>::iterator it3 = vplim3.begin();

		for(; it3!=vplim3.end();){
			if(round((*it3).y) == 0 or round((*it3).y) == roomSize.height-1){
				if((*it3).x < 0 or (*it3).x > roomSize.width-1) vplim3.erase(it3);
				else it3++;

			}else if(round((*it3).x) == 0 or round((*it3).x) == roomSize.width-1){
				if((*it3).y < 0 or (*it3).y > roomSize.height-1) vplim3.erase(it3);
				else it3++;

			}else {
				cout<<"Error intersection vplim point"<<endl;
				return -1;
			}
		}

		it3 = vplim3.begin();

		cout<<"vplim3: ";
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
			}else if(sin(tAngle) == -1){ //270 deg
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
			}else {
				cout<<"Error impossible angle"<<endl;
				return -1;
			}
		}else {
			cout<<"Error plim3 intersection not found"<<endl;
			return -1;
		}
		
		//--------------calculate ecliudean distance--------------//
		cout<<"plim3: "<<plim3<<" tpos: "<<tpos<<endl;
		dist = sqrt(pow(plim3.x-tpos.x,2)+pow(plim3.y-tpos.y,2));

		cout<<"Dist: "<<dist<<endl;
		//--------------calculate weights-------------------------//
		aFloor = atan(dist/persHeight);
		aCeiling = atan(dist/(sensorHeight-persHeight));
		aWalls = M_PI-aFloor-aCeiling;

		wFloor = aFloor/M_PI;
		wCeiling = aCeiling/M_PI;
		wWalls = aWalls/M_PI;

		cout<<wFloor<<" "<<wCeiling<<" "<<wWalls<<endl;

		//--------------replace weights---------------------------//
		replaceWeights(mFloor, wFloor, 1);
		replaceWeights(mWalls, wWalls, 1);
		replaceWeights(mWalls, wCeiling, 1);

		//Logging
		imshow("image", mFloor);
		waitKey(0);

		//8-Add to de total
		mTotalFloor+=mFloor;
		mTotalCeiling+=mCeiling;
		mTotalWalls+=mWalls;

	}
	mTotalFloor=mTotalFloor*(1/(double)rutas.size());
	mTotalCeiling=mTotalCeiling*(1/(double)rutas.size());
	mTotalWalls=mTotalWalls*(1/(double)rutas.size());

	/*//Logging
	imshow("image", mTotalFloor);
	waitKey(0);*/

	//9-Save matrices
	char op;
	cout<<"Do you want to save result? [y/n]"<<endl;
	cin>>op;
	while(op != 'y' and op != 'n'){
		cout<<"Invalid option"<<endl;
		cin>>op;
	}
	if(op == 'y'){
		cout<<endl<<"Saving Results ";
		FileStorage fs_result;
		cout<<save_dir.native()+save_file<<endl;
		if(fs_result.open(save_dir.native()+save_file, FileStorage::WRITE)){
			fs_result<<"hAngle"<<(int)(hAngle*180/M_PI);
			fs_result<<"mTotalFloor"<<mTotalFloor;
			fs_result<<"mTotalWalls"<<mTotalWalls;
			fs_result<<"mTotalCeiling"<<mTotalCeiling;
			fs_result.release();
			cout<<"OK!"<<endl;
		}else{
			cout<<"ERROR: Can't open for write"<<endl<<endl;
			return -1;
		}
	}
}

