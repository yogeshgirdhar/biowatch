#include <opencv2/opencv.hpp>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include "track.hpp"
#include "filtering.hpp"
#include "colors.hpp"
using namespace std;
namespace fs = boost::filesystem;
namespace H = entropy;
namespace po = boost::program_options;
using namespace cv;


void print_runtime_help(){
  cerr<<"Keyboard control:"<<endl
      <<setw(10)<<"[space]: "<<"toggle pausing"<<endl
      <<setw(10)<<"[delete]: "<<"delete selected object"<<endl
      <<setw(10)<<"[<],[>]: "<<"rotate selected object"<<endl
      <<setw(10)<<"[w],[s]: "<<"change selected tracked target position UP/DOWN"<<endl
      <<setw(10)<<"[a],[d]: "<<"change selected tracked target position LEFT/RIGHT"<<endl
      <<setw(10)<<"[r],[f]: "<<"change selected tracked target size"<<endl
      <<setw(10)<<"[z]: "<<"show measurement"<<endl
      <<setw(10)<<"[p]: "<<"show particles"<<endl
      <<setw(10)<<"[o]: "<<"write output"<<endl
      <<setw(10)<<"[escape]: "<<"write output and quit"<<endl;

  cerr<<endl
      <<"Mouse control:"<<endl
      <<"\t- double click or right click to add a new object"<<endl
      <<"\t- While paused, drag and object to manually update position"<<endl;

  cerr<<endl;
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
    ("owidth", po::value<int>()->default_value(20),"Maximum width of the object (post scaling)")
    ("oheight", po::value<int>()->default_value(4),"Maximum height of the object (post scaling)")
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
    ("update_appearance", po::value<bool>()->default_value(true), "continusouly update the appearance model of the tracked objects.")
    ("observation", po::value<string>()->default_value("black"), "what kind of observation to track: black, HV")

    ;

  po::positional_options_description pos_desc;
  pos_desc.add("video", -1);
  

  po::store(po::command_line_parser(argc, argv).options(desc).positional(pos_desc).run(), args);
  //po::store(po::command_line_parser(argc, argv).options(desc).run(), args);
  po::notify(args);    

  if (args.count("help")) {
    cout << desc << "\n";
    cout << "While running you can use the following keys to control the application: "<<endl;
    print_runtime_help();
    exit(0);
  }
  return 0;
}

//current position estimate of the object


vector<vector<RotatedRect> > state_at_time;
vector<vector<bool> > is_tracked;
vector<MatND> appearance_model; //color histogram for each object being tracked
int max_time;
int current_time;
Size2f object_size;  //size of the tracked object
enum state_type {uninitialized, paused, track_object};
state_type state = uninitialized;
cv::Mat_<Vec3b> current_image_hsv;
bool show_particles=false, show_measurement=false;
// hue varies from 0 to 179, see cvtColor
float hranges[] = { 0, 180 };
// saturation varies from 0 (black-gray-white) to
// 255 (pure spectrum color)
float sranges[] = { 0, 256 };
float vranges[] = { 0, 256 };
//const float* ranges[] = { hranges, sranges, vranges };
const float* ranges[] = {hranges,sranges, vranges };
// we compute the histogram from the 0-th and 1-st channels
int channels[] = {0,2};
int num_channels = 2;


int find_object_at_coords(int x, int y){
  int i;
  Point2i p(x,y);
  for(i=0; i< state_at_time.size(); ++i){
    if(state_at_time[i][current_time].boundingRect().contains(p))
      return i;
  }
  return -1;
}

template<typename T>
void extract_roi(cv::Mat_<T>& image, cv::Mat_<T>& roi, const RotatedRect& obj){
  float cos_theta = cos(obj.angle/180.0*M_PI);
  float sin_theta = sin(obj.angle/180.0*M_PI);
  roi.create(static_cast<int>(obj.size.height), static_cast<int>(obj.size.width));
  float width2 = roi.cols/2.0;
  float height2 = roi.rows/2.0;
  //for each pixel covered by the object, do bincounts
  for(int i=0; i< roi.rows; ++i){
    for(int j=0; j< roi.cols ; ++j){
      float x = j - width2; float y = i - height2;
      int t_x = round(x*cos_theta - y*sin_theta + obj.center.x);
      int t_y = round(x*sin_theta + y*cos_theta + obj.center.y);
      if(t_y < image.rows && t_y >= 0 && t_x < image.cols && t_x >= 0 )
        roi(i,j) = image(t_y,t_x);
      else
        roi(i,j) = T();
    }
  }
}

