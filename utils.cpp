#include "utils.hpp"
#include "cv.h"
#include <string>
#include <boost/format.hpp>
#include <fstream>
using boost::format;


using namespace std;
namespace entropy{
  template std::ostream& operator<<(ostream&out, const std::set<int>&);
  template std::ostream& operator<<(ostream&out, const std::set<float>&);
  template std::ostream& operator<<(ostream&out, const std::set<double>&);
  template std::ostream& operator<<(ostream&out, const std::set<std::string>&);

  template std::ostream& operator<<(ostream&out, const std::vector<int>&);
  template std::ostream& operator<<(ostream&out, const std::vector<float>&);
  template std::ostream& operator<<(ostream&out, const std::vector<double>&);
  template std::ostream& operator<<(ostream&out, const std::vector<std::string>&);


  string to_str(int i, int width){
    return str( format("%|05|") % i );
  }

  std::ostream& operator <<(ostream&out, const cv::Mat& m){
    out<<"{ ";
    for(int i=0;i<m.rows;i++){
      out<<"{";
      for(int j=0;j<m.cols;j++){
	switch(m.type()){
	case CV_32FC1: out<<m.at<float>(i,j); break;
	case CV_64FC1: out<<m.at<double>(i,j); break;
	case CV_8UC1: out<<(unsigned int)m.at<unsigned char>(i,j); break;
	case CV_8UC3: 
	  const cv::Vec3b& v = m.at<cv::Vec3b>(i,j);
	  out<<"{"<<v[0]<<", "<<v[1]<<", "<<v[2]<<"}"; 
	  break;
	}

	if(j != m.cols-1) out<<", ";
      }      
      out<<"}";
      if(i != m.rows-1) out<<",\n  ";
    }
    out<<"}";
    return out;
  }
  std::ostream& print(std::ostream&out, const cv::Mat& m){
    return out<<m;
  }

  template<typename Iterator>
  std::ostream& print(std::ostream& out, const Iterator& i_begin, const Iterator& i_end){
    
    Iterator i;
    //    std::set<T>::iterator i_end;
    out<<"{";
    for(i=i_begin; i!=i_end; i++){
      if(i !=i_begin) out<<", ";
      out<<*i;
    }
    out<<"}";
    return out;
  }

 
  template<typename T>
  std::ostream& print(std::ostream& out, const std::set<T>& s){
    return print(out, s.begin(), s.end());
  }

  template<typename T>
  std::ostream& operator<<(std::ostream& out, const std::set<T>& s){
    return print(out,s);
  }


  template<typename T>
  std::ostream& print(std::ostream& out, const std::vector<T>& s){
    return print(out, s.begin(), s.end());
  }

  template<typename T>
  std::ostream& operator<<(std::ostream& out, const std::vector<T>& s){
    return print(out,s);
  }
  
  bool write_mat(std::string filename, const cv::Mat&m){
    std::ofstream out(filename);
    for(int i=0;i<m.rows;i++){
      for(int j=0;j<m.cols;j++){
	switch(m.type()){
	case CV_32FC1: out<<m.at<float>(i,j); break;
	case CV_64FC1: out<<m.at<double>(i,j); break;
	case CV_8UC1: out<<(unsigned int)m.at<unsigned char>(i,j); break;
	case CV_8UC3: 
	  const cv::Vec3b& v = m.at<cv::Vec3b>(i,j);
	  out<<(unsigned int)v[0]<<" "<<(unsigned int)v[1]<<" "<<(unsigned int)v[2]; 
	  break;
	}
	out<<" ";
      }      

      out<<"\n";
    }
    out.close();
    return true;
  }

}
