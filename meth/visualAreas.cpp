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
vector<bf::path> all_masks;
bool b_all = false;

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

bool addObjectMask(Mat& mobj, bf::path masks_dir, string& mask_name){
	char op;
	while(1){
		cout<<endl<<endl<<endl;
		if(all_masks.empty() and !b_all){
			cout<<"Press 'n' to add new object mask"<<endl;
			cout<<"Press 'a' to add all object masks in the directory"<<endl;
			cout<<"Press 'q' to quit and save"<<endl;
			cin>>op;
			cin.clear(); cin.ignore();
			switch (op){
				case 'n':
					mask_name="";
					cout<<"Introduce the name of the PNG objectMask name you want to add"<<endl;
					cin>>mask_name; 
					cin.clear(); cin.ignore();
					if(!bf::exists(masks_dir.native()+mask_name+".png")) {cout<<"Error: "<<masks_dir.native()+mask_name<<" doesn't exist"<<endl; break;}
					mobj = imread(masks_dir.native()+mask_name+".png", CV_LOAD_IMAGE_GRAYSCALE);
					if(!mobj.data){cout << "Could not open or find the image" <<endl; break;}
					return true;
				break;
				case 'a':
					std::copy(bf::directory_iterator(masks_dir), bf::directory_iterator(), back_inserter(all_masks));
					b_all = true;
				break;
				case 'q':
					return false;
				break;
			}
		}else if(all_masks.empty() and b_all){
			return false;
		}else{
			mask_name = all_masks.back().filename().native();
			cout<<"Loading "<<mask_name<<endl;
			while(mask_name.find(".png") == string::npos){
				all_masks.pop_back();
				if(all_masks.empty()) return false;
				else mask_name = all_masks.back().filename().native();
			}
			all_masks.pop_back();
			mobj = imread(masks_dir.native()+mask_name, CV_LOAD_IMAGE_GRAYSCALE);
			if(!mobj.data){cout << "Could not open or find the image" <<endl; exit(-1);}
			cout<<"OK!"<<endl;
			return true;
		}
	}
}

