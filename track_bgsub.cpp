#include <cv.h>
#include <highgui.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include "track.hpp"
using namespace std;
namespace fs = boost::filesystem;
namespace H = entropy;
namespace po = boost::program_options;

void imgswap(cv::Mat& img1, cv::Mat& img2){
  cv::Mat img3;
  img3=img1;
  img1=img2;
  img2=img3;
}

///process program options..
///output is variables_map args
int process_program_options(int argc, char*argv[], po::variables_map& args){
  po::options_description desc("Tracks an ant. \nOutputs results in the same directory as the input filename.\nHit b to take background image.\n\n Options are");
  desc.add_options()
    ("help", "help")
    ("video", po::value<string>(),"Video file")
    //("camera", po::value<int>(),"Camera: device id")
    ("scale", po::value<double>()->default_value(1),"scaling")
    ("subsample", po::value<int>()->default_value(1),"Temporal subsampling rate for input video.")
    ("maxlen", po::value<int>()->default_value(15),"Maximum length of the object (post scaling)")
    //("fps",po::value<double>()->default_value(-1),"fps for input")
    ("senstivity", po::value<double>()->default_value(4),"2 = low, 4 = mid (default), 10=high")
    ("skip", po::value<int>()->default_value(0),"skip the initial frames")
    ("mask", po::value<string>(),"mask file")
    ;

  po::positional_options_description pos_desc;
  pos_desc.add("video", -1);
  

  po::store(po::command_line_parser(argc, argv).options(desc).positional(pos_desc).run(), args);
  //po::store(po::command_line_parser(argc, argv).options(desc).run(), args);
  po::notify(args);    

  if (args.count("help")) {
    cout << desc << "\n";
    exit(0);
  }
  return 0;
}

struct cmp_dist{
  cmp_dist(const cv::Point2f& p):m_p(p)
  {}
  cv::Point2f m_p;
  bool operator()(const cv::Point2f& p1, const cv::Point2f& p2)const{
    return cv::norm(p1-m_p) < cv::norm(p2-m_p);    
  }
};
void clean_points(std::vector<cv::Point2f>& points, cv::Point2f loc, int maxlen){
  if(points.size() <2)
    return;
  std::sort(points.begin(), points.end(),cmp_dist(loc));
  int end=0;
  for(int i=1;i<points.size();i++){
    if(cv::norm(points[i]-points[i-1]) > maxlen){
      end=i;
      break;
    }
  }
  if(end!=0){
    points.erase(points.begin()+end, points.end());
  }
}

int main(int argc, char* argv[]){
  po::variables_map ARGS;
  process_program_options(argc,argv, ARGS);
  //int MAX_ANT_AREA=60;
  double senstivity =ARGS["senstivity"].as<double>();
  int max_ant_len = ARGS["maxlen"].as<int>();
  string filename=ARGS["video"].as<string>();
  fs::path pathname(argv[1]);
  std::string dirname  = pathname.parent_path().string();
  if(dirname=="") dirname=".";
  std::string basename = boost::filesystem::basename (pathname);

  cerr<<"Opening "<<filename;

  //cv::VideoCapture video(filename);
  H::ImageSource video(filename, ARGS["subsample"].as<int>(), ARGS["scale"].as<double>(), ARGS["skip"].as<int>(),false);

  cerr<<(video.isOpened()?": OK":": FAIL")<<endl;

  cv::Mat image, image_tmp, image_rgb_last, motion;
  cv::Mat_<float> accum_image(image_tmp.size(),0.0);
  //  cv::Mat_<float> accum_image_01(image_tmp.size(),0.0);
  cv::Mat_<float> image_fp(image_tmp.size());
  cv::Mat_<float> image_sans_obs(image_tmp.size());
  cv::Mat_<float> observation(image_tmp.size(),0.0);
  cv::Mat_<float> observationT(image_tmp.size(),0.0);
  cv::Mat_<float> dilated_observation(image_tmp.size(),0.0);
  cv::Mat mask,mask_fp;

  if(ARGS.count("mask")){
    cerr<<"Using mask: "<<ARGS["mask"].as<string>()<<endl;
    mask = cv::imread(ARGS["mask"].as<string>(),0);
    assert(!mask.empty());
    mask.convertTo(mask_fp, CV_32F, 1.0/256.0);
    mask=mask_fp;
  }

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


  cv::namedWindow(basename, CV_WINDOW_AUTOSIZE);	
  cv::namedWindow("bg", CV_WINDOW_AUTOSIZE);	
  cv::namedWindow("ant", CV_WINDOW_AUTOSIZE);	


  std::ofstream out((dirname+"/"+basename+"_data.txt").c_str());
  bool locked=false;
  bool first=true;
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
    observation = observation.mul(mask);

    double min, max;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(image_fp,&min, &max, &minLoc, &maxLoc);
    double T = min+(max-min)/senstivity;
    cv::threshold(observation,observationT, T, 1.0, CV_THRESH_BINARY);
    points.clear();
    state=cv::Point2f(0,0);
    for(int r=0;r<observation.rows;r++){
      for(int c=0;c<observation.cols;c++){
	if(observationT(r,c)==1.0){
	  points.push_back(cv::Point2f(c,r));
	  //state = state + 
	}
      }
    }
    //if(!first)clean_points(points,last_locked_rect.center, max_ant_len);
    //cerr<<"points="<<points.size()<<endl;
    if(points.size()>=1){
      rect = cv::minAreaRect(cv::Mat(points));
      rect.angle+=90;

      //if(rect.size.width*rect.size.height < MAX_ANT_AREA)
      if(rect.size.width < max_ant_len && rect.size.height < max_ant_len)
	locked=true;
      else
	locked=false;

      if(locked){
	cv::ellipse(image_tmp, rect, cv::Scalar(0,255,0));
	last_locked_rect=rect;
	first=false;
      }
      else{
	cv::ellipse(image_tmp, last_locked_rect, cv::Scalar(255,0,0));
	cv::ellipse(image_tmp, rect, cv::Scalar(0,0,255));
      }
    }
    out<<video.frame_number()<<" "
       <<last_locked_rect.center.x<<" "
       <<(float)image_tmp.rows-last_locked_rect.center.y<<" "
       <<last_locked_rect.size.width<<" "
       <<last_locked_rect.size.height<<" "
       <<last_locked_rect.angle<<" "
       <<(int)locked<<endl;
    cerr<<video.frame_number()<<" "
	<<last_locked_rect.center.x<<" "
	<<(float)image_tmp.rows-last_locked_rect.center.y<<" "
	<<last_locked_rect.size.width<<" "
	<<last_locked_rect.size.height<<" "
	<<last_locked_rect.angle<<" "
	<<(int)locked<<endl;


    cv::imshow(basename, image_tmp);	

    //cv::imshow("bg", accum_image);	
    //cv::imshow("ant", observationT);	


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