//update the appearance model of the given object by looking at its corrent position
MatND get_appearance_model(cv::Mat_<Vec3b>& hsv_image, RotatedRect& obj){

  cv::Mat_<Vec3b> roi;
  extract_roi(hsv_image, roi, obj);
  int hbins = 30, sbins = 32, vbins=32;
  int histSize[] = {hbins, sbins, vbins};
  MatND hist;


  calcHist( &roi, 1, channels, Mat(), // do not use mask
            hist, num_channels, histSize, ranges,
            true, // the histogram is uniform
            false );

  return hist;
  //calcHist(tracked_roi, 1, 0, 0, hist, 1, &hsize, &phranges);


  /*float cos_theta = cos(obj.angle/180.0*M_PI);
  float sin_theta = sin(obj.angle/180.0*M_PI);

  //for each pixel covered by the object, do bincounts
  for(int x=-obj.size.width/2; x< obj.size.width/2; ++x){
    for(int y=-obj.size.height/2; y<obj.size.height/2; ++y){
      int t_x = x*cos_theta - y*sin_theta + obj.center.x;
      int t_y = x*sin_theta + y*cos_theta + obj.center.y;
      if(t_y < observation.rows && t_y >= 0 && t_x < observation.cols && t_x >= 0 )
        weights[i]+= observation(t_y,t_x);
    }
    }*/
}

cv::Mat_<float> make_observation(cv::Mat& image, double senstivity, int object_id){
  cv::Mat_<float> image_fp, observation;
  //cv::Mat image;
  //  cv::cvtColor(image_tmp, image, CV_BGR2GRAY);
  image.convertTo(image_fp, CV_32F, 1.0/256.0);
  
  cv::Point minLoc, maxLoc;     double minv, maxv;
  cv::minMaxLoc(image_fp,&minv, &maxv, &minLoc, &maxLoc);
  double T = minv+(maxv-minv)/senstivity;
  cv::threshold(image_fp,observation, T, 1.0, CV_THRESH_TRUNC);
  observation =  1.0 - (observation - minv)/(T-minv);
  return observation;
}

cv::Mat_<float> make_observation_hsv(cv::Mat& image_hsv, double senstivity, int object_id){
  cv::Mat_<float> image_fp, observation;
  //cv::Mat image;
  //  cv::cvtColor(image_tmp, image, CV_BGR2GRAY);
  //image.convertTo(image_fp, CV_32F, 1.0/256.0);
  cv::Mat response;
  calcBackProject(&image_hsv, 1, channels, appearance_model[object_id], response, ranges);
  response.convertTo(image_fp, CV_32F, 1.0/256.0);

  cv::Point minLoc, maxLoc;     double minv, maxv;
  cv::minMaxLoc(image_fp,&minv, &maxv, &minLoc, &maxLoc);
  double T = minv+(maxv-minv)/senstivity;
  cv::threshold(image_fp,observation, T, 1.0, CV_THRESH_TRUNC);
  observation =  (observation - minv)/(T-minv);

  //cv::imshow("hsvresponse", observation);
  /*  cv::Point minLoc, maxLoc;     double minv, maxv;
  cv::minMaxLoc(image_fp,&minv, &maxv, &minLoc, &maxLoc);
  double T = minv+(maxv-minv)/senstivity;
  cv::threshold(image_fp,observation, T, 1.0, CV_THRESH_TRUNC);
  observation =  1.0 - (observation - minv)/(T-minv);*/
  return observation;
}

//add a new object at x,y,angle to be tracked
int add_tracked_object(float x, float y, Size2f o_size, float o_angle=0){
  int oi=state_at_time.size();
  cerr<<"Adding new object: "<<oi<<endl;

  state_at_time.push_back(vector<RotatedRect>(max_time+1,RotatedRect(Point2f(x,y),o_size,o_angle)));
  is_tracked.push_back(vector<bool>(max_time+1,false));
  is_tracked[oi][current_time]=true;
  //  state_at_time[oi][current_time].center=Point2f(x,y);

  MatND appearance = get_appearance_model(current_image_hsv, state_at_time[oi][current_time]);
  appearance_model.push_back(appearance);

  return state_at_time.size()-1;  
}

