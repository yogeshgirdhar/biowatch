#include "track.hpp"
#include <iostream>
using namespace std;
namespace entropy{
  
  ImageSource::ImageSource(const std::string& filename, int subsample, double scale, int skip, bool gray, double begin_time, double end_time):
    m_subsample(subsample), 
    m_video_capture(filename),
    m_scale(scale),
    m_gray(gray),
    m_skip(skip),
    m_begin_time(begin_time),
    m_end_time(end_time)
  {
    m_num_frames=m_video_capture.get(CV_CAP_PROP_FRAME_COUNT);
    cerr<<"Num frames in the video="<<m_num_frames<<" subsampled to "<<(int)m_num_frames/subsample<<endl;
    m_curr_framenum=0;
    m_video_capture.set(CV_CAP_PROP_POS_MSEC, m_begin_time*1000.0);    
  }
  ImageSource::ImageSource(int camera_id, int subsample, double scale, int skip, bool gray):
    m_subsample(subsample), 
    m_video_capture(camera_id),
    m_scale(scale),
    m_gray(gray),
    m_skip(skip),
    m_begin_time(0.0),
    m_end_time(-1.0)
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
    if(m_end_time>0){
      double pos = m_video_capture.get(CV_CAP_PROP_POS_MSEC)/1000.0;
      if(pos > m_end_time)
	return false;
    }
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
