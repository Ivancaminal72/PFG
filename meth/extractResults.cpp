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
	cv::Size imgSize(640,480);
	string results_file, matrix_name;
	bf::path matrix_path, save_dir = "./generatedVisualAreas/",masks_dir = "./../omask/masks/";

	if (argc < 2 || argc > 8) {
		cout<<"USAGE:"<<endl<< "./extractAreas matrix_path results_name matrix_name [dir_obj_masks] " <<endl 
		<<"[dir_save/] [image_width_resolution] [image_height_resolution] "<<endl<<endl;

		cout<<"DEFAULT [dir_obj_masks] = "<<masks_dir.native()<<endl;
		cout<<"DEFAULT [dir_save/] = "<<save_dir.native()<<endl;
		cout<<"DEFAULT [image_width_resolution] = "<<imgSize.width<<endl;
		cout<<"DEFAULT [image_height_resolution] = "<<imgSize.height<<endl;
		return -1;
		
	}else{
		matrix_path = argv[1];
		if(!bf::exists(matrix_path) or !matrix_path.has_filename()) {cout<<"Wrong matrix_path"<<endl; return -1;}
		results_file = argv[2]; results_file+=".yml";
		matrix_name = argv[3];
		if(argc > 4) masks_dir = argv[4];
		if(argc > 5) save_dir = argv[5];
		if(argc > 6) imgSize.width = atoi(argv[6]);
		if(argc == 8) imgSize.height = atoi(argv[7]);
	}

	//Verify and create correct directories
	if(!verifyDir(save_dir,true)) return -1;

	/****Obtain object masks****/
	struct Obj{Mat mask; string name; double punctuation; double punctuation2;};
	Obj o;
	vector<Obj> vobj;
	vector<Obj>::iterator oit;
	Mat mobj, mTotal = Mat::zeros(imgSize, CV_8UC1);
	while(addObjectMask(mobj, masks_dir, o.name)){
		if(mobj.size() != imgSize){cout<<"Error: routes image size is different from "<<o.name<<" mask size"<<endl; return -1;}
		mTotal+=mobj;
		o.mask=mobj.clone();
		
		//cout<<o.name<<endl;
		vobj.push_back(o);
	}
	/*//Logging
	imshow("image", mTotal);
	waitKey(0);*/

	//Load matrix
  	FileStorage fs(matrix_path.native(), FileStorage::READ); 
    if (!fs.isOpened()){
        cout << "Could not open file: \"" << matrix_path.native() << "\"" << endl;
        return -1;
    }

    fs[matrix_name] >> mTotal;
    fs.release();

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
			fs << "imgSize" << imgSize;
			fs << "number_objects" << (int) vobj.size();
			fs << "objects" << "[";
			for(oit=vobj.begin(); oit!=vobj.end(); oit++){
				mobj = Mat::zeros(imgSize, CV_64F);
				mTotal.copyTo(mobj,oit->mask);
				oit->punctuation = sum(mobj)[0];
				oit->punctuation2 = (double) sum(mobj)[0]/ (double) countNonZero(mobj);
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
			//fs << "mTotalFloor"<<mTotal;
			fs.release();
			cout<<"OK!"<<endl;
		}else{
			cout<<"ERROR: Can't open for write"<<endl<<endl;
			return -1;
		}
	}
}

