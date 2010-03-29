#include <cv.h>
#include <highgui.h>
#include <iostream>
#include <fstream>
using namespace std;

void evolve(std::vector<cv::RotatedRect>& in, std::vector<cv::RotatedRect>& out){
  
}
int S=15;
int main(int argc, char* argv[]){

  string filename(argv[1]);
  cv::VideoCapture video(filename);
  cerr<<(video.isOpened()?": OK":": FAIL")<<endl;

  cv::Mat image, image_tmp, image_rgb_last, motion;
  cv::Mat_<float> accum_image(image_tmp.size(),0.0);
  cv::Mat_<float> accum_image_01(image_tmp.size(),0.0);
  cv::Mat_<float> image_fp(image_tmp.size());
  cv::Mat_<float> observation(image_tmp.size());


  video.grab();
  video.retrieve(image_tmp);
  image_rgb_last = image_tmp.clone();
  cv::cvtColor(image_tmp, image, CV_BGR2GRAY);
  image.convertTo(image_fp, CV_32F, 1.0/256.0);
  accum_image = image_fp.clone();
  cvWaitKey(10);



  int count=0;
  std::vector<cv::Point2f> points;
  cv::Point2f state;
  cv::RotatedRect rect;
  std::ofstream out("out.txt");
  while(video.grab()){
    count++;
    video.retrieve(image_tmp);
    cv::cvtColor(image_tmp, image, CV_BGR2GRAY);
    image.convertTo(image_fp, CV_32F, 1.0/256.0);
    cv::accumulateWeighted(image_fp, accum_image,0.01);
    accum_image_01 = accum_image *1.0;
    cv::absdiff(image_fp, accum_image_01, observation);


    double min, max;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(image_fp,&min, &max, &minLoc, &maxLoc);
    double T = (max-min)/4.0;
    cv::threshold(observation,observation, T, 1.0, CV_THRESH_BINARY);
    points.clear();
    state=cv::Point2f(0,0);
    for(int r=0;r<observation.rows;r++){
      for(int c=0;c<observation.cols;c++){
	if(observation(r,c)==1.0){
	  points.push_back(cv::Point_<int>(c,r));
	  //state = state + 
	}
      }
    }
    //cerr<<"points="<<points.size()<<endl;
    if(points.size()>=1){
      rect = cv::minAreaRect(points);
      rect.angle+=90;
      cv::ellipse(image_tmp, rect, cv::Scalar(255,0,0));
    }
    out<<rect.center.x<<" "<<(float)image_tmp.rows-rect.center.y<<" "<<rect.size.width<<" "<<rect.size.height<<" "<<rect.angle<<" "<<points.size()<<endl;
    cerr<<rect.center.x<<" "<<(float)image_tmp.rows-rect.center.y<<" "<<rect.size.width<<" "<<rect.size.height<<" "<<rect.angle<<" "<<points.size()<<endl;


    cv::imshow(filename, image_tmp);	
    cv::imshow("bg", accum_image_01);	
    cv::imshow("ant", observation);	

    int c = cvWaitKey(10);
    switch((char) c){
    case 27:
      return 0; break;
    case 's':
      cv::Mat bgimg;
      accum_image_01.convertTo(bgimg,CV_8U,256.0);
      cv::imwrite("background.jpg",bgimg);
      break;
    }

    std::swap(image_tmp,image_rgb_last);
    //    imgswap(image,image0);
  }

  return 0;
}