int remove_tracked_object(int oi){
  assert(oi < state_at_time.size());

  state_at_time.erase(state_at_time.begin()+oi);
  is_tracked.erase(is_tracked.begin()+oi);
  appearance_model.erase(appearance_model.begin()+oi);

  cerr<<"Removed object: "<<oi<<endl;
  return state_at_time.size()-1;  
}

///this function is used to select the object to track
int selected_object = -1;
bool dragging=false;
static void on_mouse( int event, int x, int y, int, void* )
{
//    RotatedRect &object = state_at_time[current_time];
    //if( selected_object )
    //{
     // object.angle =  atan2(y - object.center.y, x - object.center.x) /M_PI * 180.0;
    //}

    switch( event )
    {
      //add a new object to track
      case CV_EVENT_LBUTTONDBLCLK:
      case CV_EVENT_RBUTTONDOWN:
        selected_object = add_tracked_object(x,y, object_size, 0);
        break;

      case CV_EVENT_LBUTTONDOWN:
        selected_object = find_object_at_coords(x,y);
        cerr<<"Selected object at point: "<<x<<" "<<y<<" : "<<selected_object<<endl;
        dragging=true;
        break;
      case CV_EVENT_LBUTTONUP:
        //selected_object = -1;
        //state=track_object;
        dragging=false;
        break;
      case CV_EVENT_MOUSEMOVE:
        if(dragging && selected_object>=0){
          state_at_time[selected_object][current_time].center = Point2f(x,y);
          appearance_model[selected_object]=get_appearance_model(current_image_hsv,  state_at_time[selected_object][current_time]);
        }
        break;
    }
   
    if(state==uninitialized) state = paused;

}


void on_time_change(int pos,void* data){
  current_time=pos;
  //object = state_at_time[current_time];
}

void write_output(string filename){
  //initialize the output stream
  std::ofstream out(filename.c_str());
  for(int oi=0; oi< state_at_time.size(); ++oi){
    for(int i=0;i<=max_time; ++i){
      RotatedRect &o = state_at_time[oi][i];
      out<<oi<<" "<<i<<" "
         <<o.center.x<<" "
         <<o.center.y<<" "
         <<o.size.width<<" "
         <<o.size.height<<" "
         <<o.angle<<" "
         <<static_cast<int>(is_tracked[oi][i])<<endl;
    }
  }
}


