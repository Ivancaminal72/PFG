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

void getERR(const Mat& I1, const Mat& I2, double& mse, double& psnr, Mat& dif)
{
    Mat s1;
    absdiff(I1, I2, dif);       // |I1 - I2|
    dif.convertTo(s1, CV_32F);  // cannot make a square on 8 bits
    s1 = s1.mul(s1);           // |I1 - I2|^2

    Scalar s = sum(s1);         // sum elements per channel

    double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels

    if( sse <= 1e-10) // for small values return zero
        psnr = 0;
    else
    {
        mse = sse /(double)(I1.channels() * I1.total());
        psnr = 10.0*log10((255*255)/mse);
    }
}


int main( int argc, char* argv[] ) {
	if(argc != 3){cout<<"USAGE:"<<endl<< "./errImg image1_path image2_path"<<endl; return -1;}
	bf::path img_p1, img_p2;
	img_p1 = argv[1];
	img_p2 = argv[2];
	Mat img1, img2, dif;
	double psnr, mse;

	//Load mat
	img1 = imread(img_p1.native());
    if( img1.empty() ){
        cout << "Error: can't open image " << img_p1<<endl;
        return -1;
    }

    img2 = imread(img_p2.native());
    if( img2.empty() ){
        cout << "Error: can't open image " << img_p2<<endl;
        return -1;
    }

    //Calculate mesures of image quality
    getERR(img1, img2, mse, psnr, dif);


	cout<<"Mesures of image quality: "<<endl;
	printf ("PSNR: %g dB \n", psnr);
	printf ("MSE: %g \n ", mse);

	imshow("image", dif);
	waitKey(0);
}
