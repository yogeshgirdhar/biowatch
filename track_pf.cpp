#include <cv.h>
#include <highgui.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include "track.hpp"
#include "filtering.hpp"
using namespace std;
namespace fs = boost::filesystem;
namespace H = entropy;
namespace po = boost::program_options;
using namespace cv;


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
    ("maxlen", po::value<int>()->default_value(20),"Maximum length of the object (post scaling)")
    //("fps",po::value<double>()->default_value(-1),"fps for input")
    ("senstivity", po::value<double>()->default_value(4),"2.0 = low, 4 = mid (default), 10=high")
    ("skip", po::value<int>()->default_value(0),"skip the initial frames")
    ("mask", po::value<string>(),"mask file")
    ("begin", po::value<double>()->default_value(0),"begin time in seconds")
    ("end",  po::value<double>()->default_value(-1.0),"end time in seconds")
    ("out,o", po::value<string>()->default_value("out.txt"),"Output coordinates" )
    ("particles", po::value<int>()->default_value(100), "Number of particles used. Higher number will result in smoother tracking.")
    ("sigma_pos", po::value<double>()->default_value(1.0), "more jumpy the object is, higher this number should be. It is used after multiplying it with max_len")
    ("sigma_theta", po::value<double>()->default_value(30.0), "more irratic the object turning is, higher this number should be.")

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

//current position estimate of the object


vector<RotatedRect> state_at_time;
int max_time;
int current_time;
enum state_type {uninitialized, paused, track_object};
state_type state = uninitialized;

bool show_particles=true, show_measurement=true;

///this function is used to select the object to track
bool selectObject = false;
static void on_mouse( int event, int x, int y, int, void* )
{
    RotatedRect &object = state_at_time[current_time];
    if( selectObject )
    {
      object.angle =  atan2(y - object.center.y, x - object.center.x) /M_PI * 180.0;
    }

    switch( event )
    {
    case CV_EVENT_LBUTTONDOWN:
      selectObject = true;
      cerr<<"Selected object at point: "<<x<<" "<<y<<endl;
      break;
    case CV_EVENT_LBUTTONUP:
        selectObject = false;
        //state=track_object;
        break;
    }
    if(selectObject)
      object.center = Point2f(x,y);
    if(state==uninitialized) state = track_object;

}


void on_time_change(int pos,void* data){
  current_time=pos;
  //object = state_at_time[current_time];
}

void write_output(string filename){
  //initialize the output stream
  std::ofstream out(filename.c_str());
  for(int i=0;i<=max_time; ++i){
    RotatedRect &o = state_at_time[i];
    out<<i<<" "
       <<o.center.x<<" "
       <<o.center.y<<" "
       <<o.size.width<<" "
       <<o.size.height<<" "
       <<o.angle<<" "
       <<"1"<<endl;
  }
}


