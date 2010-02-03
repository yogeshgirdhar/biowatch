#ifndef ENTROPY_UTILS_HPP
#define ENTROPY_UTILS_HPP
#include <iostream>
#include "cv.h"
#include <set>
#include <vector>
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/foreach.hpp>
#include "boost/tuple/tuple_comparison.hpp"
#include "boost/tuple/tuple_io.hpp"
#include <boost/shared_ptr.hpp>

using namespace boost::lambda;
//using boost::tuple;
#define foreach BOOST_FOREACH

namespace entropy{
  std::ostream& operator <<(std::ostream&out, const cv::Mat& m);
  std::ostream& print(std::ostream&out, const cv::Mat& m);

  template<typename T>
  std::ostream& print(std::ostream& out, const std::set<T>& s);
  template<typename T>
  std::ostream& operator<<(std::ostream& out, const std::set<T>& s);

  template<typename T>
  std::ostream& print(std::ostream& out, const std::vector<T>& s);
  template<typename T>
  std::ostream& operator<<(std::ostream& out, const std::vector<T>& s);

  std::string to_str(int i, int width=0);

  template<typename T>
  int argmax(const std::vector<T> &v){
    T max_val=v[0];
    int max_i=0;
    for(int i=1;i<v.size();i++){
      if(max_val < v[i]){
	max_i=i;
	max_val=v[i];
      }
    }
    return max_i;
  }

  template<typename T>
  int argmin(const std::vector<T> &v){
    T max_val=v[0];
    int max_i=0;
    for(int i=1;i<v.size();i++){
      if(max_val > v[i]){
	max_i=i;
	max_val=v[i];
      }
    }
    return max_i;
  }
  /// Write a matrix in text format 
  bool write_mat(std::string filename, const cv::Mat&);  
}

#endif
