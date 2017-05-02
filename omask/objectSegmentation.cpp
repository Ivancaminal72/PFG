#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
//#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include <cstdio>
#include <iostream>
using namespace cv;
using namespace std;
static void help(){
	cout<<"USE: + to enlarge draw tool"<<endl;
	cout<<"USE: - to reduce draw tool"<<endl;
	cout<<"USE: m to change between manual and watershed mode"<<endl;
	cout<<"USE: r to restart marks"<<endl;
}
int c;
uint8_t inx=4;
vector<unsigned char> vexp;
char strBgr[]= "Mark background", strObj[]= "Mark the object", 
strFin[]= "Press space to finish", strSave[]= "Press 's' to save",
strMan[]= "Manual mode", strCorr[]= "Position correction";
bool bgrDrawed = false, objDrawed =false, manual = false;
Mat markerMask, img, imgt, img0, imgGray, mres, mres_c;
vector<vector<Point> > contours;
vector<Vec4i> hierarchy;
Point prevPt(-1, -1);

static void onMouse( int event, int x, int y, int flags, void*){
    if( x < 0 || x >= img.cols || y < 0 || y >= img.rows )
        return;
    if( event == EVENT_LBUTTONUP){ //|| !(flags & EVENT_FLAG_LBUTTON) 
        prevPt = Point(-1,-1);
        findContours(markerMask.clone(), contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);
        if((contours.size() == 1) and manual) objDrawed = true;
        else if(contours.size() == 1 and !manual) bgrDrawed = true;
        else if((contours.size() == 2) and !manual) objDrawed = true;
        else{
            markerMask = Scalar::all(0);
            img0.copyTo(img);
            bgrDrawed=false;
            objDrawed=false;
        }
        imgt=img.clone();
        if(manual) putText(imgt, strMan, Point2f(10,470), FONT_HERSHEY_PLAIN, 2,  Scalar(0,255,255,255));
        if(objDrawed and !manual) putText(imgt, strFin, Point2f(250,470), FONT_HERSHEY_PLAIN, 2,  Scalar(255,0,0,255));
    	else if(!bgrDrawed and !manual) putText(imgt, strBgr, Point2f(350,470), FONT_HERSHEY_PLAIN, 2,  Scalar(0,0,255,255));
        else if(objDrawed and  manual) putText(imgt, strFin, Point2f(250,470), FONT_HERSHEY_PLAIN, 2,  Scalar(255,0,0,255));
        else putText(imgt, strObj, Point2f(350,470), FONT_HERSHEY_PLAIN, 2,  Scalar(0,0,255,255));
        imshow("image", imgt);
    }
    else if( event == EVENT_LBUTTONDOWN ) prevPt = Point(x,y);
    else if( event == EVENT_MOUSEMOVE && (flags & EVENT_FLAG_LBUTTON) ){
        Point pt(x, y);
        if( (pt.x >= 0 or pt.y >= 0 or pt.x<img.cols or pt.y<img.rows) and (pt != prevPt) ){
        	line( markerMask, prevPt, pt, Scalar(255), vexp.at(inx), 8, 0 );
			line( img, prevPt, pt, Scalar(255,255,255), vexp.at(inx), 8, 0 );
			prevPt = pt;
			imgt=img.clone();
			if(manual) putText(imgt, strMan, Point2f(10,470), FONT_HERSHEY_PLAIN, 2,  Scalar(0,255,255,255));
			if(objDrawed and !manual) putText(imgt, strFin, Point2f(250,470), FONT_HERSHEY_PLAIN, 2,  Scalar(255,0,0,255));
			else if(!bgrDrawed and !manual) putText(imgt, strBgr, Point2f(350,470), FONT_HERSHEY_PLAIN, 2,  Scalar(0,0,255,255));
			else if(objDrawed and  manual) putText(imgt, strFin, Point2f(250,470), FONT_HERSHEY_PLAIN, 2,  Scalar(255,0,0,255));
			else putText(imgt, strObj, Point2f(350,470), FONT_HERSHEY_PLAIN, 2,  Scalar(0,0,255,255));
			imshow("image", imgt);
        }
    }
}

