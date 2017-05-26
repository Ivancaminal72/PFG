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
	int frame=-1;
	Point pos;
	float angle;
};

// dibujar rectangulo en la imagen
void draw_box(Mat& img, Rect rect, Scalar color) {
	  rectangle(img, rect, color);

}

//dibujar angulo de la cabeza
void draw_arrow (Mat& img, Point pt1, Point pt2, Scalar color){
	arrowedLine(img, pt1, pt2, color, 1, 8, 0, 0.1);
}

bool loadRotesAndDrawArrows(bf::path loadRoutesPath, vector<Mat>& video, Scalar color, float arrowLength, vector<TrayPoint>& vRoutes, vector<Rect> vrecs){
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
			uint count=0;
			TrayPoint t;
			Point arrPoint;
			vector<TrayPoint> vinit (video.size(), t);
			vRoutes = vinit;
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
					t.frame = (*it_frame)["frame"]; 
					FileNode nodePoint = (*it_frame)["Punto"];
					t.pos.x=(int) nodePoint[0];
					t.pos.y=(int) nodePoint[1];
					t.angle=(float)(*it_frame)["angle"];
					if(t.angle == 90){
						arrPoint.x=t.pos.x;
						arrPoint.y=t.pos.y - arrowLength;
					}else if(t.angle == 270){
						arrPoint.x=t.pos.x;
						arrPoint.y=t.pos.y + arrowLength;
					}else if(t.angle < 90 and t.angle > 270){
						arrPoint.x=t.pos.x+round(arrowLength*cos(t.angle*M_PI/180));
						arrPoint.y=t.pos.y-round(arrowLength*sin(t.angle*M_PI/180));
					}else{
						arrPoint.x=t.pos.x+round(arrowLength*cos(t.angle*M_PI/180));
						arrPoint.y=t.pos.y-round(arrowLength*sin(t.angle*M_PI/180));
					}
					if((uint) t.frame < video.size() and !video.empty()){
						draw_arrow(video.at(t.frame), t.pos, arrPoint, color);

						if(count < vrecs.size() and !vrecs.empty()) {
							draw_box(video.at(t.frame), vrecs.at(count), color);
						}
						count+=1;
						vRoutes.at(t.frame)=t;
					}else{cout<<"Error: negative or out of video length frame"<<endl; return false;}
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

bool loadCascadeAngles(bf::path loadCasAngPath, vector<Mat>& video, vector<Rect>& vrecs){
	bool withAngles = false;
	std::size_t found = loadCasAngPath.native().find("angles");
	if (found!=std::string::npos){withAngles = true;}

	cout<<"Loading ";
	if(withAngles) cout<<"Angles ";
	else cout<<"Cascades ";

	cout<<loadCasAngPath.native()<<endl;
	ifstream fd(loadCasAngPath.c_str());
	if(fd.is_open()){
		stringstream line;
		string param;
		int x, y, w, h;
		while(getline(fd, param)){
			line.str(param);
			line>>param;
			found = param.find("/images/");
			if (found == std::string::npos){cout<<"Error: image path is not correct"<<endl; return false;}
			else {
				line>>param;
				line>>param; x=stoi(param);
				line>>param; y=stoi(param);
				line>>param; w=stoi(param);
				line>>param; h=stoi(param);
				vrecs.push_back(Rect(x,y,w,h));
			}
			line.clear();
		}
		fd.close();
		cout<<"OK!"<<endl;
		return true;	
	}else{
		cout<<"ERROR: Can't open for read"<<endl<<endl;
		return false;
	}
}

int main( int argc, char* argv[] ) {
	Mat Image;
	char c;
	vector<TrayPoint> vR1, vR2;
	bf::path video_path, routes1_path, routes2_path, cascades1_path, cascades2_path;
	if (argc < 2 || argc > 6) {
		cout<<"USAGE:"<<endl<< "./seeVideos video_path_to_read [routes1_path] [cascades1_path] [routes2_path] [cascades2_path]"<<endl;
		return -1;
	}else{
		video_path = argv[1];
		if(argc > 2) routes1_path = argv[2];
		if(argc > 3) cascades1_path = argv[3];
		if(argc > 4) routes2_path = argv[4];
		if(argc == 6) cascades2_path = argv[5]; 
	}

	VideoCapture videoCap(video_path.native());
	if(!videoCap.isOpened()){cout<<"Error: invalid video "<<video_path<<endl; return -1;}
	vector<Mat> video;
	while(videoCap.read(Image)){
		video.push_back(Image.clone());
	}

	vector<Rect> vrecs1,vrecs2;
	//load cascades
	if(bf::exists(cascades1_path)){
		if(!loadCascadeAngles(cascades1_path, video, vrecs1)) return -1;
	}
	//load de routes1 and draw arrow
	if(bf::exists(routes1_path)){
		if(!loadRotesAndDrawArrows(routes1_path, video, Scalar(0,0,255), 80, vR1, vrecs1)) return -1;
	}


	if(bf::exists(cascades2_path)){
		if(!loadCascadeAngles(cascades2_path, video, vrecs2)) return -1;
	}
	//load de routes2 and draw arrow
	if(bf::exists(routes2_path)){
		if(!loadRotesAndDrawArrows(routes2_path, video, Scalar(0,255,255), 80, vR2, vrecs2)) return -1;
		double err_x=0, err_y=0, err_a=0;
		int count = 0;
		vector<TrayPoint>::iterator itr1 = vR1.begin();
		vector<TrayPoint>::iterator itr2 = vR2.begin();
		for(; itr1 != vR1.end() or itr2 != vR2.end(); itr1++, itr2++){
			if((*itr1).frame == -1 or (*itr2).frame == -1) continue;
			else{
				count+=1;
				err_x+=abs((*itr1).pos.x - (*itr2).pos.x);
				err_y+=abs((*itr1).pos.y - (*itr2).pos.y);
				err_a+=(180.0 - abs(fmod(abs((*itr1).angle - (*itr2).angle), 360.0) - 180.0));
			}
		}
		err_x=err_x/count;
		err_y=err_y/count;
		err_a=err_a/count;
		
		cout<<"Average distances: "<<endl;
		printf ("Point_x: %g \n", err_x);
		printf ("Point_y: %g \n", err_y);
		printf ("Angles: %g \n", err_a);

	}

	vector<Mat>::iterator it=video.begin();
	//vector<Mat> videoMix;
	int frame = 0;
	for(;;){
		imshow("image", (*it));
		c = waitKey(0);
		switch (c){
			case 27:{// esc
				/*if(videoMix.size()!=0){
					vector<Mat>::iterator vit = videoMix.begin();
					cv::Size frameSize = (*it).size();
					bf::path save_path = "./mix/2und.AVI";
					cout<<endl<<"Saving sequence "+save_path.native()<<endl;
					VideoWriter writer1 = VideoWriter(save_path.native(),CV_FOURCC('M','P','4','3'),25,frameSize,true);
					for(; vit!=videoMix.end(); vit++){
						writer1.write(*vit);
					}
					cout<<"OK!"<<endl;
				}*/
				return 0;
				}	
			break;

			case '+':{
				if(it!=video.end()--) {
					it++;
					frame+=1;
					cout<<frame<<" frame"<<endl;
				}
			}
			break;

			case '-':{
				if(it!=video.begin()) {
					it--;
					frame-=1;
					cout<<frame<<" frame"<<endl;
				}
			}
			break;

			/*case 'a':{
				videoMix.push_back((*it).clone());
				cout<<videoMix.size()<<" images"<<endl;
			}
			break;

			case 'd':{
				videoMix.pop_back();
				cout<<videoMix.size()<<" images"<<endl;
			}
			break;*/
		}
	}
}