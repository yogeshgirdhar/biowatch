#include <opencv2/opencv.hpp>
#include <vector>
#include <random>
//#include "multivariate_gaussian.hpp"
using namespace cv;
using namespace std;

template<typename T>
T standardize_angle( T v, T N=180){  
  T r = fmod(v,(2*N));
  r = r >= N ? r-2*N: r; 
  r = r < -N ? r+2*N: r;
  return r;
}

template<typename RNG>
void generate_particles(RotatedRect& obj, int n, double sigma_x, double sigma_y, double sigma_theta, RNG& gen,
						vector<RotatedRect>& particles,
						vector<float>& importance
	){
	particles.clear();
	importance.clear();

	normal_distribution<float> dist_x(0,sigma_x);
	normal_distribution<float> dist_y(0,sigma_y);
	normal_distribution<float> dist_theta(0,sigma_theta);

	float cos_theta = cos(obj.angle/180.0*M_PI);
	float sin_theta = sin(obj.angle/180.0*M_PI);

	for(int i=0; i<n; ++i){
		float x = round(dist_x(gen));
		float y = round(dist_y(gen));
		float theta = round(dist_theta(gen));

		//rotate the points by theta and translate
		float x_t = x*cos_theta - y*sin_theta + obj.center.x;
		float y_t = x*sin_theta + y*cos_theta + obj.center.y;
		float theta_t = standardize_angle<float>(obj.angle + theta);
		particles.push_back(RotatedRect(Point2f(x_t,y_t), obj.size, theta_t));
		float dist = x*x/sigma_x/sigma_x + y*y/sigma_y/sigma_y + theta*theta/sigma_theta/sigma_theta;
		importance.push_back(exp(-dist));
	}
	//return particles;
}

void measurement_importance(vector<RotatedRect>& particles, 
							RotatedRect& obj, 
							cv::Mat_<float>& observation,
							vector<float>& importance){

	vector<float> weights(particles.size(),0);
	for(size_t i=0; i< particles.size(); ++i){
		float cos_theta = cos(particles[i].angle/180.0*M_PI);
		float sin_theta = sin(particles[i].angle/180.0*M_PI);

		for(int x=-obj.size.width/2; x< obj.size.width/2; ++x){
			for(int y=-obj.size.height/2; y<obj.size.height/2; ++y){
				int t_x = x*cos_theta - y*sin_theta + particles[i].center.x;
				int t_y = x*sin_theta + y*cos_theta + particles[i].center.y;
				if(t_y < observation.rows && t_y >= 0 && t_x < observation.cols && t_x >= 0 )
					weights[i]+= observation(t_y,t_x);
			}
		}
		importance[i]*=weights[i];
	}
}
