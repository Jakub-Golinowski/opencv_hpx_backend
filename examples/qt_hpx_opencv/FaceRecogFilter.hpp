#ifndef FACERECOGFILTER_H
#define FACERECOGFILTER_H

//#include "opencv2/core/core_c.h"
//#include "opencv2/core/core.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
// Boost Accumulators
//#include <boost/accumulators/accumulators.hpp>
//#include <boost/accumulators/statistics/stats.hpp>
//#include <boost/accumulators/statistics/rolling_mean.hpp>
//#include <boost/accumulators/statistics/median.hpp>
//#include <boost/accumulators/statistics/weighted_median.hpp>

//class PSNRFilter;
//class Filter;

//
// Filter which computes some motion estimates
//
class FaceRecogFilter {
public:
   FaceRecogFilter();
  ~FaceRecogFilter();
  //
  virtual void process(const cv::Mat &image);
  //
  void DeleteTemporaryStorage();
  void countPixels(const cv::Mat &image);

  void setRenderer(Filter* renderer) {this->renderer = renderer; }

protected:
  Filter      *renderer;
  //
  // input variables
  //
  cv::CascadeClassifier cascade;
  cv::CascadeClassifier nestedCascade;
  double                scale;
  cv::Size              imageSize;
  int                   frameCount;
  bool                  detectEyes;
  cv::Mat               inputImage;
  cv::Mat               outputImage;
  cv::Size              text_size;
};

#endif
