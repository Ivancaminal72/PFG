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
Point ar_ori (-1,-1);
Point ar_end (-1,-1);
int nframe = -1;

struct TrayPoint{
	Point pos;
	Point pos2;
	double dist;
	float angle;
};

// dibujar rectangulo en la imagen
void draw_box(Mat img, CvRect rect) {
	  rectangle(img, cvPoint(box.x,box.y),cvPoint(box.x+box.width,box.y+box.height), Scalar(0,255,255));

	}

//dibujar angulo de la cabeza
void draw_arrow (Mat img, Point pt1, Point pt2){
	arrowedLine(img, pt1, pt2, Scalar(0,0,255), 1, 8, 0, 0.1);
}

int main() {
	int repetitions;
	string save_path,video_path;
	cout<<"How many repetitions?"<<endl;
	cin>>repetitions;
	if(repetitions < 2 ) {cout<<"Error: invalid repetitions"<<endl; return -1;}

	cout<<"Video name?"<<endl;
	cin>>video_path;
	save_path = "./"+video_path+".csv";
	video_path = "./"+video_path+".AVI";

	box = cvRect(-1,-1,0,0);
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

	TrayPoint t;

	double tx [video.size()], ty [video.size()], dist [video.size()], ang [video.size()]; //arrays containing sum
	double med_tx [video.size()], med_ty [video.size()], med_dist [video.size()], med_ang [video.size()]; //arrays containing median
	double dev_tx [video.size()], dev_ty [video.size()], dev_dist [video.size()], dev_ang [video.size()]; //arrays containing deviation  
	double theta1=0, tdev_tx=0, tdev_ty=0, tdev_dist=0, tmed_dist=0, tdev_ang=0;
	float deltax=0, deltay=0;

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
			case 27: 
				return 0;
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

					//origen de la flecha -- centro del rectangulo
					t.pos.x = (int) box.x + (int) box.width /2;
					t.pos.y = (int) box.y + (int) box.height/2;

					tx[index] += (double) t.pos.x;
					ty[index] += (double) t.pos.y;
					vt[index].push_back(t);
					
					index +=1;
					if(index == video.size()){index = 0; repet+=1;}
					Image = video.at(index).clone();
					box_drawed=false;
					if(repet == (uint) repetitions) {
						drawingBoxes=false; 
						drawingArrows=true;
						repet = 0;
						drawing_arrow=true;
						setMouseCallback("image",my_mouse_callback_arrow);
						cout<<endl<<endl;
						ar_ori.x = vt[index].at(repet).pos.x;
						ar_ori.y = vt[index].at(repet).pos.y;
						for(unsigned i=0; i<video.size(); i++){
							med_ty[i]=ty[i]/repetitions;
							med_tx[i]=tx[i]/repetitions;
						}
					}else setMouseCallback("image", my_mouse_callback_box );

					cout<<"Doing: "<<index<<"/"<<video.size()-1<<" index  "<<repet<<"/"<<repetitions-1<<" repet"<<endl;
				}
			break;


			
			case 110:  //n
				if(drawingArrows and arrow_drawed){
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

					ang[index] += round(theta1);
					dist[index] += sqrt(pow((double)vt[index].at(repet).pos2.x-(double)vt[index].at(repet).pos.x,2)
						+pow((double)vt[index].at(repet).pos2.y-(double)vt[index].at(repet).pos.y,2));

					vt[index].at(repet).pos2.x = ar_end.x;
					vt[index].at(repet).pos2.y = ar_end.y;
					vt[index].at(repet).angle = round(theta1);
					vt[index].at(repet).dist = sqrt(pow((double)vt[index].at(repet).pos2.x-(double)vt[index].at(repet).pos.x,2)
						+pow((double)vt[index].at(repet).pos2.y-(double)vt[index].at(repet).pos.y,2));

					index +=1;
					if(index == video.size()){index = 0; repet+=1;}
					Image = video.at(index).clone();
					arrow_drawed=false;
					if(repet == (uint) repetitions) { //SAVE RESULTS
						fstream fd(save_path.c_str(), std::ios::out | std::ios::trunc);
						if (!fd.is_open()){cout <<"Error opening file"<<endl; return -1;}
						cout<<endl<<endl;
						stringstream line;
						line <<"personX;"
						<<"position_x;"
						<<"position_y;"
						<<"position2_x;"
						<<"position2_y;"
						<<"angles;"
						<<"distance;";
						fd << line.str() <<endl;
						line.str("");
						for(unsigned i=0; i<video.size(); i++){
							for (unsigned j=0; j<(uint)repetitions; j++){
								line <<"person"<<i
								<<";"<<vt[i].at(j).pos.x
								<<";"<<vt[i].at(j).pos.y
								<<";"<<vt[i].at(j).pos2.x
								<<";"<<vt[i].at(j).pos2.y
								<<";"<<vt[i].at(j).angle
								<<";"<<vt[i].at(j).dist;
								fd << line.str() <<";"<<endl;
								line.str("");
							}
							med_ang[i]=ang[i]/repetitions;
							med_dist[i]=dist[i]/repetitions;
						}
						for(unsigned i=0; i<video.size(); i++){
							for(unsigned j=0; j<(uint)repetitions; j++){
								dev_ty[i]+=pow(vt[i].at(j).pos.y-med_ty[i],2);
								dev_tx[i]+=pow(vt[i].at(j).pos.x-med_tx[i],2);
								dev_ang[i]+=pow(vt[i].at(j).angle-med_ang[i],2);
								dev_dist[i]+=pow(vt[i].at(j).dist-med_dist[i],2);
							}
						}
						fd<<endl;
						line <<"personX;"
						<<"deviation_x;"
						<<"deviation_y;"
						<<"deviation_ang;"
						<<"deviation_dist;"
						<<"median_dist;";
						fd << line.str() <<endl;
						line.str("");
						for(unsigned i=0; i<video.size(); i++){
							dev_ty[i]=sqrt(dev_ty[i]/repetitions);
							dev_tx[i]=sqrt(dev_tx[i]/repetitions);
							dev_ang[i]=sqrt(dev_ang[i]/repetitions);
							dev_dist[i]=sqrt(dev_dist[i]/repetitions);
							line <<"person"<<i
							<<";"<<dev_ty[i]
							<<";"<<dev_tx[i]
							<<";"<<dev_ang[i]
							<<";"<<dev_dist[i]
							<<";"<<med_dist[i];
							fd << line.str() <<";"<<endl;
							line.str("");
							tdev_ty += dev_ty[i];
							tdev_tx += dev_tx[i];
							tdev_ang += dev_ang[i];
							tdev_dist += dev_dist[i];
							tmed_dist += med_dist[i];
						}
						tdev_ty = tdev_ty/video.size();
						line <<"TOTAL"
						<<";"<<tdev_ty/video.size()
						<<";"<<tdev_tx/video.size()
						<<";"<<tdev_ang/video.size()
						<<";"<<tdev_dist/video.size()
						<<";"<<tmed_dist/video.size();
						fd << line.str() <<";"<<endl;
						fd.close();
						return 0;

					}else{
						drawing_arrow=true;
						cout<<"Doing: "<<index<<"/"<<video.size()-1<<" index  "<<repet<<"/"<<repetitions-1<<" repet"<<endl;
						setMouseCallback("image",my_mouse_callback_arrow);
						ar_ori.x = med_tx[index];
						ar_ori.y = med_ty[index];
					}
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
