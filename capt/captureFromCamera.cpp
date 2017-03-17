#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/contrib/contrib.hpp"

#include <stdio.h>
#include <iostream>
using namespace cv;
using namespace std;


#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "stdio.h"

int main(int argc, char* argv[]) {

//if (argc < 2 || argc > 3) { //Case when calibrate video generation is optional
  if (argc != 3) {
    //cout<<"USAGE:"<<endl<< "DataBaseGenerator [video_to_read] [directory/to/save/images] [route/to/cascade_angles.dat] [route/to/Cascade.dat] [fichero/trayectorias.yml]"<<endl;
    cout<<"USAGE:"<<endl<< "DataBaseGenerator [model_name] [camera_calibration_file]"<<endl;
    return -1;
  }

const string modelName = argv[1];
const string cameraCalibrationFile = argc > 2 ? argv[2] : "";

Mat cameraMatrix, distCoeffs, viewOriginal, viewUnDistorted;
//time_t start = time(0);

// Open the cameras attached to your computer
VideoCapture camera1 = VideoCapture(1); // open the video camera port usb no. 0

// Get the width/height of the camera frames (from cam1 to cam5)
Size size1 = Size(
                   (int)camera1.get(CV_CAP_PROP_FRAME_WIDTH),
                 (int)camera1.get(CV_CAP_PROP_FRAME_HEIGHT)
               );

namedWindow(modelName, CV_WINDOW_AUTOSIZE );
VideoWriter writerUnDistorted, writerOriginal; 
writerOriginal = VideoWriter("/home/ivan/videos/" + modelName + "_original.AVI",CV_FOURCC('M','P','4','3'),25,size1,true);

if(!cameraCalibrationFile.empty()){
  FileStorage fs(cameraCalibrationFile, FileStorage::READ); // Read the calibration values
    if (!fs.isOpened())
    {
        cout << "Could not open the configuration file: \"" << cameraCalibrationFile << "\"" << endl;
        return -1;
    }

    fs["Camera_Matrix"] >> cameraMatrix;
    fs["Distortion_Coefficients"] >> distCoeffs;
    fs.release();
    writerUnDistorted = VideoWriter("/home/ivan/videos/" + modelName + "_undistorted.AVI",CV_FOURCC('M','P','4','3'),25,size1,true);                                        
}



int numFrames = 0;

while(1)
{
	if(numFrames >= 250) break;
	//if (difftime( time(0), start) >= 5000) break;

  if(!camera1.read(viewOriginal)){
    cout<<"No frames has been grabbed (camera has been disconnected, or there are no more frames in video file"<<endl;
    return -1;
  }
  writerOriginal<<viewOriginal;
  if(!cameraCalibrationFile.empty()){
    viewUnDistorted = viewOriginal.clone();
    undistort(viewOriginal, viewUnDistorted, cameraMatrix, distCoeffs);
    writerUnDistorted<<viewUnDistorted;
    imshow(modelName, viewUnDistorted);
  }else{
    imshow(modelName, viewOriginal);
  }
  waitKey(40);
  numFrames++;
}
camera1.release();
destroyWindow(modelName);
return 0;
}