int main( int argc, char* argv[] ) {
	float RPM = 130, room_width_dim = 8.47, room_length_dim = 6.37, room_height_dim = 3.12,
	sensorHeight = 3.07, persHeight = 1.6, hAngle = 124, roomHeight, forehead_dim = 0.14, persEyesHeight,
	eAngle = 5;
	cv::Size imgSize(640,480); // (5 ,4);
	string results_file,routes_name;
	bf::path routes_path, save_dir = "./generatedVisualAreas/",masks_dir = "./../omask/masks/";

	if (argc < 2 || argc > 15) {
		cout<<"USAGE:"<<endl<< "./visualAreas rotues_path [horizontal_angle] [results_name] [dir_obj_masks] " <<endl 
		<<"[dir_save/] [person_height] [room_height] [room_width] [room_length] "<<endl 
		<<"[relation_pixel_meter] [sensor_height] [error_angle] [image_width_resolution] "<<endl
		<<"[image_height_resolution] "<<endl<<endl;

		cout<<"DEFAULT [horizontal_angle] = "<<hAngle<<endl;
		cout<<"DEFAULT [results_name] = routes_name"<<endl;
		cout<<"DEFAULT [dir_obj_masks] = "<<masks_dir.native()<<endl;
		cout<<"DEFAULT [dir_save/] = "<<save_dir.native()<<endl;
		cout<<"DEFAULT [person_height] = "<<persHeight<<endl;
		cout<<"DEFAULT [room_height] = "<<room_height_dim<<endl;
		cout<<"DEFAULT [room_width] = "<<room_width_dim<<endl;
		cout<<"DEFAULT [room_length] = "<<room_length_dim<<endl;
		cout<<"DEFAULT [relation_pixel_meter] = "<<RPM<<endl;
		cout<<"DEFAULT [sensor_height] = "<<sensorHeight<<endl;
		cout<<"DEFAULT [error_angle] = "<<eAngle<<endl;
		cout<<"DEFAULT [image_width_resolution] = "<<imgSize.width<<endl;
		cout<<"DEFAULT [image_height_resolution] = "<<imgSize.height<<endl;
		return -1;
		
	}else{
		routes_path = argv[1];
		if(!bf::exists(routes_path) or !routes_path.has_filename()) {cout<<"Wrong routes_path"<<endl; return -1;}
		if(argc > 2) {hAngle = atof(argv[2]); if(hAngle > 114 or hAngle<0) {cout<<"Error: wrong hAngle "<<hAngle<<endl; return -1;}}
		if(argc > 3) {results_file = argv[3]; results_file+=".yml";}
		else results_file=routes_path.filename().native();
		if(argc > 4) masks_dir = argv[4];
		if(argc > 5) save_dir = argv[5];
		if(argc > 6) persHeight = atof(argv[6]);
		if(argc > 7) room_height_dim = atof(argv[7]);
		if(argc > 8) room_width_dim = atof(argv[8]);
		if(argc > 9) room_length_dim = atof(argv[9]);
		if(argc > 10) RPM = atof(argv[10]);
		if(argc > 11) sensorHeight = atof(argv[11]);
		if(argc > 12) {eAngle = atof(argv[12]); if(eAngle >= 55 or eAngle<0) {cout<<"Error: wrong eAngle "<<eAngle<<endl; return -1;}}
		if(argc > 13) imgSize.width = atoi(argv[13]);
		if(argc == 15) imgSize.height = atoi(argv[14]);
		if(hAngle == 0 and eAngle ==0) {cout<<"Error: both eAngle and hAngle are zero"<<endl; return -1;}
	}

	//Verify and create correct directories
	if(!verifyDir(save_dir,true)) return -1;

	cv::Size roomSize;
	roomSize.width = round(room_width_dim*RPM); //1101 8
	roomSize.height = round(room_length_dim*RPM); //828 6
	persHeight *= RPM;//3
	sensorHeight *= RPM; //4
	forehead_dim *= RPM;
	persEyesHeight = persHeight - forehead_dim;
	roomHeight = room_height_dim*RPM;
	hAngle *= M_PI/180/2;
	eAngle *= M_PI/180;
	hAngle += eAngle;
	Point camCoor(round(5.35*RPM-imgSize.width/2),round(3.77*RPM-imgSize.height/2));//Point camCoor(3,2);

	/****Obtain object masks****/
	struct Obj{Mat mask; string name; double punctuation; double punctuation2;};
	Obj o;
	vector<Obj> vobj;
	vector<Obj>::iterator oit;
	Mat mobj, mTotal = Mat::zeros(roomSize, CV_8UC1), mobj_room;
	while(addObjectMask(mobj, masks_dir, o.name)){
		if(mobj.size() != imgSize){cout<<"Error: routes image size is different from "<<o.name<<" mask size"<<endl; return -1;}
		mobj_room = Mat::zeros(roomSize, CV_8UC1);
		mobj.copyTo(mobj_room(Rect(camCoor.x, camCoor.y, imgSize.width, imgSize.height)));
		mTotal+=mobj_room;
		o.mask=mobj_room.clone();
		
		//cout<<o.name<<endl;
		vobj.push_back(o);
	}
	/*imshow("image", mTotal);
	waitKey(0);*/

	/****Compute visual Areas (this version only floor ones)****/

	//Declare variables
	int lim1,lim2,cor1,cor2,cor3,cor4,markPxFloor;
	double tAngle, dist, wFloor, aFloor, wWalls, wCeiling, aWalls, aCeiling;
	Point2d tpos, ori, prev, act, plim1, plim2, plim3;  //2 dimensions <double> precission
	Point pver1, pver2, pver3, pver4;
	vector<Point> vpts, vpts2;
	vector<Point2d> vplim3;
	Mat mR, mOri, mAct, mFloor, mTotalFloor; 
	bool correctedAct,firstFunctionDone;

	//Define room vertices
	pver1.x=0; pver1.y=0;
	pver2.x=roomSize.width-1; pver2.y=0;
	pver3.x=roomSize.width-1; pver3.y=roomSize.height-1;
	pver4.x=0; pver4.y=roomSize.height-1;

	//Inicialize mTotals
	mTotalFloor = Mat::zeros(roomSize, CV_64F);

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
		//Logging: cout<<"it->pos: "<<tpos<<endl;

		//1-Correction of tpos real position
		tpos.x-= imgSize.width/2;
		tpos.y-= imgSize.height/2;
		tpos-= persHeight/sensorHeight*tpos;
		tpos.x+= imgSize.width/2;
		tpos.y+= imgSize.height/2;

		//2-Move tpos from sensor to room coordinates
		//Logging: cout<<"camCoor: "<<camCoor<<" it->pos: "<<tpos<<endl;
		tpos.x += camCoor.x;
		tpos.y += camCoor.y;
		//Logging: cout<<"tpos: "<<tpos<<endl;
		vpts.push_back(Point(round(tpos.x),round(tpos.y)));
		mR = (Mat_<double>(2,2)<<cos(tAngle),-sin(tAngle),sin(tAngle),cos(tAngle));

		//3-Generate function points
		while(1){

			//--------------compute new function point--------//
			prev=act;
			ori.y=tan(hAngle)*sqrt(pow(persEyesHeight,2)+pow(ori.x,2));
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

		//6-Draw mFloor
		mFloor = Mat::zeros(roomSize, CV_64F);
		Point pts[1][vpts.size()];
		vector<Point>::iterator it2=vpts.begin();
		int posPts=0;
		for(; it2!=vpts.end(); it2++){
			pts[0][posPts]=(*it2);
			posPts+=1;
		}

		//cv::fillConvexPoly(mFloor, pts, vpts.size(),cv::Scalar(1)); //Leaves some areas not drawn
		int npt[] = {(int) vpts.size()};
		const Point* ppt[1] = {pts[0]}; 
		cv::fillPoly(mFloor, ppt, npt, 1, cv::Scalar(1),8);

		if(it==rutas.begin()){//Logging
			imshow("image", mFloor);
			waitKey(5);
		}


		//7-Normalize mFoor
		markPxFloor = 0;

		for(int i = 0; i < mFloor.rows; i++){
			for(int j = 0; j < mFloor.cols; j++){
				if(mFloor.at<double>(i,j) != 0 and mFloor.at<double>(i,j) != 1) {cout<<"Error: wrong mFloor value"<<endl; return -1;}
				if(mFloor.at<double>(i,j) != 0) markPxFloor+=1;
			}
		} 
		/* //Logging
		cout<<markPxFloor<<endl;*/

		//--------------find tpos-bounds intersections--------------//
		//Logging: cout<<"tAngle: "<<tAngle<<endl;
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

		/*Logging
		cout<<"vplim3: ";
		for(; it3!=vplim3.end(); it3++){
			cout<<(*it3)<<" ,";
		}
		cout<<endl;*/
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
		//Logging: cout<<"plim3: "<<plim3<<" tpos: "<<tpos<<endl;
		dist = sqrt(pow(plim3.x-tpos.x,2)+pow(plim3.y-tpos.y,2));

		//Logging: cout<<"Dist: "<<dist<<endl;
		//--------------calculate weights-------------------------//
		aFloor = atan(dist/persEyesHeight);
		aCeiling = atan(dist/(roomHeight-persEyesHeight));
		aWalls = M_PI-aFloor-aCeiling;

		wFloor = aFloor/M_PI;
		wCeiling = aCeiling/M_PI;
		wWalls = aWalls/M_PI;

		cout<<wFloor<<" "<<wCeiling<<" "<<wWalls<<endl;

		
		//--------------replace weights---------------------------//
		replaceWeights(mFloor, wFloor/markPxFloor, 1);

		

		//8-Add to de total
		mTotalFloor+=mFloor;

	}
	mTotalFloor=mTotalFloor*(1/(double)rutas.size());

	/*//Logging
	imshow("image", mTotalFloor);
	waitKey(0);*/

	//9-Save results
	char op;
	cout<<"Do you want to save result? [y/n]"<<endl;
	cin>>op;
	while(op != 'y' and op != 'n'){
		cout<<"Invalid option"<<endl;
		cin>>op;
	}
	if(op == 'y'){
		char buffer [50];
		double totalP=0, totalP2=0;
		cout<<endl<<"Saving Results ";
		FileStorage fs;
		cout<<save_dir.native()+results_file<<endl;
		if(fs.open(save_dir.native()+results_file, FileStorage::WRITE)){
			time_t rawtime; time(&rawtime);
	    	fs << "Date"<< asctime(localtime(&rawtime));
			fs << "RPM" << RPM;
			fs << "sensor_height" << sensorHeight;
			fs << "person_height" << persHeight;
			fs << "person_eyes_height" << persEyesHeight;
			fs << "horizonal_angle"<< (hAngle*180/M_PI - eAngle*180/M_PI)*2;
			fs << "error_angle" << eAngle*180/M_PI;
			fs << "imgSize" << imgSize;
			fs << "number_objects" << (int) vobj.size();
			fs << "number_areas" << (int) rutas.size();
			fs << "objects" << "[";
			for(oit=vobj.begin(); oit!=vobj.end(); oit++){
				mFloor = Mat::zeros(roomSize, CV_64F);
				mTotalFloor.copyTo(mFloor,oit->mask);
				oit->punctuation = sum(mFloor)[0];
				oit->punctuation2 = (double) sum(mFloor)[0]/ (double) countNonZero(mFloor);
				totalP += oit->punctuation;
				totalP2 += oit->punctuation2;
			}

			for(oit=vobj.begin(); oit!=vobj.end(); oit++){
				sprintf (buffer,"%.2f", oit->punctuation2/totalP2);
				fs << "{:" << "punctuation2_n" << buffer << "name" << oit->name 
				<< "punctuation_fn" << oit->punctuation/totalP 
				<< "punctuation2_fn" << oit->punctuation2/totalP2 << "}";
				
				cout<<buffer<<" "<<oit->name<<endl;
			}
			fs << "]";
			fs << "totalPunctuation"<<totalP;
			//fs << "mTotalFloor"<<mTotalFloor;
			fs.release();
			cout<<"OK!"<<endl;
		}else{
			cout<<"ERROR: Can't open for write"<<endl<<endl;
			return -1;
		}
	}
}

