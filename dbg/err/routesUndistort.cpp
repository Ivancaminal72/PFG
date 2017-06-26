#include "opencv2/opencv.hpp"
#include <cmath>
#include "opencv/cv.h"
#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <string>
#include <boost/filesystem.hpp>

using namespace cv;
using namespace std;
namespace bf = boost::filesystem;

struct TrayPoint{
	int frame;
	Point Pos;
	float angle;
};
vector<vector<TrayPoint>> vRutas;
vector<TrayPoint> ruta;
vector<string> vCascades;
vector<string> vAngles;

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

bool loadRoutes(bf::path loadRoutesPath, Mat cameraMatrix, Mat distCoeffs, Size s){
	Mat mOri(1,1,CV_64FC2), mUnd(1,1,CV_64FC2), eye;
	int num_routes=-1;
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
					ptray.frame=(int)(*it_frame)["frame"];
					FileNode nodePoint = (*it_frame)["Punto"];
					//actPoint.x=(int) nodePoint[0];
					//actPoint.y=(int) nodePoint[1];
					//cout<<"node:"<<(double) nodePoint[0]<<endl;
					mOri.at<Vec2d> (0,0)[0] = (double) nodePoint[0];
					mOri.at<Vec2d> (0,0)[1] = (double) nodePoint[1];
					//cout<<"ori:"<<mOri.at<Vec2d> (0,0)[0]<<endl;
					cv::undistortPoints(mOri, mUnd, cameraMatrix, distCoeffs, eye, cameraMatrix);
					//cout<<"und:"<<mUnd.at<Vec2d> (0,0)[0]<<endl;
					actPoint.x = (int) mUnd.at<Vec2d> (0,0)[0];
					actPoint.y = (int) mUnd.at<Vec2d> (0,0)[1];
					//cout<<"act:"<<actPoint.x<<endl;
					ptray.Pos=actPoint;
					if(actPoint.x<0 or actPoint.x>s.width-1 or actPoint.y<0 or actPoint.y>s.height-1) continue;
					ptray.angle=(float)(*it_frame)["angle"];
					ruta.push_back(ptray);
				}
				vRutas.push_back(ruta);
				ruta.clear();
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

/*bool loadCascadeAngles(bf::path loadCasAngPath, string seq_name){
	bool withAngles = false;
	std::size_t found = loadCasAngPath.native().find("angles");
	if (found!=std::string::npos){withAngles = true;}

	cout<<"Loading ";
	if(withAngles) cout<<"Angles ";
	else cout<<"Cascades ";

	cout<<loadCasAngPath.native()<<endl;
	ifstream fd(loadCasAngPath.c_str());
	if(fd.is_open()){
		string line;
		std::size_t pos;
		while(getline(fd, line)){
			pos = line.find("/images/");
			if(pos == string::npos){
				cout<<"ERROR: Wrong image path"<<endl<<endl;
				return false;
			}
			line=line.substr(pos);
			line=seq_name+line;
			if(withAngles) vAngles.push_back(line);
			else vCascades.push_back(line);
		}
		fd.close();
		cout<<"OK!"<<endl;
		return true;	
	}else{
		cout<<"ERROR: Can't open for read"<<endl<<endl;
		return false;
	}
}*/


bool saveRoutes(bf::path saveRoutesPath){
	int num=-1;
	cout<<endl<<"Saving Routes ";
	vector<vector<TrayPoint> >::iterator itRutas, itRutasEnd;
	vector<TrayPoint>::iterator itRuta;
	FileStorage fs_routes;
	cout<<saveRoutesPath.native()<<endl;
	if(fs_routes.open(saveRoutesPath.native(), FileStorage::WRITE)){
		itRutasEnd = vRutas.end();
		for (itRutas=vRutas.begin();itRutas!=vRutas.end();itRutas++){
			num+=1;
			fs_routes<<"Ruta"+to_string(num)<<"[";
		 	for (itRuta=itRutas->begin();itRuta!=itRutas->end();itRuta++){
		 		fs_routes<<"{:"<< "Punto" << itRuta->Pos << "frame" << itRuta->frame << "angle" << itRuta->angle <<"}";
		 	}
		 	fs_routes<<"]";
		}
		fs_routes<<"ultimaRuta"<<(int) vRutas.size();
		fs_routes.release();
		cout<<"OK!"<<endl;
		return true;
	}else{
		cout<<"ERROR: Can't open for write"<<endl<<endl;
		return false;
	}
}

