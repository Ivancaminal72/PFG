/*
 * Pgrogram to generate image databases
 *
 *
*/

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


Mat Image;
Mat ImageObj;
Mat temp;
Mat croppedImage;
Mat OriImage;
Mat cpIbyn;
Point ar_ori (-1,-1);
Point ar_end (-1,-1);

struct TrayPoint{
	int frame;
	Point Pos;
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

int lastLabeledFrame(string case_dir, string video_name){
	int num_routes=-1;
	int llf=-1;
	string routes_path = case_dir+"routes/"+video_name+".yml";
	FileStorage fs_routes;
	if(fs_routes.open(routes_path, FileStorage::READ)){
		FileNode rootNode = fs_routes.root(0);
		if(rootNode.empty()){
			fs_routes.release();
			return llf;
		}else{
			fs_routes["ultimaRuta"]>>num_routes;
			FileNode frames = fs_routes["Ruta"+to_string(num_routes)];
			FileNodeIterator it_last = frames.end();
			it_last--;
			llf = (int)(*it_last)["frame"];
			fs_routes.release();
			return llf;
		}
	}else{
		fs_routes.release();
		return llf;
	}
}

//guardar fichero de trayectorias
bool saveRoutes(vector<vector<TrayPoint>> rutas, string case_dir, string video_name){
    vector<vector<TrayPoint>> old_rutas;
    vector<TrayPoint> old_ruta;
    string routes_path = case_dir+"routes/"+video_name+".yml";
    vector<vector<TrayPoint> >::iterator itRutas, itOldRutas, itOldRutasEnd;
    vector<TrayPoint>::iterator itRuta, itOldRuta;

	// escritura general de trayecto en disco
	bool isNewVideo = true;
	int num_routes=-1;
	int num=-1;
	FileStorage fs_routes;
	if(fs_routes.open(routes_path, FileStorage::READ)){
		FileNode rootNode = fs_routes.root(0);
		if(rootNode.empty()){
			fs_routes.open(routes_path, FileStorage::WRITE);
		}else{
			isNewVideo=false;
			TrayPoint t;
			Point p;
			if(fs_routes["ultimaRuta"].isNone()){
				cout<<"Error de lectura fitxer routes"<<endl;
				return false;
			}
			fs_routes["ultimaRuta"]>>num_routes;
			for(int i=0; i<num_routes+1; i++){
				FileNode ruta = fs_routes["Ruta"+to_string(i)];
				FileNodeIterator it_frame = ruta.begin(); 
				for(;it_frame != ruta.end(); it_frame++){
					t.frame=(int)(*it_frame)["frame"];
					FileNode nodePoint = (*it_frame)["Punto"];
					p.x=(int) nodePoint[0];
					p.y=(int) nodePoint[1];
					t.Pos=p;
					t.angle=(float)(*it_frame)["angle"];
					old_ruta.push_back(t);
				}
				old_rutas.push_back(old_ruta);
				old_ruta.clear();
			}
			FileNode frames = fs_routes["Ruta"+to_string(num_routes)];
			FileNodeIterator it_last = frames.end();
			it_last--;

			//addVideos

			/*offset_frames = (int)(*it_last)["frame"];
			offset_frames+=1;*/
			

			fs_routes.open(routes_path, FileStorage::WRITE);
			itOldRutasEnd = old_rutas.end();
			itOldRutasEnd--; //move one position left
			for (itOldRutas=old_rutas.begin();itOldRutas!=old_rutas.end();itOldRutas++){
				num+=1;
				fs_routes<<"Ruta"+to_string(num)<<"[";
			 	for (itOldRuta=itOldRutas->begin();itOldRuta!=itOldRutas->end();itOldRuta++){
			 		fs_routes<<"{:"<< "Punto" << itOldRuta->Pos << "frame" << itOldRuta->frame << "angle" << itOldRuta->angle <<"}";
			 	}
			 	if(itOldRutas!=itOldRutasEnd) fs_routes<<"]"; //This prevents clossing the Route in the last Frame of the last route
			}
		}

	}else{
		fs_routes.open(routes_path, FileStorage::WRITE);
	}
	num = num_routes;
	for (itRutas=rutas.begin();itRutas!=rutas.end();itRutas++){
		num+=1;
		if(isNewVideo || num>num_routes+1) {fs_routes<<"Ruta"+to_string(num)<<"[";}
	 	for (itRuta=itRutas->begin();itRuta!=itRutas->end();itRuta++){
	 		fs_routes<<"{:"<< "Punto" << itRuta->Pos << "frame" << itRuta->frame << "angle" << itRuta->angle <<"}";
	 	}
	 	fs_routes<<"]";
	}
	if(!isNewVideo) num-=1;		
	fs_routes<<"ultimaRuta"<<num;
	fs_routes.release();
	return true;
}

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



int main( int argc, char* argv[] ) {

	if (argc != 3) {
		//cout<<"USAGE:"<<endl<< "DataBaseGenerator [video_to_read] [directory/to/save/images] [route/to/cascade_angles.dat] [route/to/Cascade.dat] [fichero/trayectorias.yml]"<<endl;
		cout<<"USAGE:"<<endl<< "DataBaseGenerator [video_path_to_read] [directory/case]"<<endl;
		return -1;
	}

	box = cvRect(-1,-1,0,0);
	string video_path = argv[1];
	string case_dir = argv[2];
	size_t found = video_path.find_last_of("/");
	string video_name = video_path.substr(found+1);
	found = video_name.find(".AVI");

	if(found == string::npos){
		cout<<"Wrong video path"<<endl;
		return -1;
	}else{
		video_name=video_name.substr(0,found);
	}

	bf::path images_dir = case_dir+"images/";
	bf::path angles_dir = case_dir+"angles/";
	bf::path cascades_dir = case_dir+"cascades/";
	bf::path routes_dir = case_dir+"routes/";
	if(!verifyDir(images_dir, true)) return -1;
	if(!verifyDir(angles_dir, true)) return -1;
	if(!verifyDir(cascades_dir, true)) return -1;
	if(!verifyDir(routes_dir, true)) return -1;
	bf::path angles_path = angles_dir.native()+video_name+".dat";
	bf::path cascades_path = cascades_dir.native()+video_name+".dat";

	cout<<endl<<"Press P to save rectangle"<<endl<<"Press D to clear a wrong box"<<endl;
	cout<<"Press N to save arrow"<<endl;
	cout<<"Press Q to reset current objects"<<endl;
	cout<<"Press R to add point to a route"<<endl;
	cout<<"Press T to create a new route (not first time)"<<endl;
	cout<<"Press SPACE to next image"<<endl<<"Press ESC to save and exit"<<endl;


	vector<LabelProps> vObj;

	//inicializacion trayectorias
	vector <vector < TrayPoint> > VTrayectorias;
	vector <TrayPoint> VTrayectoria;

	TrayPoint puntotray;
	puntotray.Pos = Point(-1,-1);
	puntotray.frame = -1;
	puntotray.angle = 0.0;

	float deltax=0;
	float deltay=0;
	double theta1=0;

	/*
	//contar imagenes del directorio de imagenes
	int cimagesp = 0;
	//Con un puntero a DIR abriremos el directorio
	DIR* dp = opendir(images_dir.c_str());

	// en *ent habrá información sobre el archivo que se está "sacando" a cada momento
	struct dirent *ent;

	 if (dp == NULL){
		cout<<"No puedo abrir el directorio"<<endl;;
		return -1;
	 }

	 while ((ent = readdir (dp)) != NULL)
		 {
		   //Nos devolverá el directorio actual (.) y el anterior (..), como hace ls
		   if ( (strcmp(ent->d_name, ".")!=0) && (strcmp(ent->d_name, "..")!=0) )
		 {
		   cimagesp++;
		 }
		}

	 closedir (dp);

	//cout<< "numero de imagenes en el directorio "<< cimagesp <<endl;*/

	fstream fd_angles(angles_path.c_str(), std::ios::out | std::ios::app);
	fstream fd_cascades(cascades_path.c_str(), std::ios::out | std::ios::app);


	if (!fd_angles.is_open()){
		cout <<"no se ha podido abrir el archivo de datos 1"<<endl;
		return -1;
	}
	if(!fd_cascades.is_open()){
		cout <<"no se ha podido abrir el archivo de datos 2"<<endl;
		return -1;
	}

	cout<<"abriendo video.."<<endl;

	VideoCapture video(video_path);

	//avançament de frames
	int nframe = -1;
	int llf =lastLabeledFrame(case_dir, video_name);
	if(llf < video.get(CV_CAP_PROP_FRAME_COUNT)){
		for(; nframe<llf+1; nframe++){
			if(!(video.read(Image))){
				cout<<"Error lectura video: "+video_name<<endl;
				return -1;
			}
			if(nframe>=0) cout<<"frame "<<nframe<<" DONE"<<endl;
		}
	}

    ImageObj = Image.clone(); //Imagen con el ultimo rectangulo dibujado
    OriImage = Image.clone(); //Imagen original
	temp = ImageObj.clone(); //Imagen con los rectangulos temporales y finales

	namedWindow( "image" );

	setMouseCallback("image", my_mouse_callback_box );

	stringstream ruta;

    cout<<"inicio del programa"<<endl;
	bool point_saved = false;
	bool press_space = false;
	bool obj_done = false;
	bool obj_start = true;

	LabelProps obj;

    while(1) {

        temp = Image.clone();

		if(drawing_box) draw_box(temp, box);

		if(drawing_arrow) draw_arrow(temp,ar_ori,ar_end);

		imshow("image", temp );

		char c = waitKey(1);
		switch (c){
			case 27:// esc
				cout<<endl<<"programa cerrado manualmente"<<endl;
				if(!VTrayectoria.empty()) VTrayectorias.push_back(VTrayectoria);
				if(!VTrayectorias.empty()){
					//guardar trayectorias
					//añadir ultima trayectoria
					cout<<"guardando trayectorias"<<endl;
					if(!saveRoutes(VTrayectorias, case_dir, video_name)){
						return -1;
					}
					fd_cascades.close();
					fd_angles.close();
					//destroyWindow("image");
					//Salir del programa
				}
				return 0;
			break;


			case 113://Q reset current objects
				if(!point_saved){
					vObj.clear();
					Image=OriImage.clone();
					box_drawed = false;
					arrow_drawed = false;
					obj_done=true;
					obj_start=true;
					cout<<"frame "<<nframe<<" RESET all labels"<<endl;
					setMouseCallback("image",NULL,NULL);
					setMouseCallback("image", my_mouse_callback_box);
				}else{
					cout<<"You have already saved the point you'll have to delete it manually"<<endl;
					setMouseCallback("image",NULL,NULL);
					setMouseCallback("image", my_mouse_callback_box);
				}

			break;


			case 32://espacio
				if(!point_saved && !drawing_arrow && !drawing_box){ //Restart drawing box
					setMouseCallback("image",NULL,NULL);
					setMouseCallback("image", my_mouse_callback_box);
					box_drawed=false;
					Image=ImageObj.clone();
				}

				if(point_saved){
					cout<<"frame "<<nframe<<" OK"<<endl;
					ruta <<images_dir.native()<<nframe<<"_"<<video_name<<".png"; //ruta para guardar la imagen

					imwrite(ruta.str(), OriImage);

					ruta<<" "<<vObj.size();

					//Escribir los datos en el fichero
					for (uint i=0; i<vObj.size(); i++){
						ruta <<" "<<vObj[i].x<<" "<<vObj[i].y<<" "<<vObj[i].width<<" "<<vObj[i].height;
					}

					//datos para entrenamiento de cascade
					fd_cascades << ruta.str() <<endl;

					ruta<<" angles";

					for (uint i=0; i<vObj.size(); i++){
						ruta<<" "<<vObj[i].angle;
					}
					

					fd_angles << ruta.str() << endl;

					ruta.str("");
				}

				//pasar a la siguiente imagen

				//leer frame
				if(point_saved || press_space){
					if(!(video.read(Image))){//End of video, time to save routes
						cout << "fin de la secuencia" << endl;
						VTrayectorias.push_back(VTrayectoria);
						cout<<"guardando trayectorias"<<endl;
						if(!saveRoutes(VTrayectorias, case_dir, video_name)){
							return -1;
						}

					}else{//New img
						ImageObj = Image.clone();
						OriImage = Image.clone();
						arrow_drawed=false;
						box_drawed=false;
						obj_done=false;
						obj_start=true;
						if(point_saved){
							press_space=false;
							point_saved = false;
						}else cout<<"frame "<<nframe<<" NOT LABELED"<<endl;
						vObj.clear();
						nframe++;
						setMouseCallback("image",NULL,NULL);
						setMouseCallback("image", my_mouse_callback_box);
					}
				}else{
					if(press_space==false){
						cout<<"Are you sure you don't want to label this image?  [space again]"<<endl;
						press_space = true;
					}
				}
			break;


			case 100: //D
				//borrar ultimo rectangulo hecho
				Image = ImageObj.clone();

			break;


			case 112 : //P
				if(!point_saved){
					if(!drawing_arrow && !arrow_drawed && (obj_done || obj_start)){
						if(!drawing_box && box_drawed){
							obj_done=false;
							obj_start=false;
							arrow_drawed = false;
							// imagen positiva
							draw_box(ImageObj,box);

							Image = ImageObj.clone();

							//añadir rectangulo
							obj.x = box.x;
							obj.y = box.y;

							obj.width = box.width;
							obj.height = box.height;

							
							setMouseCallback("image",NULL,NULL);
							
							//poder dibujar flechas

							//origen de la flecha -- centro del rectangulo
							ar_ori.x = obj.x + obj.width /2;
							ar_ori.y = obj.y + obj.height/2;
							ar_end.x = obj.x + obj.width /2;
							ar_end.y = obj.y + obj.height/2;
							//dibujar flecha
							setMouseCallback("image",my_mouse_callback_arrow);
							drawing_arrow=true;
						}else cout<<"Draw box first"<<endl;
					}else cout<<"Draw arrow first"<<endl;
				}else cout<<"You have saved the point, press space for an other frame"<<endl;
			break;


			case 110:  //n
				if(!point_saved){
					if(!obj_done && !obj_start){
						if(arrow_drawed){
							//desactivar flecha
							setMouseCallback("image",NULL,NULL);

							//guardar flecha
							draw_arrow(ImageObj,ar_ori,ar_end);

							Image = ImageObj.clone();

							drawing_arrow = false;
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
							puntotray.Pos = ar_ori;
							puntotray.angle = round(theta1);
							puntotray.frame = nframe;
							cout<<"frame "<<nframe<<" label "<<vObj.size()<<endl;
							setMouseCallback("image",NULL,NULL);
							setMouseCallback("image", my_mouse_callback_box);
							obj_done=true;
						}else cout<<"Draw arrow first"<<endl;
					}else cout<<"Draw box first"<<endl;
				}else cout<<"You have saved the point, press space for an other frame"<<endl;
			break;


			case 116://t Generar nueva trayectoria
				if(!point_saved){
					if(obj_done){
						if(!VTrayectoria.empty()){
							cout<<"Are you sure you want to add new route?  [press t again]"<<endl;
							char op;
							cin>>op;
							if(op=='t'){
								//añadir ruta guardada
								VTrayectorias.push_back(VTrayectoria);
								//borrar la ruta
							    VTrayectoria.clear();
							    //generar la ruta con el punto nuevo
							    VTrayectoria.push_back(puntotray);
							    point_saved=true;
							}else{
								cout<<"Cancelled new route"<<endl;
							}
						}else{
							cout<<"Cancelled new route (Your route has no points)"<<endl;
						}
					}else cout<<"Uncompleted label"<<endl;
				}else cout<<"You have already saved the point, press space for an other frame"<<endl;
			break;


			case 114://r almacenar puntos de la trayectoria
				if(!point_saved){
					if(obj_done){
						//despues de guardar la flecha
						VTrayectoria.push_back(puntotray);
						point_saved=true;
					}else cout<<"Uncompleted label"<<endl;
				}else cout<<"You have already saved the point, press space for an other frame"<<endl;
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
