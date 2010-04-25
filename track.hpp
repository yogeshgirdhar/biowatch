#ifndef TRACK_HPP_239384
#define TRACK_HPP_239384
#include <cv.h>
#include <highgui.h>
#include <string>






namespace entropy{
  class ImageSource{
  public:
    ImageSource(const std::string& filename, int subsample=1, double scale=1.0, int skip=0, bool gray=true);
    ImageSource(int camera_id, int subsample=1, double scale=1.0, int skip=0, bool gray=true);
    bool grab();
    bool retrieve(cv::Mat&);
    bool isOpened();
    int frame_number();
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
  };
}

#endif
