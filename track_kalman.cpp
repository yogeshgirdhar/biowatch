#include <cv.h>
#include <highgui.h>
#include <iostream>
using namespace std;

void imgswap(cv::Mat& img1, cv::Mat& img2){
  cv::Mat img3;
  img3=img1;
  img1=img2;
  img2=img3;
}



bool find_ant(cv::Mat& img, int width, cv::Point& loc, cv::Mat& response){
  cv::Mat sum;
  cv::integral(img,sum,CV_32F);
  for(int row=width; row < (img.rows -width); row++){

    for(int col=width; col< (img.cols -width); col++){
      response.at<float>(row,col)= 
	sum.at<float>(row+width, col+width)-
	sum.at<float>(row+width, col-width)-
	sum.at<float>(row-width, col+width)+
	sum.at<float>(row-width, col-width);
    }
  }
  double min, max;
  cv::Point minLoc, maxLoc;
  cv::minMaxLoc(response,&min, &max, &minLoc, &maxLoc);
  double mean = cv::mean(response)[0];
  //  response/=max;
  loc=maxLoc;
  cerr<<max<<" "<<mean<<" "<<max/mean<<endl;
  if(max/mean > 40)
    return true;
  else
    return false;
}

cv::Mat measure_ant(cv::Mat& img, int width, cv::Mat& response, cv::Mat& measurement){
  cv::Mat sum;
  cv::integral(img,sum,CV_32F);
  for(int row=width; row < (img.rows -width); row++){

    for(int col=width; col< (img.cols -width); col++){
      response.at<float>(row,col)= 
	sum.at<float>(row+width, col+width)-
	sum.at<float>(row+width, col-width)-
	sum.at<float>(row-width, col+width)+
	sum.at<float>(row-width, col-width);
    }
  }
  double min, max;
  cv::Point minLoc, maxLoc;
  cv::minMaxLoc(response,&min, &max, &minLoc, &maxLoc);
  double mean = cv::mean(response)[0];
  //  response/=max;
  //  loc=maxLoc;

  measurement.at<float>(0,0)=(float)maxLoc.x;
  measurement.at<float>(1,0)=(float)maxLoc.y;
  measurement.at<float>(2,0)=(float)0;
  measurement.at<float>(3,0)=(float)0;

  return measurement;
}

int main(int argc, char* argv[]){

  string filename(argv[1]);
  cv::VideoCapture video(filename);
  cerr<<(video.isOpened()?": OK":": FAIL")<<endl;

  cv::Mat image, image_tmp, image0, motion;
  cv::namedWindow(filename,CV_WINDOW_AUTOSIZE);

  video.grab();
  video.retrieve(image_tmp);
  cv::cvtColor(image_tmp, image0, CV_BGR2GRAY);
  cvWaitKey(10);

  cv::Mat_<float> response(image0.rows, image0.cols);
  cv::Point antLoc, antLoc0, tmpPoint;

  cv::KalmanFilter kalman(4,4);
  cv::setIdentity(kalman.transitionMatrix, cv::Scalar(1)); 
  kalman.transitionMatrix.at<float>(0,2)=1; 
  kalman.transitionMatrix.at<float>(1,3)=1;
  cv::setIdentity(kalman.measurementMatrix, cv::Scalar(10)); 
  cv::setIdentity(kalman.processNoiseCov, cv::Scalar(10));
  cv::setIdentity(kalman.measurementNoiseCov, cv::Scalar(10));
  cv::setIdentity(kalman.errorCovPost, cv::Scalar(100));

  cv::Mat_<float>measurement(4,1);

  //  kalman.errorCovPost.at<float>(
  while(video.grab()){
    video.retrieve(image_tmp);


    cv::imshow(filename, image_tmp);	

    int c = cvWaitKey(10);
    if( (char) c == 27 )
      exit(0);
    
    if( (char) c == ' '){
      break;
    }
    imgswap(image,image0);
  }

 while(video.grab()){
    video.retrieve(image_tmp);

    cv::cvtColor(image_tmp, image, CV_BGR2GRAY);
    
    cv::absdiff(image, image0, motion);
    if(find_ant(motion,10, tmpPoint, response)){
      //      if(tmpPoint-antLoc
      antLoc=tmpPoint;
    }

    kalman.predict();
    kalman.correct(measure_ant(motion,10, response,measurement));

    cv::circle(image_tmp,cv::Point((int)kalman.statePost.at<float>(0,0), 
				   (int)kalman.statePost.at<float>(1,0))
	       ,10,cv::Scalar(255.0,0.0,0.5));

    cv::imshow(filename, image_tmp);	

    int c = cvWaitKey(10);
    if( (char) c == 27 )
      break;
    
    imgswap(image,image0);
  }

  return 0;
}
