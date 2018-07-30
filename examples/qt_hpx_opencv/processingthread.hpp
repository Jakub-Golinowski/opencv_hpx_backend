#ifndef PROCESSING_THREAD_H
#define PROCESSING_THREAD_H
//
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
//
#include <boost/shared_ptr.hpp>
#include "ConcurrentCircularBuffer.hpp"
typedef boost::shared_ptr< ConcurrentCircularBuffer<cv::Mat> > ImageBuffer;
//
class Filter;
class PSNRFilter;
class GraphUpdateFilter;
#include "MotionFilter.hpp"

class ProcessingThread {
public:
   ProcessingThread(ImageBuffer buffer, cv::Size &size);
  ~ProcessingThread();
  //
  void CopySettings(ProcessingThread *thread);
  void DeleteTemporaryStorage();
  //
  double getmotionEstimate() { return this->motionFilter->motionEstimate; }
  //
  void setRootFilter(Filter* filter) {   this->motionFilter->renderer = filter; }
  void setThreshold(int val) { this->motionFilter->threshold = val; }
  void setAveraging(double val) { this->motionFilter->average = val; }
  void setErodeIterations(int val) { this->motionFilter->erodeIterations = val; }
  void setDilateIterations(int val) { this->motionFilter->dilateIterations = val; }
  void setDisplayImage(int image) { this->motionFilter->displayImage = image; }
  void setBlendRatios(double ratio1) {
    this->motionFilter->blendRatio = ratio1;
  }

  void run();
  void setAbort(bool a) { this->abort = a; }

  double getPSNR();
  cv::Scalar getMSSIM(const cv::Mat& i1, const cv::Mat& i2);

  //
  GraphUpdateFilter  *graphFilter;
  MotionFilter       *motionFilter;


private:
  ImageBuffer  imageBuffer;
  bool         abort;
  //
#ifndef Q_MOC_RUN
  boost::circular_buffer<cv::Mat> RecordBuffer;
#endif
};

#endif
