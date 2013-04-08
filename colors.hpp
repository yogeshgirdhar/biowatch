
cv::Mat get_colors(int n_colors=32, int value=255){
  cv::Mat bgr_colors, hsv_colors;
  hsv_colors.create(n_colors,1,CV_8UC3);
  bgr_colors.create(n_colors,1,CV_8UC3);
  for(int i=0;i<n_colors; ++i){
    hsv_colors.at<cv::Vec3b>(i,0) = cv::Vec3b(160/n_colors*i,255,value);
  }
  cv::cvtColor(hsv_colors, bgr_colors, CV_HSV2BGR);
  return bgr_colors;
}
cv::Scalar get_color(int i){
  static cv::Mat bgr_colors_high = get_colors(16,255);
  static cv::Mat bgr_colors_low = get_colors(16,127);
  i = i%32;
  if(i<16){
    return cv::Scalar (bgr_colors_high.at<cv::Vec3b>(i,0)[0],
		       bgr_colors_high.at<cv::Vec3b>(i,0)[1],
		       bgr_colors_high.at<cv::Vec3b>(i,0)[2]);
  }
  else{
    return cv::Scalar (bgr_colors_low.at<cv::Vec3b>(i-16,0)[0],
		       bgr_colors_low.at<cv::Vec3b>(i-16,0)[1],
		       bgr_colors_low.at<cv::Vec3b>(i-16,0)[2]);
  }
}
