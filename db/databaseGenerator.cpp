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

#define PI 3.14159265

using namespace cv;
using namespace std;

// Define our callback which we will install for
// mouse events.
//

void my_mouse_callback_box( int event, int x, int y, int flags, void* param );

void my_mouse_callback_arrow( int event, int x, int y, int flags, void* param );

CvRect box;
bool drawing_box = false;
bool drawing_arrow = false;
bool arrow_drawed = false;
bool point_saved = false;

Mat Image;
Mat ImageRect;
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


// dibujar rectangulo en la imagen
void draw_box( Mat img, CvRect rect ) {

	  rectangle (img, cvPoint(box.x,box.y),cvPoint(box.x+box.width,box.y+box.height), Scalar(0,255,255));

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



int main( int argc, char* argv[] ) {

	if (argc != 3) {
		//cout<<"USAGE:"<<endl<< "DataBaseGenerator [video_to_read] [directory/to/save/images] [route/to/cascade_angles.dat] [route/to/Cascade.dat] [fichero/trayectorias.yml]"<<endl;
		cout<<"USAGE:"<<endl<< "DataBaseGenerator [video_path_to_read] [directory/case]"<<endl;
		return -1;
	}	

	cout<<"Press P to save rectangle"<<endl<<"Press D to clear a wrong box"<<endl;
	cout<<"Press N to save arrow"<<endl;
	cout<<"Press R to add point to a route"<<endl;
	cout<<"Press T to create a new route (not first time)"<<endl;
	cout<<"Press SPACE to next image"<<endl<<"Press ESC to exit"<<endl;

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

	string images_dir = case_dir+"images/";
	string angles_path = case_dir+"angles/"+video_name+".dat";
	string cascade_path = case_dir+"cascades/"+video_name+".dat";

	vector<Point> origin;
	vector<Point> tam;


	//inicializacion trayectorias
	vector <vector < TrayPoint> > VTrayectorias;
	vector <TrayPoint> VTrayectoria;

	TrayPoint puntotray;
	puntotray.Pos = Point(-1,-1);
	puntotray.frame = -1;
	puntotray.angle = 0.0;

	vector<double> angles;

	Point orig (-1,-1);
	Point siz (0,0);

	float deltax=0;
	float deltay=0;
	double theta1=0;


	int cimagesp = 0;

	//contar cuantas imagenes existen en el directorio

	/* Con un puntero a DIR abriremos el directorio */
	DIR* dp = opendir(images_dir.c_str());

	/* en *ent habrá información sobre el archivo que se está "sacando" a cada momento */
	struct dirent *ent;

	 if (dp == NULL){
		cout<<"No puedo abrir el directorio"<<endl;;
		return -1;
	 }


	 while ((ent = readdir (dp)) != NULL)
		 {
		   /* Nos devolverá el directorio actual (.) y el anterior (..), como hace ls */
		   if ( (strcmp(ent->d_name, ".")!=0) && (strcmp(ent->d_name, "..")!=0) )
		 {
		   cimagesp++;
		 }
		}

	 closedir (dp);

	//cout<< "numero de imagenes en el directorio "<< cimagesp <<endl;


	int objects = 0;

	fstream anglesFile, cascadeFile;
	anglesFile.open(angles_path.c_str(), std::ios::out | std::ios::app);
	cascadeFile.open(cascade_path.c_str(), std::ios::out | std::ios::app);


	if (!anglesFile.is_open()){

		cout <<"no se ha podido abrir el archivo de datos 1"<<endl;
		return -1;
	}
	if(!cascadeFile.is_open()){
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

    ImageRect = Image.clone(); //Imagen con el ultimo rectangulo dibujado

    OriImage = Image.clone(); //imagen original

	temp = ImageRect.clone(); // imagen con los rectangulos temporales y finales

	namedWindow( "image" );

	setMouseCallback("image", my_mouse_callback_box );

	stringstream ruta;

    cout<<"inicio del programa"<<endl;
	
    while(1) {

        temp = Image.clone();

		if(drawing_box) draw_box( temp, box );

		if(drawing_arrow) draw_arrow(temp,ar_ori,ar_end);

		imshow("image", temp );

		char c = waitKey(1);
		switch (c){
			case 27: {// esc
				cout<<endl<<"programa cerrado manualmente"<<endl;
				if(!VTrayectoria.empty()) VTrayectorias.push_back(VTrayectoria);
				if(!VTrayectorias.empty()){
					//guardar trayectorias
					//añadir ultima trayectoria
					cout<<"guardando trayectorias"<<endl;
					if(!saveRoutes(VTrayectorias, case_dir, video_name)){
						return -1;
					}
					cascadeFile.close();
					anglesFile.close();
					//destroyWindow("image");
					//Salir del programa
				}
				return 0;
			}
			break;
			case 32: {//espacio
				if(!point_saved){
					setMouseCallback("image",NULL,NULL);
					drawing_arrow=false;
					arrow_drawed=false;
					setMouseCallback("image", my_mouse_callback_box );
				}

				if(objects > 0 && point_saved){
					cout<<"frame "<<nframe<<" OK"<<endl;
					ruta <<images_dir<<nframe<<"_"<<video_name<<".png"; //ruta para guardar la imagen

					imwrite(ruta.str(), OriImage);

					ruta<<" "<<objects;

					//Escribir los datos en el fichero
					for (int i=0; i< objects; i++){
						ruta <<" "<<origin[i].x<<" "<<origin[i].y<<" "<<tam[i].x<<" "<<tam[i].y;
					}

					//datos para entrenamiento de cascade
					cascadeFile << ruta.str() <<endl;


					if(!angles.empty()){
						ruta<<" angles";

						for (int i=0; i<objects; i++){
							ruta<<" "<<angles[i];
						}
					}

					anglesFile << ruta.str() << endl;

					ruta.str("");
				}else{
					cout<<"frame "<<nframe<<" NOT LABELED"<<endl;
				}

				//pasar a la siguiente imagen

				//leer frame
				if(!(video.read(Image))){
					cout << "fin de la secuencia" << endl;
					//añadir ultima trayectoria
					VTrayectorias.push_back(VTrayectoria);
					if(!VTrayectorias.empty()){ 
						//guardar trayectorias
						cout<<"guardando trayectorias"<<endl;
						if(!saveRoutes(VTrayectorias, case_dir, video_name)){
							return -1;
						}
					}
					return -1;
				}

				ImageRect = Image.clone();
				OriImage = Image.clone();
				objects = 0;
				tam.clear();
				origin.clear();
				angles.clear();
				nframe++;
			}
			break;
			case 100: { //D

				//borrar ultimo rectangulo hecho
				Image = ImageRect.clone();


			}
			break;
			case 112 : { //P
				if(!drawing_arrow && !arrow_drawed && !drawing_box){
					// imagen positiva
					draw_box(ImageRect,box);

					Image = ImageRect.clone();

					//añadir rectangulo
					objects ++;

					orig.x = box.x;
					orig.y = box.y;

					siz.x = box.width;
					siz.y = box.height;

					origin.push_back(orig);
					tam.push_back(siz);

					//dibujar flecha

					//anular dibujar cajas
					setMouseCallback("image",NULL,NULL);
					//poder dibujar flechas

					//origen de la flecha -- centro del rectangulo
					ar_ori.x = (orig.x + (siz.x /2) );
					ar_ori.y = (orig.y + (siz.y /2) );
					ar_end.x = (orig.x + (siz.x /2) );
					ar_end.y = (orig.y + (siz.y /2) );

					setMouseCallback("image",my_mouse_callback_arrow);

					drawing_arrow=true;
				}else{
					cout<<"You have to draw an arrow"<<endl;
				}
			}
			break;
			case 110:  //n
				if(arrow_drawed){
					//desactivar flecha
					setMouseCallback("image",NULL,NULL);

					//guardar flecha
					draw_arrow(ImageRect,ar_ori,ar_end);

					Image = ImageRect.clone();

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

					angles.push_back(round(theta1));

					//guardar datos para las trayectorias
					puntotray.Pos = ar_ori;
					puntotray.angle = round(theta1);
					puntotray.frame = nframe;
					//reestablecer valores
					ar_ori.x=-1;
					ar_ori.y=-1;
					ar_end.x=-1;
					ar_end.y=-1;

				}else{
					cout<<"Draw arrow first"<<endl;
				}
			break;
			case 116://t Generar nueva trayectoria
				if(arrow_drawed && !point_saved){
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
				}else{
					cout<<"Draw arrow first"<<endl;
				}
			break;
			case 114://r almacenar puntos de la trayectoria
				if(arrow_drawed && !point_saved){
				//despues de guardar la flecha
				VTrayectoria.push_back(puntotray);
				point_saved=true;
				}else{
					cout<<"Draw arrow first"<<endl;
				}
			break;
		}
	}
}


void my_mouse_callback_box( int event, int x, int y, int flags, void* param )
    {

		switch( event ) {
			case CV_EVENT_MOUSEMOVE: {
				if(drawing_box) {
					box.width = x-box.x;
					box.height = y-box.y;
				}
			}
			break;
			case CV_EVENT_LBUTTONDOWN: {
				drawing_box = true;
				box = cvRect(x, y, 0, 0);
				}
			break;
			case CV_EVENT_LBUTTONUP: {
				drawing_box = false;
				if(box.width<0) {
					box.x+=box.width;
					box.width *=-1;
				}
				if(box.height<0) {
					box.y+=box.height;
					box.height*=-1;
				}

				draw_box(Image,box);
				}
			break;
		}
	}

void my_mouse_callback_arrow( int event, int x, int y, int flags, void* param )
    {

		switch(event) {
			case CV_EVENT_LBUTTONUP:
				drawing_arrow=false;
				ar_end.x=x;
				ar_end.y=y;
				draw_arrow(Image,ar_ori,ar_end);
				arrow_drawed=true;
			break;

			case CV_EVENT_MOUSEMOVE:
				if(drawing_arrow) {
					ar_end.x=x;
					ar_end.y=y;
				}
			break;

			case CV_EVENT_LBUTTONDOWN:
				if(!drawing_arrow) drawing_arrow=true;
			break;
		}
	}