/*bool saveCascadeAngles(bf::path saveCasAngPath){
	bool withAngles = false;
	std::size_t found = saveCasAngPath.native().find("angles");
	if (found!=std::string::npos){withAngles = true;}

	cout<<"Saving ";
	if(withAngles) cout<<"Angles ";
	else cout<<"Cascades ";

	cout<<saveCasAngPath.native()<<endl;
	ofstream fd (saveCasAngPath.c_str(), std::ios::out | std::ios::app);
	if(fd.is_open()){
		if(withAngles){
			vector<string>::iterator ait=vAngles.begin();
			for(; ait!=vAngles.end(); ait++){
				fd<<(*ait)<<endl;
			}
		} 
		else {
			vector<string>::iterator cit=vCascades.begin();
			for(; cit!=vCascades.end(); cit++){
				fd<<(*cit)<<endl;
			}
		}
		fd.close();
		cout<<"OK!"<<endl;
		return true;
	}else{
		cout<<"ERROR: Can't open for write"<<endl<<endl;
		return false;
	}
}*/

int main( int argc, char* argv[] ) {
	string seq_name, pack_name;
	string seq_dir = "/home/ivan/videos/sequences/", pack_dir = seq_dir;
	string routes_path, angles_path, cascades_path, images_dir = "images/";
	bf::path calib_file, video_path;
	vector<Mat> video;
	VideoCapture vCapt;
	char op;
	Mat img,uimg, cameraMatrix, distCoeffs;

	if (argc < 3 || argc > 10) {//wrong arguments
		cout<<"USAGE:"<<endl<< "./routesUndistort sequence_name camera_calibration_path [pack_name] \n"; 
		cout<<"[dir_save/] [dir_packs/] [routes_path] [angles_path] [cascades_path] \n[video_path]"<<endl<<endl;
		cout<<"DEFAULT [pack_name] = sequence_name"<<endl;
		cout<<"DEFAULT [dir_save/] = "<<seq_dir<<endl;
		cout<<"DEFAULT [dir_packs/] = "<<pack_dir<<endl;
		cout<<"DEFAULT [routes_path] = dir_packs/pack_name/routes.yml"<<endl;
		//cout<<"DEFAULT [angles_path] = dir_packs/pack_name/angles.dat"<<endl;
		//cout<<"DEFAULT [cascades_path] = dir_packs/pack_name/cascades.dat"<<endl;
		cout<<"DEFAULT [video_path] = dir_packs+pack_name/pack_name.AVI"<<endl;
		return -1;
	}else{
		seq_name = argv[1];
		calib_file = argv[2];
		if(argc > 3) pack_name = argv[3];
		else pack_name = seq_name;
		if(argc > 4) seq_dir = argv[4];
		if(argc > 5) pack_dir = argv[5];
		if(argc > 6) routes_path = argv[6];
		else routes_path = pack_dir+pack_name+"/routes.yml";
		if(argc > 7) angles_path = argv[7];
		else angles_path = pack_dir+pack_name+"/angles.dat";
		if(argc > 8) cascades_path = argv[8];
		else cascades_path = pack_dir+pack_name+"/cascades.dat";
		if(argc == 10) video_path = argv[9];
		else video_path = pack_dir+pack_name+"/"+pack_name+".AVI";
		images_dir = pack_dir+pack_name+"/"+images_dir;
	}

	//Verify and create correct directories
	if(!verifyDir(seq_dir,true) || !verifyDir(pack_dir+pack_name+"/",false)) return -1;
	if(!bf::exists(routes_path)) {cout<<"Error: routes_path doesn't exist"<<endl; return -1;}
	//if(!bf::exists(angles_path)) {cout<<"Error: angles_path doesn't exist"<<endl; return -1;}
	//if(!bf::exists(cascades_path)) {cout<<"Error: cascades_path doesn't exist"<<endl; return -1;}
	if(!bf::exists(video_path)) {cout<<"Error: video_path doesn't exist"<<endl; return -1;}
	if(!bf::exists(calib_file)) {cout<<"Error: calib_file doesn't exist"<<endl; return -1;}
	//if(!verifyDir(images_dir, false)) return -1;

	//Load calibration parameters
	if(!calib_file.empty()){
	  FileStorage fs(calib_file.native(), FileStorage::READ); // Read the calibration values
	    if (!fs.isOpened()){
	        cout << "Could not open the configuration file: \"" << calib_file.native() << "\"" << endl;
	        return -1;
	    }

	    fs["Camera_Matrix"] >> cameraMatrix;
	    fs["Distortion_Coefficients"] >> distCoeffs;
	    fs.release();
	}else{
		cout<<"Error: empty camera calibration file"<<endl; 
		return -1;
	}


	vCapt.open(video_path.native());
	cout<<endl<<"Undistorting video...";
	cout.flush();
	while(vCapt.read(img)){
		undistort(img, uimg, cameraMatrix, distCoeffs);
		video.push_back(uimg.clone());
	}

	if(video.size() == 0) {cout<<"Error: empty video"<<endl; return -1;}
	vector<Mat>::iterator vit = video.begin();
	cv::Size frameSize = (*vit).size();

	if(!loadRoutes(routes_path, cameraMatrix, distCoeffs,frameSize)) {cout<<"Error: loading routes"<<endl; return -1;}
	//if(!loadCascadeAngles(angles_path,seq_name)) {cout<<"Error: loading angles"<<endl; return -1;}
	//if(!loadCascadeAngles(cascades_path,seq_name)) {cout<<"Error: loading cascades"<<endl; return -1;}


	//Save data
	bf::path save_dir = seq_dir+seq_name+"/";
	cout<<endl<<"Creating save direcotry "+save_dir.native()<<endl;
	if(bf::exists(save_dir)){
		cout<<"Already exists, do you want to overwrite it? [y/n]"<<endl;
		cin>>op;
		if(op == 'y'){
			bf::remove_all(save_dir);
		}else return 0;
	}
	if(!bf::create_directory(save_dir)){
		cout<<"ERROR: Can't create save directory"<<endl;
		return -1;
	}
	cout<<"OK!"<<endl;

	//Copy pack/images
	/*bf::path save_img_dir = save_dir.native()+"/images";
	bf::path save_img_file;
	bf::path img_dir = pack_dir+pack_name+"/images";
	bf::directory_iterator dit_end;
	if(!bf::create_directory(save_img_dir)){
		cout<<"Error creating save image directory "+save_img_dir.native()<<endl;
		return -1;
	}
	cout<<endl<<"Saving images "+img_dir.native()<<endl;
	for (bf::directory_iterator dit(img_dir); dit != dit_end; ++dit){
		save_img_file=save_img_dir;
		save_img_file/=(dit->path()).filename();
		if(bf::is_regular_file(dit->status())){
			copy_file(dit->path(),save_img_file,bf::copy_option::overwrite_if_exists);
		}
	}*/
	
	if(!saveRoutes(save_dir.native()+"routes.yml")) {cout<<"Error: saving routes"<<endl; return -1;}
	//if(!saveCascadeAngles(save_dir.native()+"angles.dat")) {cout<<"Error: saving angles"<<endl; return -1;}
	//if(!saveCascadeAngles(save_dir.native()+"cascades.dat")) {cout<<"Error: saving cascades"<<endl; return -1;}

	//Write sequence
	bf::path saveSeqPath = save_dir.native()+seq_name+".AVI";
	cout<<endl<<"Saving sequence "+saveSeqPath.native()<<endl;
	VideoWriter writer1 = VideoWriter(saveSeqPath.native(),CV_FOURCC('M','P','4','3'),25,frameSize,true);
	for(; vit!=video.end(); vit++){
		writer1.write(*vit);
	}
	cout<<"OK!"<<endl;
}
