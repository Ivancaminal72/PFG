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
	int horAngle = 20;
	cv::Size imageSize;
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
		routes_path = "/home/ivan/videos/sequences/pack3/routes.yml"
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

	imageSize.width = 640;
	imageSize.height = 480;

	/*loadRoutes(routes_path);
	vector<TrayPoint>::iterator it=rutas.begin();
	for(; it<rutas.end(); it++){
		//Point prespective correction
		cout<<(it->pos).x<<endl;
	}*/

}
