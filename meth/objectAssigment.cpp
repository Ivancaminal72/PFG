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

int main( int argc, char* argv[] ) {
	string results_file;
	bf::path routes_path, save_dir = "./generatedObjectAssigments/";

	if (argc < 2 || argc > 4) {//wrong arguments
		cout<<"USAGE:"<<endl<< "./objectAssigment rotues_path [results_name] [dir_save/]"<<endl<<endl;
		cout<<"DEFAULT [results_name] = routes_name"<<endl;
		cout<<"DEFAULT [dir_save/] = "<<save_dir<<endl;
		return -1;
	}else{
		routes_path = argv[1];
		if(!bf::exists(routes_path) or !routes_path.has_filename()) {cout<<"Wrong routes_path"<<endl; return -1;}
		if(argc > 2) {results_file = argv[2]; results_file+=".yml";}
		else results_file=routes_path.filename().native();
		if(argc > 3) save_dir = argv[3];
	}

	//Verify and create correct directories
	if(!verifyDir(save_dir,true)) return -1;

	cv::Size imgSize;
	imgSize.width = 640; //640 5
	imgSize.height = 480; //480 4

	//Declare variables
	Point2d tpos;

	/*loadRoutes(routes_path);
	vector<TrayPoint>::iterator it=rutas.begin();
	for(; it<rutas.end(); it++){
		//Point prespective correction
		cout<<(it->pos).x<<endl;
	}*/

	//0-Prove tpos is inside the image bounds
	if(tpos.x<0 or tpos.x>imgSize.width-1 or tpos.y<0 or tpos.y>imgSize.height-1){
		cout<<"Error tpos is out of image bounds"<<endl;
		return -1;
	}
	cout<<"it->pos: "<<tpos<<endl;
	//1-Correction of tpos real position
	tpos-=persHeight/sensorHeight*tpos;
}