int main(int argc, char* argv[]){
  po::variables_map ARGS;
  process_program_options(argc,argv, ARGS);
  //int MAX_ANT_AREA=60;
  double senstivity =ARGS["senstivity"].as<double>();
  int max_ant_len = ARGS["maxlen"].as<int>();
  string filename=ARGS["video"].as<string>();
  double begin_time = ARGS["begin"].as<double>();
  double end_time = ARGS["end"].as<double>();
  fs::path pathname(argv[1]);
  std::string dirname  = pathname.parent_path().string();
  //if(dirname=="") 
    dirname=".";
  std::string basename = boost::filesystem::basename (pathname);



  //open the video stram
  cerr<<"Opening "<<filename;
  H::ImageSource video(filename, ARGS["subsample"].as<int>(), ARGS["scale"].as<double>(), ARGS["skip"].as<int>(),false, begin_time, end_time);
  cerr<<(video.isOpened()?": OK":": FAIL")<<endl;
  double frame_count = video.source().get(CV_CAP_PROP_FRAME_COUNT);
  cerr<<"Number of frames: "<<frame_count<<endl;
  max_time = frame_count-1;
  if(frame_count==0){
    cerr<<"Error. frame count is 0."<<endl;
    return 0;
  }

  //initialize the size of the object to track
  Size2f object_size(max_ant_len, max_ant_len/4.0);

  //resize the state vector to hold posision for each time step
  state_at_time.resize(frame_count,RotatedRect(Point2f(0,0),object_size,0));


  cv::Mat image, image_tmp;
  cv::Mat_<float> image_fp(image_tmp.size());
  cv::Mat mask,mask_fp;


  //load the image mask
  if(ARGS.count("mask")){
    cerr<<"Using mask: "<<ARGS["mask"].as<string>()<<endl;
    mask = cv::imread(ARGS["mask"].as<string>(),0);
    assert(!mask.empty());
    mask.convertTo(mask_fp, CV_32F, 1.0/256.0);
    mask=mask_fp;
  }

  //save the first image as the background image
  video.grab();
  video.retrieve(image_tmp);
  cv::imwrite(dirname+"/"+basename+"_bg.jpg",image_tmp);
  cv::cvtColor(image_tmp, image, CV_BGR2GRAY);
  image.convertTo(image_fp, CV_32F, 1.0/256.0);






  
  
  //initialize mouse callback for selecting objects
  cv::namedWindow(basename, CV_WINDOW_AUTOSIZE);	
  setMouseCallback(basename, on_mouse, 0 );
  createTrackbar("time", basename, &current_time, max_time, on_time_change); 

  //random number generator
  std::mt19937 gen;
  current_time=0;
  int last_time=-1;
  cv::Mat image_status;
  Mat_<float> observation;

  while(true){
    if(state != paused){
      if(current_time >= max_time){
        current_time = max_time;
        state=paused;
      }
      else{
        current_time++; 
      }
      setTrackbarPos("time",basename,current_time);
    }

    if(last_time != current_time){
      video.source().set(CV_CAP_PROP_POS_FRAMES, current_time);
      if(!video.grab())
        break;
      video.retrieve(image_tmp);
      cv::cvtColor(image_tmp, image, CV_BGR2GRAY);
      image.convertTo(image_fp, CV_32F, 1.0/256.0);
      
      image_status = image_tmp.clone();
         
      cv::Point minLoc, maxLoc;     double minv, maxv;
      cv::minMaxLoc(image_fp,&minv, &maxv, &minLoc, &maxLoc);
      double T = minv+(maxv-minv)/senstivity;
      cv::threshold(image_fp,observation, T, 1.0, CV_THRESH_TRUNC);
      observation =  1.0 - (observation - minv)/(T-minv);
      if(show_measurement)
        cv::imshow("z", observation);	
    }



    if(state == track_object){
      //Mat_<float> z2 = posterior_dist(observation,object);
      //cv::imshow("z2", z2); 

      //generate particles
      vector<RotatedRect> particles;
      vector<float> importance;
      generate_particles(state_at_time[current_time-1],
                        ARGS["particles"].as<int>(),
                        ARGS["sigma_pos"].as<double>()*state_at_time[current_time].size.width, 
                        ARGS["sigma_pos"].as<double>()*state_at_time[current_time].size.height, 
                        ARGS["sigma_theta"].as<double>(), 
                        gen, particles, importance);
      measurement_importance(particles, state_at_time[current_time-1], observation, importance);
      float imp_max = *max_element(importance.begin(), importance.end());
      float imp_min = *min_element(importance.begin(), importance.end());

      if(show_particles){
        for(size_t i=0;i<particles.size(); ++i){
          cv::ellipse(image_status, particles[i], cv::Scalar(255*importance[i]/imp_max,0,0));
        }
      }
      //maximum likelikiid estimate
      state_at_time[current_time]=particles[max_element(importance.begin(), importance.end())  - importance.begin()];
    }

    cv::ellipse(image_status, state_at_time[current_time], cv::Scalar(0,255,0));
    cv::imshow(basename, image_status); 

    last_time = current_time;

    //handle keyboard input
    int c = cvWaitKey(5);
    switch(c){
      case 27:
        cerr<<"Writing output to:"<<ARGS["out"].as<string>()<<endl;
        write_output(ARGS["out"].as<string>());
        return 0; break;

      case 'o':
        cerr<<"Writing output to:"<<ARGS["out"].as<string>()<<endl;
        write_output(ARGS["out"].as<string>());
        break;

      case 'b':
        cerr<<"Writing Background image"<<endl;
        cv::imwrite(dirname+"/"+basename+"_bg.jpg",image_tmp);
        break;

      case ' ':
        if(state == paused){
          state = track_object;
        }
        else{
          state = paused;
        }
        break;

      case 'p':
        show_particles = !show_particles; 
        break;

      case 's': //step one frame if paused
        if(state==paused){
          current_time = min(current_time+1, max_time);
        }
        break;

      case ',':
      case '<':
        if(state == paused){
  	     state_at_time[current_time].angle += 7.5;
        }
        break;

      case '.':
      case '>':
        if(state == paused){
         state_at_time[current_time].angle -= 7.5;
        }
        break;
    }
  }

  return 0;
}
