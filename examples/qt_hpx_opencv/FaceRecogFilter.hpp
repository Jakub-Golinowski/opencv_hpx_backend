#ifndef FACERECOGFILTER_H
#define FACERECOGFILTER_H

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

struct FaceRecogFilterParams {
    bool     detectEyes;
    double   scale;

};
//
// Filter which performs face recognition
//
class FaceRecogFilter {
public:
   FaceRecogFilter();
   FaceRecogFilter(FaceRecogFilterParams frfp);
  ~FaceRecogFilter();
  //
  virtual void process(const cv::Mat &image);
  //
  void setDecimationCoeff(int val);
  void setEyesRecogState(bool state) { this->eyesRecogState = state; }
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
  bool                  eyesRecogState;
  cv::Mat               inputImage;
  cv::Mat               outputImage;
  cv::Size              text_size;
};

#endif
