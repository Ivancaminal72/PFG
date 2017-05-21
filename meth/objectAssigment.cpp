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
			all_masks.pop_back();
			if(mask_name.find(".png") == string::npos) return true;
			mobj = imread(masks_dir.native()+mask_name, CV_LOAD_IMAGE_GRAYSCALE);
			if(!mobj.data){cout << "Could not open or find the image" <<endl; exit(-1);}
			cout<<"OK!"<<endl;
			return true;
		}
	}
}

int main( int argc, char* argv[] ) {
	float RPM = 130.0813, persHeight = 1.6, sensorHeight = 3.07, fAngle = 124, eAngle = 7;
	cv::Size imgSize(640,480); // (5 ,4);
	string results_file;
	bf::path masks_dir = "./../omask/masks/", routes_path, save_dir = "./generatedObjectAssigments/";

	if (argc < 2 || argc > 12) {//wrong arguments
		cout<<"USAGE:"<<endl<< "./objectAssigment rotues_path [results_name] [dir_save/] [dir_obj_masks] "<<endl<<
			" [filter_angle] [person_height] [sensor_height] [relation_pixel_meter] "<<endl<<
			" [error_angle] [image_width_resolution] [image_height_resolution] "<<endl<<endl;
		cout<<"DEFAULT [results_name]  = routes_name"<<endl;
		cout<<"DEFAULT [dir_save/]     = "<<save_dir.native()<<endl;
		cout<<"DEFAULT [dir_obj_masks] = "<<masks_dir.native()<<endl;
		cout<<"DEFAULT [filter_angle]  = "<<fAngle<<endl;
		cout<<"DEFAULT [person_height] = "<<persHeight<<endl;
		cout<<"DEFAULT [sensor_height] = "<<sensorHeight<<endl;
		cout<<"DEFAULT [relation_pixel_meter] = "<<RPM<<endl;
		cout<<"DEFAULT [error_angle] = "<<eAngle<<endl;
		cout<<"DEFAULT [image_width_resolution] = "<<imgSize.width<<endl;
		cout<<"DEFAULT [image_height_resolution] = "<<imgSize.height<<endl;
		return -1;
	}else{
		routes_path = argv[1];
		if(!bf::exists(routes_path) or !routes_path.has_filename()) {cout<<"Wrong routes_path"<<endl; return -1;}
		if(argc > 2) {results_file = argv[2]; results_file+=".yml";}
		else results_file=routes_path.filename().native();
		if(argc > 3) save_dir = argv[3];
		if(argc > 4) masks_dir = argv[4];
		if(argc > 5) {fAngle = atof(argv[5]); if(fAngle >= 124 or fAngle<=0) cout<<"Error: wrong fAngle "<<fAngle<<endl; return -1;}
		if(argc > 6) persHeight = atof(argv[6]);
		if(argc > 7) sensorHeight = atof(argv[7]);
		if(argc > 8) RPM = atof(argv[8]);
		if(argc > 9) {eAngle = atof(argv[9]); if(eAngle >= 55 or eAngle<0) cout<<"Error: wrong eAngle "<<eAngle<<endl; return -1;}
		if(argc > 10) imgSize.width = atoi(argv[10]);
		if(argc == 12) imgSize.height = atoi(argv[11]);
	}

	//Verify and create correct directories
	if(!verifyDir(save_dir,true)) return -1;
	if(!verifyDir(masks_dir,false)) return -1;

	struct Obj{Point2d cen; string name; double assigments=0;};
	Obj o;
	Mat mobj, mTotal = Mat::zeros(imgSize, CV_8UC1);;
	vector<Obj> vobj;
	vector<Obj>::iterator oit;
	Moments M;
	
	/****Calculate object masks centroids****/
	while(addObjectMask(mobj, masks_dir, o.name)){
		mTotal=mTotal+mobj;

		//Find mask moments
		M = moments(mobj, true);
		//Get the mass centers
		o.cen = Point2d(M.m10/M.m00, M.m01/M.m00);
		circle(mTotal, o.cen, 1, Scalar(128)); 
		//cout<<o.cen<<endl; cout.flush();
		//cout<<o.name<<endl;
		vobj.push_back(o);
	}

	/*//Logging
	imshow("image", mTotal);
	waitKey(0);*/

	//Define variables
	persHeight *= RPM;// 3
	sensorHeight *= RPM; // 4
	fAngle *= M_PI/180/2;
	eAngle *= M_PI/180;
	fAngle += eAngle;

	//Declare variables
	int validAssigCount=0;
	Point2d tpos;
	double tAngle, sum;
	Mat mRot, mTra, mOri, mRes;
	vector<unsigned int> vindx;


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

		//3-Do the assigments
		sum = 0; 
		for(unsigned int i=0; i<vobj.size(); i++){
			if(cen_t[i].x > 0){//Filter back points
				if(abs(atan(cen_t[i].y/cen_t[i].x)) < fAngle){//Filter by binocular vision angle 
					sum += fAngle - abs(atan(cen_t[i].y/cen_t[i].x));
					vindx.push_back(i);
				}
			}
		}
		if(vindx.size() != 0) {
			for(unsigned int i=0; i<vindx.size(); i++){		
				vobj.at(vindx.at(i)).assigments += (fAngle - abs(atan(cen_t[vindx.at(i)].y/cen_t[vindx.at(i)].x)))/sum;
			}
			validAssigCount+=1;
		}
		vindx.clear();
	}

	cout<<endl<<"**************RESULTS**************"<<endl<<endl;
	char op;
	cout<<"Do you want to save result? [y/n]"<<endl;
	cin>>op;
	while(op != 'y' and op != 'n'){
		cout<<"Invalid option"<<endl;
		cin>>op;
	}
	if(op == 'y'){
		char buffer [50];
		cout<<endl<<"Saving Results "<<endl;
		FileStorage fs;
		if(fs.open(save_dir.native()+results_file, FileStorage::WRITE)){
			time_t rawtime; time(&rawtime);
	    	fs << "Date"<< asctime(localtime(&rawtime));
			fs << "RPM" << RPM;
			fs << "sensor_height" << sensorHeight;
			fs << "person_height" << persHeight;
			fs << "filter_angle" << fAngle*180/M_PI*2 - eAngle*180/M_PI;
			fs << "error_angle" << eAngle*180/M_PI;
			fs << "imgSize" << imgSize;
			fs << "number_objects" << (int) vobj.size();
			fs << "number_assigments" << (int) rutas.size();
			fs << "number_valid_assigments" << validAssigCount;
			fs << "objects" << "[";
			for(oit=vobj.begin(); oit!=vobj.end(); oit++){
				sprintf (buffer,"%.2f", oit->assigments/validAssigCount);
				fs << "{:" << "punctuation" << buffer << "center" << Point(round(oit->cen.x),(round(oit->cen.y))) 
				<< "name" << oit->name << "punctuation_f" << oit->assigments/validAssigCount << "}";
				
				cout<<buffer<<" "<<Point(round(oit->cen.x),(round(oit->cen.y)))<<" "<<oit->name<<endl;
			}
			fs << "]";
			fs.release();
			cout<<"OK!"<<endl;

		}else{
			cout<<"ERROR: Can't open for write"<<endl<<endl;
			return -1;
		}
	}
}
