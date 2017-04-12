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
	Point Pos;
	float angle;
};
vector<vector<TrayPoint>> vRutas;
vector<TrayPoint> ruta;
vector<string> vCascades;
vector<string> vAngles;
int offset_frames = 0;
vector<int> videoFrames;

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
					ptray.frame=(int)(*it_frame)["frame"]+offset_frames;
					FileNode nodePoint = (*it_frame)["Punto"];
					actPoint.x=(int) nodePoint[0];
					actPoint.y=(int) nodePoint[1];
					ptray.Pos=actPoint;
					ptray.angle=(float)(*it_frame)["angle"];
					ruta.push_back(ptray);
				}
				vRutas.push_back(ruta);
				ruta.clear();
			}
		}
		offset_frames+=videoFrames.at(videoFrames.size()-1);
		fs_routes.release();
		cout<<"OK!"<<endl;
		return true;
	}else{
		cout<<"ERROR: Can't open for read"<<endl<<endl;
		return false;
	}
}

bool loadCascadeAngles(bf::path loadCasAngPath, string seq_name){
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
}


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
		fs_routes.release();
		cout<<"OK!"<<endl;
		return true;
	}else{
		cout<<"ERROR: Can't open for write"<<endl<<endl;
		return false;
	}
}

bool saveCascadeAngles(bf::path saveCasAngPath){
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
}

int main( int argc, char* argv[] ) {
	string seq_name, vid_file, pack_name, vid_name;
	string vid_dir = "../../videos/", seq_dir = "/home/ivan/videos/sequences/", pack_dir = "./";
	vector<Mat> video;
	VideoCapture vCapt;
	char op;
	Mat img;

	if (argc < 2 || argc > 5) {//wrong arguments
		cout<<"USAGE:"<<endl<< "AddVideos sequence_name [pack_name] [dir_save/] [dir_videos/] [dir_packs/]"<<endl<<endl;
		cout<<"DEFAULT [pack_name] = sequence_name"<<endl;
		cout<<"DEFAULT [dir_save/] = "<<seq_dir<<endl;
		cout<<"DEFAULT [dir_videos/] = "<<vid_dir<<endl;
		cout<<"DEFAULT [dir_packs/] = "<<pack_dir<<endl;
		return -1;
	}else{
		seq_name = argv[1];
		pack_name = seq_name;
		if(argc > 2) pack_name = argv[2];
		if(argc > 3) seq_dir = argv[3];
		if(argc > 4) vid_dir = argv[4];
		if(argc == 6) pack_dir = argv[5];
	}

	//Verify and create correct directories
	if(!verifyDir(vid_dir,false) || !verifyDir(seq_dir,true) || !verifyDir(pack_dir+pack_name,false)) return -1;
	if(!verifyDir(pack_dir+pack_name+"/routes/",false)) return -1;
	if(!verifyDir(pack_dir+pack_name+"/angles/", false)) return -1;
	if(!verifyDir(pack_dir+pack_name+"/cascades/", false)) return -1;
	if(!verifyDir(pack_dir+pack_name+"/images/", false)) return -1;

	while(1){
		cout<<endl<<endl<<endl;
		cout<<"Press 'a' to add new video"<<endl;
		cout<<"Press 'q' to quit and save"<<endl;
		cin>>op;
		switch (op){
			case 'q'://Q (quit and save)
				if(video.size() == 0) return -1;
				cout<<"Are you sure you want to quit and save video?  [y/n]"<<endl;
				cin>>op;
				if(op == 'y'){
					vector<Mat>::iterator vit = video.begin();
					cv::Size frameSize = (*vit).size();
					bf::path save_dir = seq_dir+seq_name+"/";
					cout<<endl<<"Creating save direcotry "+save_dir.native()<<endl;
					if(bf::exists(save_dir)){
						cout<<"Already exists, do you want to overwrite it? [y/n]"<<endl;
						cin>>op;
						if(op == 'y'){
							bf::remove_all(save_dir);
						}else break;
					}
					if(!bf::create_directory(save_dir)){
						cout<<"ERROR: Can't create save directory"<<endl;
						return -1;
					}
					cout<<"OK!"<<endl;
					//Copy pack/images
					bf::path save_img_dir = save_dir.native()+"/images";
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
					}
					
					//Save Routes
					if(!saveRoutes(save_dir.native()+"routes.yml")) break;
				
					//Save Angles
					if(!saveCascadeAngles(save_dir.native()+"angles.dat")) break;
				
					//Save Cascades
					if(!saveCascadeAngles(save_dir.native()+"cascades.dat")) break;

					//Write sequence
					bf::path saveSeqPath = save_dir.native()+seq_name+".AVI";
					cout<<endl<<"Saving sequence "+saveSeqPath.native()<<endl;
					VideoWriter writer1 = VideoWriter(saveSeqPath.native(),CV_FOURCC('M','P','4','3'),25,frameSize,true);
					for(; vit!=video.end(); vit++){
						writer1.write(*vit);
					}
					cout<<"OK!"<<endl;
					return 0;
				}
			break;
			

			case 'a'://A (add new video)
				vid_name="";
				vid_file="";
				cout<<"Introduce the name of the videoFile you want to add"<<endl;
				cin.clear();
				cin.ignore();
				getline(cin,vid_file);
				if(vCapt.isOpened()) vCapt.release();
				size_t found = vid_file.find(".AVI");
				if(found == string::npos){
					cout<<"Wrong video file"<<endl;
					break;
				}else{
					vCapt.open(vid_dir+vid_file);
					videoFrames.push_back(vCapt.get(CV_CAP_PROP_FRAME_COUNT));
					vid_name=vid_file.substr(0,found);
				}
				if(!loadRoutes(pack_dir+pack_name+"/routes/"+vid_name+".yml")) break;
				if(!loadCascadeAngles(pack_dir+pack_name+"/angles/"+vid_name+".dat",seq_name)) break;
				if(!loadCascadeAngles(pack_dir+pack_name+"/cascades/"+vid_name+".dat",seq_name)) break;
				while(vCapt.read(img)){
					video.push_back(img.clone());
				}
			break;
		}
	}
}
