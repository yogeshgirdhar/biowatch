#include <cv.h>
#include <highgui.h>
#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>

using namespace std;
namespace fs = boost::filesystem;

void imgswap(cv::Mat& img1, cv::Mat& img2){
  cv::Mat img3;
  img3=img1;
  img1=img2;
  img2=img3;
}

/*
cv::Mat measure_ant(cv::Mat& img, cv::Mat& measurement){
  
  double min, max;
  cv::Point minLoc, maxLoc;
  cv::minMaxLoc(img,&min, &max, &minLoc, &maxLoc);
  double T = (max-min)/2.0;
  cv::Mat timg;
  cv::threshold(img,timg, T, 1.0, THRESH_BINARY);

  measurement.at<float>(0,0)=(float)maxLoc.x;
  measurement.at<float>(1,0)=(float)maxLoc.y;
  measurement.at<float>(2,0)=(float)0;
  measurement.at<float>(3,0)=(float)0;

  return measurement;
}
*/

int main(int argc, char* argv[]){
  if(argc < 2){
    cerr<<"Usage: "<<argv[0]<<" <videofilename>"<<endl;
    exit(0);
  }
  int MAX_ANT_AREA=60;
  string filename(argv[1]);
  fs::path pathname(argv[1]);
  std::string dirname  = pathname.parent_path().string();
  if(dirname=="") dirname="./";
  std::string basename = boost::filesystem::basename (pathname);

  cerr<<dirname<<"  "<<basename<<endl;

  cv::VideoCapture video(filename);
  cerr<<(video.isOpened()?": OK":": FAIL")<<endl;

  cv::Mat image, image_tmp, image_rgb_last, motion;
  cv::Mat_<float> accum_image(image_tmp.size(),0.0);
  //  cv::Mat_<float> accum_image_01(image_tmp.size(),0.0);
  cv::Mat_<float> image_fp(image_tmp.size());
  cv::Mat_<float> image_sans_obs(image_tmp.size());
  cv::Mat_<float> observation(image_tmp.size(),0.0);
  cv::Mat_<float> observationT(image_tmp.size(),0.0);
  cv::Mat_<float> dilated_observation(image_tmp.size(),0.0);


  video.grab();
  video.retrieve(image_tmp);
  cv::imwrite(dirname+"/"+basename+"_bg.jpg",image_tmp);
  image_rgb_last = image_tmp.clone();
  cv::cvtColor(image_tmp, image, CV_BGR2GRAY);
  image.convertTo(image_fp, CV_32F, 1.0/256.0);
  accum_image = image_fp.clone();
  cvWaitKey(10);



  int count=0;
  std::vector<cv::Point2f> points;
  cv::Point2f state;
  cv::RotatedRect rect;
  cv::RotatedRect last_locked_rect;

  std::ofstream out(dirname+"/"+basename+"_data.txt");
  bool locked=false;
  while(video.grab()){
    count++;
    video.retrieve(image_tmp);
    cv::cvtColor(image_tmp, image, CV_BGR2GRAY);
    image.convertTo(image_fp, CV_32F, 1.0/256.0);


    if(!points.empty() && locked){
      cv::dilate(observationT,observationT,cv::Mat());
      image_sans_obs =(1.0-observationT).mul(image_fp) + observationT.mul(accum_image);
      cv::accumulateWeighted(image_sans_obs, accum_image,0.01);
    }
    else{
      cv::accumulateWeighted(image_fp, accum_image,0.01);
    }

    cv::absdiff(image_fp, accum_image, observation);


    double min, max;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(image_fp,&min, &max, &minLoc, &maxLoc);
    double T = min+(max-min)/4.0;
    cv::threshold(observation,observationT, T, 1.0, CV_THRESH_BINARY);
    points.clear();
    state=cv::Point2f(0,0);
    for(int r=0;r<observation.rows;r++){
      for(int c=0;c<observation.cols;c++){
	if(observationT(r,c)==1.0){
	  points.push_back(cv::Point_<int>(c,r));
	  //state = state + 
	}
      }
    }
    //cerr<<"points="<<points.size()<<endl;
    if(points.size()>=1){
      rect = cv::minAreaRect(points);
      rect.angle+=90;

      if(rect.size.width*rect.size.height < MAX_ANT_AREA)
	locked=true;
      else
	locked=false;

      if(locked){
	cv::ellipse(image_tmp, rect, cv::Scalar(0,255,0));
	last_locked_rect=rect;
      }
      else{
	cv::ellipse(image_tmp, last_locked_rect, cv::Scalar(255,0,0));
	cv::ellipse(image_tmp, rect, cv::Scalar(0,0,255));
      }
    }
    out<<count<<" "
       <<last_locked_rect.center.x<<" "
       <<(float)image_tmp.rows-last_locked_rect.center.y<<" "
       <<last_locked_rect.size.width<<" "
       <<last_locked_rect.size.height<<" "
       <<last_locked_rect.angle<<" "
       <<(int)locked<<endl;
    cerr<<count<<" "
	<<last_locked_rect.center.x<<" "
	<<(float)image_tmp.rows-last_locked_rect.center.y<<" "
	<<last_locked_rect.size.width<<" "
	<<last_locked_rect.size.height<<" "
	<<last_locked_rect.angle<<" "
	<<(int)locked<<endl;


    cv::imshow(filename, image_tmp);	
    cv::imshow("bg", accum_image);	
    cv::imshow("ant", observation);	


    int c = cvWaitKey(10);
    switch((char) c){
    case 27:
      return 0; break;
    case 'b':
      cerr<<"Writing Background image"<<endl;
      cv::Mat bgimg;
      accum_image.convertTo(bgimg,CV_8U,256.0);
      cv::imwrite(dirname+"/"+basename+"_bg.jpg",bgimg);
      break;
    }
    std::swap(image_tmp,image_rgb_last);
    //    imgswap(image,image0);
  }

  return 0;
}
