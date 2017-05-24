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

#define PI 3.14159265

using namespace cv;
using namespace std;
namespace bf = boost::filesystem;

// Define our callback which we will install for mouse events.
void my_mouse_callback_box( int event, int x, int y, int flags, void* param );

void my_mouse_callback_arrow( int event, int x, int y, int flags, void* param );

CvRect box;
bool drawing_box = false;
bool drawing_arrow = false;
bool arrow_drawed = false;
bool box_drawed = false;


Mat Image, temp;
Mat croppedImage;
Mat cpIbyn;
Point ar_ori (-1,-1);
Point ar_end (-1,-1);
int nframe = -1;

struct TrayPoint{
	Point pos;
	Point pos2;
	double dist;
	float angle;
};

struct LabelProps{
	int x;
	int y;
	int width;
	int height;
	double angle;
};

// dibujar rectangulo en la imagen
void draw_box(Mat img, CvRect rect) {
	  rectangle(img, cvPoint(box.x,box.y),cvPoint(box.x+box.width,box.y+box.height), Scalar(0,255,255));

	}

//dibujar angulo de la cabeza
void draw_arrow (Mat img, Point pt1, Point pt2){
	arrowedLine(img, pt1, pt2, Scalar(0,0,255), 1, 8, 0, 0.1);
}

bool saveValues(fstream& fd, vector<TrayPoint> vt){
	cout<<"Saving...";

	if(!fd.is_open()) return false;
	
	stringstream line;
	line <<"persona"
	<<" position_x"
	<<" position_y"
	<<" position2_x"
	<<" position2_y"
	<<" angle"
	<<" dist";
	
	fd << line.str() <<endl;
	line.str("");

	//Write cascade objects
	for (uint i=0; i<vt.size(); i++){
		line <<"persona"<<i
		<<" "<<vt.at(i).pos.x
		<<" "<<vt.at(i).pos.y
		<<" "<<vt.at(i).pos2.x
		<<" "<<vt.at(i).pos2.y
		<<" "<<vt.at(i).angle
		<<" "<<vt.at(i).dist;
	}

	fd << line.str() <<endl;

	cout<<"OK!"<<endl;
	return true;
}