int main(int argc, char* argv[]){
  po::variables_map ARGS;
  process_program_options(argc,argv, ARGS);
  //int MAX_ANT_AREA=60;
  double senstivity =ARGS["senstivity"].as<double>();
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
  object_size = Size2f(ARGS["owidth"].as<int>(), ARGS["oheight"].as<int>());

  //resize the state vector to hold posision for each time step
//  state_at_time.resize(frame_count,RotatedRect(Point2f(0,0),object_size,0));


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

  print_runtime_help();
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
      
    }
    image_status = image_tmp.clone();
    if(state==uninitialized) state=paused;

    if(state == track_object){

      //split the HSV color channels
      cv::Mat image_hsv;
      cv::Mat_<uchar> image_hue(image_tmp.size()) , image_saturation(image_tmp.size()), image_value(image_tmp.size());
      cv::cvtColor(image_tmp,image_hsv,CV_BGR2HSV);
      current_image_hsv = image_hsv;
      cv::Mat splitchannels[]={image_hue,image_saturation,image_value};
      cv::split(image_hsv,splitchannels);
      //imshow("img_hue",image_hue);
      //imshow("img_sat",image_saturation);
      //imshow("img_value",image_value);
      //generate particles for each object and select the max likelihood particle
      for(size_t oi=0; oi< state_at_time.size() ; ++oi){
        vector<RotatedRect> particles;
        vector<float> importance;
        generate_particles(state_at_time[oi][current_time-1],
                          ARGS["particles"].as<int>(),
                          ARGS["sigma_pos"].as<double>()*state_at_time[oi][current_time].size.width, 
                          ARGS["sigma_pos"].as<double>()*state_at_time[oi][current_time].size.height, 
                          ARGS["sigma_theta"].as<double>(), 
                          gen, particles, importance);
        
        if(ARGS["observation"].as<string>()=="black")
          observation = make_observation(image_value,senstivity,oi);
        else
          observation = make_observation_hsv(image_hsv, senstivity, oi);
        if(show_measurement)  cv::imshow("z", observation);

        measurement_importance(particles, state_at_time[oi][current_time-1], observation, importance);
        float imp_max = *max_element(importance.begin(), importance.end());
        float imp_min = *min_element(importance.begin(), importance.end());

        if(show_particles){
          for(size_t i=0;i<particles.size(); ++i){
            cv::ellipse(image_status, particles[i], cv::Scalar(255*importance[i]/imp_max,0,0));
          }
        }
        //maximum likelikiid estimate
        state_at_time[oi][current_time]=particles[max_element(importance.begin(), importance.end())  - importance.begin()];
        is_tracked[oi][current_time]=true;

        //update the visual model
        Mat_<Vec3b> tracked_roi, img_=image_hsv;
        extract_roi(img_, tracked_roi, state_at_time[oi][current_time]); 
        
        if(ARGS["update_appearance"].as<bool>())
          appearance_model[oi]=get_appearance_model(current_image_hsv, state_at_time[oi][current_time]);

         //      calcHist(tracked_roi, 1, 0, maskroi, hist, 1, &hsize, &phranges);
         //imshow("tracked_roi",tracked_roi);
      }
    }
    else if(state==paused){
      if(selected_object>=0)
        cv::ellipse(image_status, state_at_time[selected_object][current_time], get_color(selected_object),3);      
    }

    //draw the current position of each tracked object
    int colorid=0;
    for(auto &object: state_at_time){
      cv::ellipse(image_status, object[current_time], get_color(colorid++));
    }
    cv::imshow(basename, image_status); 

    last_time = current_time;

    //handle keyboard input
    int c = cvWaitKey(5) & 255;
    bool update_appearance=false;
    //if (c != -1) cerr<<"pressed_key: "<<c<<endl;
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
        cerr<<"Writing Background image: "<<dirname+"/"+basename+"_bg.jpg"<<endl;
        cv::imwrite(dirname+"/"+basename+"_bg.jpg",image_tmp);
        break;

      case ' ':
        if(state == paused){
          state = track_object;
          cerr<<"Unpaused"<<endl;
        }
        else{
          state = paused;
          cerr<<"Paused"<<endl;
        }
        break;

      case 'p':
        show_particles = !show_particles; 
        break;
      case 'z':
        show_measurement = !show_measurement; 
        break;

      case 8:
        if(state == paused){
          if(selected_object>=0)
            remove_tracked_object(selected_object);
        }
        break;

      case 'n': //step one frame if paused
        if(state==paused){
          current_time = min(current_time+1, max_time);
        }
        break;

      case ',':
      case '<':
        if(state == paused){
          if(selected_object>=0)
               state_at_time[selected_object][current_time].angle += 7.5;
          update_appearance=true;
        }
        break;

      case '.':
      case '>':
        if(state == paused){
          if(selected_object>=0)
            state_at_time[selected_object][current_time].angle -= 7.5;
          update_appearance=true;
        }
        break;

    case 'w': //up
        if(state == paused){
          if(selected_object>=0)
            state_at_time[selected_object][current_time].center.y -= 1;
          update_appearance=true;
        }
        break;
    case 's': //down
        if(state == paused){
          if(selected_object>=0)
            state_at_time[selected_object][current_time].center.y += 1;
          update_appearance=true;
        }
        break;
    case 'd': //right
        if(state == paused){
          if(selected_object>=0)
            state_at_time[selected_object][current_time].center.x += 1;
          update_appearance=true;
        }
        break;
    case 'a': //left
        if(state == paused){
          if(selected_object>=0)
            state_at_time[selected_object][current_time].center.x -= 1;
          update_appearance=true;
        }
        break;
    case 'r'://increase size
      if(selected_object>=0){
        state_at_time[selected_object][current_time].size.width *= 1.1;
        state_at_time[selected_object][current_time].size.height *= 1.1;
      }
      break;
    case 'f'://decrease size
      if(selected_object>=0){
        state_at_time[selected_object][current_time].size.width /= 1.1;
        state_at_time[selected_object][current_time].size.height /= 1.1;
      }
      break;


    default:
      if (c != -1 && c!=255) cerr<<"pressed_key: "<<c<<endl;
    }
    if(update_appearance && ARGS["observation"].as<string>()!="black"){
      appearance_model[selected_object]=get_appearance_model(current_image_hsv, state_at_time[selected_object][current_time]);
      observation = make_observation_hsv(current_image_hsv, senstivity, selected_object);
      if(show_measurement)  cv::imshow("z", observation);
    }
  }

  return 0;
}
