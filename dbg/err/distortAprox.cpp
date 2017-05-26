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


bool loadCalibParams(bf::path calib_file, Mat& cameraMatrix, Mat& distCoeffs){
	FileStorage fs(calib_file.native(), FileStorage::READ); // Read the calibration values
	if (!fs.isOpened()){
	    cout << "Could not open the configuration file: \"" << calib_file.native() << "\"" << endl;
	    return false;
	}

	fs["Camera_Matrix"] >> cameraMatrix;
	fs["Distortion_Coefficients"] >> distCoeffs;
	fs.release();
	return true;
}

int main( int argc, char* argv[] ) {
	bf::path calib_path1, calib_path2;
	Mat cameraMatrix2, cameraMatrix1, distCoeffs1, distCoeffs2;
	cv::Size imgSize;
	int max_argc = 5;

	if (argc < 2 || argc > max_argc) {//wrong arguments
		cout<<"USAGE:"<<endl<< "./distortAprox calibration_file1 img_width img_height [calibration_file2] \n";
		return -1;
	}else{
		calib_path1 = argv[1];
		imgSize.width = atoi(argv[2]);
		imgSize.height = atoi(argv[3]);
		if(argc == max_argc) {
			calib_path2 = argv[4];
			if(!bf::exists(calib_path2)) {cout<<"Error: calib_path2 doesn't exist"<<endl; return -1;}
		}
		if(!bf::exists(calib_path1)) {cout<<"Error: calib_path1 doesn't exist"<<endl; return -1;}
	}
	
	
	//Load calibrations parameters
	loadCalibParams(calib_path1, cameraMatrix1, distCoeffs1);
	if(argc == max_argc) loadCalibParams(calib_path2, cameraMatrix2, distCoeffs2);

	//Calculate aprox error dist calib1
	Mat mOri(1,1,CV_64FC2), mUnd(1,1,CV_64FC2), mUnd2(1,1,CV_64FC2), eye;
	Point2d u, u2;
	double med_dist_uu2=0, med_dist_ou=0, med_dist_ou2=0;
	double dev_dist_uu2=0, dev_dist_ou=0, dev_dist_ou2=0, dist;
	int valid=0, valid2=0;

	for(double i=0; i<imgSize.width; i++){
		for(double j=0; j<imgSize.height; j++){

			mOri.at<Vec2d> (0,0)[0] = i;
			mOri.at<Vec2d> (0,0)[1] = j;
			cv::undistortPoints(mOri, mUnd, cameraMatrix1, distCoeffs1, eye, cameraMatrix1);
			u.x = trunc(mUnd.at<Vec2d> (0,0)[0]);
			u.y = trunc(mUnd.at<Vec2d> (0,0)[1]);

			//Calculate distance
			if(u.x >= 0 and u.x <imgSize.width and u.y >=0 and u.x <imgSize.height){
				med_dist_ou += sqrt(pow(u.x-i,2.0)+pow(u.y-j,2.0));
				valid +=1;
			}

			if(argc == max_argc){
				//Calculate aprox error dist calib2
				mOri.at<Vec2d> (0,0)[0] = i;
				mOri.at<Vec2d> (0,0)[1] = j;
				cv::undistortPoints(mOri, mUnd2, cameraMatrix2, distCoeffs2, eye, cameraMatrix2);
				u2.x = trunc(mUnd2.at<Vec2d> (0,0)[0]);
				u2.y = trunc(mUnd2.at<Vec2d> (0,0)[1]);
				
				//Calculate distance
				if(u2.x >= 0 and u2.x <imgSize.width and u2.y >=0 and u2.x <imgSize.height){
					med_dist_uu2 += sqrt(pow(u2.x-u.x,2.0)+pow(u2.y-u.y,2.0));
					med_dist_ou2 += sqrt(pow(u2.x-i,2.0)+pow(u2.y-j,2.0));
					valid2 +=1;
				}
			}
		}
	}
	med_dist_ou /= valid;
	if(argc == max_argc){
		med_dist_ou2 /= valid2;
		med_dist_uu2 /= valid2;
	}

	for(double i=0; i<imgSize.width; i++){
		for(double j=0; j<imgSize.height; j++){

			mOri.at<Vec2d> (0,0)[0] = i;
			mOri.at<Vec2d> (0,0)[1] = j;
			cv::undistortPoints(mOri, mUnd, cameraMatrix1, distCoeffs1, eye, cameraMatrix1);
			u.x = trunc(mUnd.at<Vec2d> (0,0)[0]);
			u.y = trunc(mUnd.at<Vec2d> (0,0)[1]);

			//Calculate distance
			if(u.x >= 0 and u.x <imgSize.width and u.y >=0 and u.x <imgSize.height){
				dist = sqrt(pow(u.x-i,2.0)+pow(u.y-j,2.0));
				dev_dist_ou += abs(med_dist_ou - dist); 
			}

			if(argc == max_argc){
				//Calculate aprox error dist calib2
				mOri.at<Vec2d> (0,0)[0] = i;
				mOri.at<Vec2d> (0,0)[1] = j;
				cv::undistortPoints(mOri, mUnd2, cameraMatrix2, distCoeffs2, eye, cameraMatrix2);
				u2.x = trunc(mUnd2.at<Vec2d> (0,0)[0]);
				u2.y = trunc(mUnd2.at<Vec2d> (0,0)[1]);
				
				//Calculate distance
				if(u2.x >= 0 and u2.x <imgSize.width and u2.y >=0 and u2.x <imgSize.height){
					dist = sqrt(pow(u2.x-u.x,2.0)+pow(u2.y-u.y,2.0));
					dev_dist_uu2 += abs(med_dist_ou2 - dist);
					dist = sqrt(pow(u2.x-i,2.0)+pow(u2.y-j,2.0));
					dev_dist_ou2 += abs(med_dist_uu2 - dist);
				}
			}
		}
	}

	dev_dist_ou /= valid;
	if(argc == max_argc){
		dev_dist_ou2 /= valid2;
		dev_dist_uu2 /= valid2;
	}

	cout<<"Average error between routes: "<<endl;
	printf ("Original_vs_Calib1-> Median: %g Deviation: %g \n", med_dist_ou, dev_dist_ou);
	if(argc == max_argc){
		printf ("Original_vs_Calib1-> Median: %g Deviation: %g \n", med_dist_ou2, dev_dist_ou2);
		printf ("Calib1__vs__Calib2-> Median: %g Deviation: %g \n", med_dist_uu2, dev_dist_uu2);
	}
}
