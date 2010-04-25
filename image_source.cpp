#include "track.hpp"
#include <cv.h>
#include <highgui.h>
#include <iostream>
using namespace std;
namespace entropy{
  
  ImageSource::ImageSource(const std::string& filename, int subsample, double scale, int skip, bool gray):
    m_subsample(subsample), 
    m_video_capture(filename),
    m_scale(scale),
    m_gray(gray),
    m_skip(skip)
  {
    m_num_frames=m_video_capture.get(CV_CAP_PROP_FRAME_COUNT);
    cerr<<"Num frames="<<m_num_frames<<" subsampled to "<<(int)m_num_frames/subsample<<endl;
    m_curr_framenum=0;
  }
  ImageSource::ImageSource(int camera_id, int subsample, double scale, int skip, bool gray):
    m_subsample(subsample), 
    m_video_capture(camera_id),
    m_scale(scale),
    m_gray(gray),
    m_skip(skip)
  {
    //    m_video_capture.set(CV_CAP_PROP_FPS,1);
    m_num_frames=-1;
    m_curr_framenum=0;
  }
  bool ImageSource::isOpened(){
    return m_video_capture.isOpened();
  }
  bool ImageSource::grab(){
    bool r;
    for(m_skip;m_skip>0;m_skip--){
      r=m_video_capture.grab();
      m_curr_framenum++;
    }
    for(int i=0;i<m_subsample;i++){
      r=m_video_capture.grab();
      m_curr_framenum++;
    }

    if(r)
      m_has_new_frame=true;
    return r;
  }

  int ImageSource::frame_number(){
    return m_curr_framenum;
  }
  bool ImageSource::retrieve(cv::Mat& img){
    //if(m_has_new_frame){
      //      cv::Mat img;
      cv::Mat img_out;
      bool r;
      r=m_video_capture.retrieve(img);      
      if(m_scale !=1.0){
	cv::resize(img,img_out,cv::Size(),m_scale, m_scale);
	img = img_out;
      }
      if(m_gray){
	cv::cvtColor(img,img_out,CV_RGB2GRAY);//gray image
	cv::equalizeHist(img_out, img_out);
	img=img_out;
      }
      m_has_new_frame=false;    
      m_current_frame=img;
      return r;
      //    }
      //    return m_current_frame;
  }
}
