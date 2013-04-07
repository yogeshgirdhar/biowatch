#ifndef TRACK_HPP_239384
#define TRACK_HPP_239384
#include <cv.h>
#include <highgui.h>
#include <string>






namespace entropy{
  class ImageSource{
  public:
    //begin_time = sart time in seconds
    //end_time = end time in seconds, -1 for no end time.
    ImageSource(const std::string& filename, int subsample=1, double scale=1.0, int skip=0, bool gray=true, double begin_time=0.0, double end_time=-1.0);
    ImageSource(int camera_id, int subsample=1, double scale=1.0, int skip=0, bool gray=true);
    bool grab();
    bool retrieve(cv::Mat&);
    bool isOpened();
    int frame_number();
    cv::VideoCapture& source(){return m_video_capture;}
  protected:
    int m_skip;
    cv::VideoCapture m_video_capture;
    double m_scale;
    double m_subsample;
    int m_num_frames;
    cv::Mat m_current_frame;
    int m_curr_framenum;
    bool m_has_new_frame;
    bool m_gray;
    double m_begin_time, m_end_time;
  };
}

#endif