int main( int argc, char** argv ){
    string imgPath, savePath;
    float RPM = 130;
    float objHeight, sensorHeight = 3.07*RPM;
    if(argc < 4 || argc > 6) {
        cout<<"USAGE:"<<endl<< "./objectSegmentation image_path mask_name object_height [sensor_height]"<<endl<<
        " [relation_pixel_meter]"<<endl<<endl;
        cout<<"DEFAULT [sensor_height] = "<<sensorHeight/RPM<<"m"<<endl;
        cout<<"DEFAULT [relation_pixel_meter] = "<<RPM<<"pixels/meter"<<endl;
        return -1;
    }else{
         imgPath = argv[1];
         savePath = argv[2];
         objHeight = atof(argv[3]);
         if(argc > 4) {
         	sensorHeight = atof(argv[4]);
         	if(argc == 6) RPM = atof(argv[5]);
         	sensorHeight *= RPM;
         }
         objHeight = objHeight*RPM;
         savePath = "./masks/"+savePath+".png";
    }
    img0 = imread(imgPath, 1);
    if( img0.empty() ){
        cout << "Error: can't open image " << imgPath<<endl;
        return -1;
    }
    help();
    namedWindow( "image", 1 );
    cv::Size imgSize = img0.size();
    img0.copyTo(img);
    cvtColor(img, imgGray, COLOR_BGR2GRAY);
    markerMask = Mat::zeros(imgSize, CV_8UC1);
    imgt=img.clone();
    putText(imgt, strBgr, Point2f(350,470), FONT_HERSHEY_PLAIN, 2,  Scalar(0,0,255,255));
    imshow( "image", imgt );
    setMouseCallback( "image", onMouse, 0 );
    float ii=0, resp=round(pow(1.4,ii));
    while(resp<=255){
    	vexp.push_back((unsigned char)resp);
    	ii+=1;
    	resp=round(pow(1.4,ii));
    }

    for(;;){
        c = waitKey(0);
        if( (char)c == 27 ) break;

        if( (char)c == '-' && inx>0) inx-=1;

        if( (char)c == '+' && inx<vexp.size()-1) inx+=1;

		if( (char)c == 'r' ){
			markerMask = Scalar::all(0);
			img0.copyTo(img);
			imgt=img.clone();
			bgrDrawed=false;
			objDrawed=false;
			putText(imgt, strBgr, Point2f(350,470), FONT_HERSHEY_PLAIN, 2,  Scalar(0,0,255,255));
			if(manual) putText(imgt, strMan, Point2f(10,470), FONT_HERSHEY_PLAIN, 2,  Scalar(0,255,255,255));
			imshow("image", imgt);
		}

		if( ((char)c == 'm') ){
			manual = !manual;
			markerMask = Scalar::all(0);
			img0.copyTo(img);
			imgt=img.clone();
			objDrawed=false;
			if(manual) {
				putText(imgt, strMan, Point2f(10,470), FONT_HERSHEY_PLAIN, 2,  Scalar(0,255,255,255));
				putText(imgt, strObj, Point2f(350,470), FONT_HERSHEY_PLAIN, 2,  Scalar(0,0,255,255));
			}else{
				putText(imgt, strBgr, Point2f(350,470), FONT_HERSHEY_PLAIN, 2,  Scalar(0,0,255,255));
			}
			imshow("image", imgt);
		}

		if( ((char)c == ' ') ){
			if(!manual){
				int i, j, compCount = 0;
				findContours(markerMask, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);
				if( contours.size() > 2){
				    cout<<"Too many marks"<<endl;
				    bgrDrawed=false;
				    objDrawed=false;
				    markerMask = Scalar::all(0);
				    img0.copyTo(img);
				    imgt=img.clone();
				    putText(imgt, strBgr, Point2f(350,470), FONT_HERSHEY_PLAIN, 2,  Scalar(0,0,255,255));
				    imshow("image", imgt);
				    continue;
				}

				Mat markers(imgSize, CV_32S);
				markers = Scalar::all(0);
				int idx = 0;
				for( ; idx >= 0; idx = hierarchy[idx][0], compCount++ )
				    drawContours(markers, contours, idx, Scalar::all(compCount+1), -1, 8, hierarchy, INT_MAX);
				if( compCount == 0 )
				    continue;


				double t = (double)getTickCount();
				watershed( img0, markers );
				t = (double)getTickCount() - t;
				printf( "execution time = %gms\n", t*1000./getTickFrequency() );
				mres = Mat::zeros(imgSize, CV_8UC1);
				// paint the watershed image
				for( i = 0; i < markers.rows; i++ )
				    for( j = 0; j < markers.cols; j++ )
				    {
				        int index = markers.at<int>(i,j);
				        if(index == 1)
				            mres.at<uint8_t>(i,j) = 255;
				        else
				            mres.at<uint8_t>(i,j) = 0;
				    }

			}else mres=markerMask;

			
            Point2d pcorr;
            mres_c = Mat::zeros(imgSize, CV_8UC1);
			for(int i = 0; i < mres.rows; i++)
				for(int j = 0; j < mres.cols; j++){
					if(mres.at<uint8_t>(i,j) == 255){
						pcorr.x = (double)j - imgSize.width/2;
						pcorr.y = (double)i - imgSize.height/2;
						pcorr -= objHeight/sensorHeight*pcorr;
						pcorr.x += imgSize.width/2;
						pcorr.y += imgSize.height/2;
						mres_c.at<uint8_t>(round(pcorr.y), round(pcorr.x)) = 255;
					}
				}
			img = mres*0.4 + mres_c*0.4 + imgGray*0.2;
			cvtColor(img, img, COLOR_GRAY2BGR);
			putText(img, strCorr, Point2f(300,470), FONT_HERSHEY_PLAIN, 2,  Scalar(0,0,255,255));
			imshow( "image", img );
			c = waitKey(0);

			img = mres*0.6 + imgGray*0.4;
			cvtColor(img, img, COLOR_GRAY2BGR);
			putText(img, strSave, Point2f(300,470), FONT_HERSHEY_PLAIN, 2,  Scalar(0,255,0,255));
			imshow( "image", img );
			c = waitKey(0);

            if( (char)c == 's') {
                vector<int> compression_params;
                compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
                compression_params.push_back(0);
                imwrite(savePath, mres_c, compression_params);
            }
            return 0;
        }
    }
}