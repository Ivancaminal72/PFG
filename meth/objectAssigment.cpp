#include "opencv2/opencv.hpp"
#include <cmath>
#include <climits>
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

bool addObjectMask(Mat& mobj, bf::path masks_dir, string& mask_name){
	char op;
	while(1){
		cout<<endl<<endl<<endl;
		cout<<"Press 'a' to add new object mask"<<endl;
		cout<<"Press 'q' to quit and save"<<endl;
		cin>>op;
		cin.clear(); cin.ignore();
		switch (op){
			case 'a':
				mask_name="";
				cout<<"Introduce the name of the PNG objectMask name you want to add"<<endl;
				cin>>mask_name; 
				cin.clear(); cin.ignore();
				if(!bf::exists(masks_dir.native()+mask_name+".png")) {cout<<"Error: "<<masks_dir.native()+mask_name<<" doesn't exist"<<endl; break;}
				mobj = imread(masks_dir.native()+mask_name+".png", CV_LOAD_IMAGE_GRAYSCALE);
				if(!mobj.data){cout <<  "Could not open or find the image" <<endl; break;}
				return true;
			break;
			case 'q':
				return false;
			break;
		}
	}
}

int main( int argc, char* argv[] ) {
	double hAngle = 90*M_PI/180;//mean horizontal visualCamp (verify >= 90 degrees)
	double RPM = 130;
	string results_file;
	bf::path masks_dir = "./../omask/masks/", routes_path, save_dir = "./generatedObjectAssigments/";

	if (argc < 2 || argc > 5) {//wrong arguments
		cout<<"USAGE:"<<endl<< "./objectAssigment rotues_path [results_name] [dir_save/] [dir_obj_masks]"<<endl<<endl;
		cout<<"DEFAULT [results_name]  = routes_name"<<endl;
		cout<<"DEFAULT [dir_save/]     = "<<save_dir.native()<<endl;
		cout<<"DEFAULT [dir_obj_masks] = "<<masks_dir.native()<<endl;
		return -1;
	}else{
		routes_path = argv[1];
		if(!bf::exists(routes_path) or !routes_path.has_filename()) {cout<<"Wrong routes_path"<<endl; return -1;}
		if(argc > 2) {results_file = argv[2]; results_file+=".yml";}
		else {results_file=routes_path.filename().native();}
		if(argc > 3) save_dir = argv[3];
		if(argc == 5) masks_dir = argv[4];

	}

	//Verify and create correct directories
	if(!verifyDir(save_dir,true)) return -1;
	if(!verifyDir(masks_dir,false)) return -1;

	struct Obj{Point2d cen; string name; int assigments=0;};
	Obj o;
	Mat mobj;
	vector<Obj> vobj;
	vector<Obj>::iterator oit;
	Moments M;
	
	/****Calculate object masks centroids****/
	while(addObjectMask(mobj, masks_dir, o.name)){
		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;
		findContours( mobj.clone(), contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

		if(contours.size()!=1){cout<<"Error: more than one contour found"<<endl; return -1;}
		
		//Find mask moments
		M = moments(contours[0]);

		//Get the mass centers
		o.cen = Point2d(M.m10/M.m00, M.m01/M.m00); 
		//cout<<o.cen<<endl; cout.flush();
		//cout<<o.name<<endl;
		vobj.push_back(o);
	}

	cv::Size imgSize;
	imgSize.width = 640; //640 5
	imgSize.height = 480; //480 4
	double persHeight = 1.6*RPM;//1.6*RPM 3
	double sensorHeight = 3.1*RPM; //~3*RPM 4

	//Declare variables
	int dist, best;
	Point2d tpos;
	double tAngle;
	Mat mRot, mTra, mOri, mRes;


	/****Compute the assigments for each Traypoint****/
	loadRoutes(routes_path);
	vector<TrayPoint>::iterator it=rutas.begin();
	for(; it<rutas.end(); it++){
		tAngle = (it->angle)*M_PI/180;
		tpos = (it->pos);
		
		/*tpos.x = 10;
		tpos.y = 10;
		tAngle = 270*M_PI/180;
		o.cen = Point2d(12, 11);
		o.name = "[12,11]";
		vobj.push_back(o);*/

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

		//2-Transform object centroids to TrayPoint coordenates
		Point2d cen_t[vobj.size()];
		oit = vobj.begin();
		mRot = (Mat_<double>(2,2)<<cos(tAngle),-sin(tAngle),sin(tAngle),cos(tAngle));//Rotation matrix
		mTra = (Mat_<double>(2,1)<<-tpos.x,-tpos.y);//Translation matrix

		for(int i = 0; oit!=vobj.end(); oit++, i++){
			//cout<<oit->name;
			mOri = (Mat_<double>(2,1)<<(oit->cen).x,(oit->cen).y); //Original Object Center
			//cout<<mOri<<mTra<<mRot<<mRes<<endl;
			mRes = mRot*(mOri+mTra); //Transformation
			cen_t[i].x=mRes.at<double>(0,0);
			cen_t[i].y=-1*mRes.at<double>(1,0);//Invert 'y' Axis
			//cout<<" "<<cen_t[i]<<endl;
		}

		//3-Do the assigment
		best = -1, dist = INT_MAX;
		for(unsigned int i=0; i<vobj.size(); i++){
			if(cen_t[i].x > 0){//Filter back points
				if(abs(atan(cen_t[i].y/cen_t[i].x)) <= hAngle){//Filter by horizontal Angle 
					if(round(cen_t[i].y) < dist){
						best = i; dist = round(abs(cen_t[i].y));
					}else if((round(abs(cen_t[i].y)) == round(dist)) and 
						(round(cen_t[i].x) < round(cen_t[best].x))) best = i;
				}
			}
		}

		if(best != -1) vobj.at(best).assigments += 1;
	}

	for(oit=vobj.begin(); oit!=vobj.end(); oit++){
		cout<<oit->name<<" "<<oit->assigments<<endl;
	}
}