int main() {
	int repetitions;
	cout<<"How many repetitions?"<<endl;
	cin>>repetitions;
	if(repetitions < 2 ) {cout<<"Error: invalid repetitions"<<endl; return -1;}
	box = cvRect(-1,-1,0,0);
	string save_path = "./1ori.dat";
	string video_path = "./1ori.AVI";

	float deltax=0;
	float deltay=0;
	double theta1=0;

	fstream fd(save_path.c_str(), std::ios::out | std::ios::app);

	if (!fd.is_open()){
		cout <<"Error opening file"<<endl;
		return -1;
	}

	cout<<"abriendo video.."<<endl;

	VideoCapture videoCap(video_path);
	if(!videoCap.isOpened()){cout<<"Error: invalid video "<<video_path<<endl; return -1;}
	vector<Mat> video;
	while(videoCap.read(Image)){
		video.push_back(Image.clone());
	}

	cout.flush();
	Image = video.at(0).clone(); //Original image
	namedWindow( "image" );
	setMouseCallback("image", my_mouse_callback_box );

	bool drawingBoxes = true;
	bool drawingArrows = false;

	LabelProps obj;
	TrayPoint t;

	double tx [video.size()]; //array containing sum x component
	double ty [video.size()]; //array containing sum y component
	double ang [video.size()]; //array containing sum of angles
	double med_tx [video.size()]; //array containing median of angles
	double med_ty [video.size()]; //array containing median of angles
	double med_ang [video.size()]; //array containing median of angles
	double dev_tx [video.size()]; //array containing sum of angles
	double dev_ty [video.size()]; //array containing sum of angles
	double dev_ang [video.size()]; //array containing sum of angles

	vector<TrayPoint> vt[video.size()]; 

	uint index = 0;
	uint repet = 0;
    while(1) {

        temp = Image.clone();
		if(drawing_box) draw_box(temp, box);
		if(drawing_arrow) draw_arrow(temp,ar_ori,ar_end);
		imshow("image", temp );

		char c = waitKey(1);
		switch (c){
			case 27:{
				fd.close();
				return 0;
			}
			break;


			case 100:{ //D
				if(drawingBoxes and box_drawed){
					Image = video.at(index).clone();
					box_drawed = false;
				}else if(drawingArrows and arrow_drawed){
					Image = video.at(index).clone();
					arrow_drawed = false;
				}
			}
			break;


			case 112 : //P
				if(drawingBoxes and box_drawed){
					setMouseCallback("image",NULL,NULL);
					draw_box(Image,box);

					//añadir rectangulo
					obj.x = box.x;
					obj.y = box.y;

					obj.width = box.width;
					obj.height = box.height;

					//origen de la flecha -- centro del rectangulo
					t.pos.x = obj.x + obj.width /2;
					t.pos.y = obj.y + obj.height/2;

					tx[index] += (double) t.pos.x;
					ty[index] += (double) t.pos.y;
					vt[index].push_back(t);
					
					index +=1;
					if(index == video.size()){index = 0; repet+=1;}
					else setMouseCallback("image", my_mouse_callback_box );
					Image = video.at(index).clone();

					if(repet == (uint) repetitions) {
						setMouseCallback("image",NULL,NULL);
						drawingBoxes=false; 
						drawingArrows=true;
						repet = 0;
						setMouseCallback("image",my_mouse_callback_arrow);
						drawing_arrow=true;
						cout<<endl<<endl;
						ar_ori.x = vt[index].at(repet).pos.x;
						ar_ori.y = vt[index].at(repet).pos.y;
					}

					cout<<"Doing: "<<index<<"/"<<video.size()-1<<" index  "<<repet<<"/"<<repetitions-1<<" repet"<<endl;
				}
			break;


			
			case 110:  //n
				if(drawingArrows){
					setMouseCallback("image",NULL,NULL);
					draw_arrow(Image,ar_ori,ar_end);

					//calcular angulo y guardar el angulo
					deltax = ar_end.x - ar_ori.x;
					deltay = ar_ori.y - ar_end.y;

					if(deltax == 0){
						if(deltay>0)
							 theta1 = 90;
						else
							theta1 = 270;
					}else{
						//param = (deltay/deltax);
						theta1 = atan2(deltay,deltax) * (180.0 / PI);
					}


					if(theta1 < 0 ) theta1=360-(-1*theta1);

					//cout <<"angulo de la cabeza "<< round(theta1)<<endl;

					obj.angle = round(theta1);
					vObj.push_back(obj);

					//guardar datos para las trayectorias
					puntotray.pos = ar_ori;
					puntotray.angle = round(theta1);
					puntotray.frame = nframe;
					cout<<"frame "<<nframe<<" label "<<vObj.size()<<" ok"<<endl;
					setMouseCallback("image",NULL,NULL);
					setMouseCallback("image", my_mouse_callback_box);
					obj_done=true;
				}
			break;
		}
	}
}


void my_mouse_callback_box( int event, int x, int y, int flags, void* param )
    {

		switch(event) {
			case CV_EVENT_MOUSEMOVE:
				if(drawing_box){
					box.width = x-box.x;
					box.height = y-box.y;
				}
			break;
			case CV_EVENT_LBUTTONDOWN:
				if(!drawing_box) drawing_box = true;
				box_drawed = false;
				arrow_drawed = false;
				box = cvRect(x, y, 0, 0);
			break;
			case CV_EVENT_LBUTTONUP:
				drawing_box = false;
				if(box.width<0){
					box.x+=box.width;
					box.width *=-1;
				}
				if(box.height<0){
					box.y+=box.height;
					box.height*=-1;
				}

				box_drawed=true;
				draw_box(Image,box);
			break;
		}
	}

void my_mouse_callback_arrow( int event, int x, int y, int flags, void* param ){

		switch(event) {
			case CV_EVENT_MOUSEMOVE:
				if(drawing_arrow){
					ar_end.x=x;
					ar_end.y=y;
				}
			break;
			case CV_EVENT_LBUTTONDOWN:
				if(!drawing_arrow) drawing_arrow=true;
				ar_end.x=x;
				ar_end.y=y;
			break;
			case CV_EVENT_LBUTTONUP:
				drawing_arrow=false;
				ar_end.x=x;
				ar_end.y=y;
				draw_arrow(Image,ar_ori,ar_end);
				arrow_drawed=true;
			break;
		}
	}
