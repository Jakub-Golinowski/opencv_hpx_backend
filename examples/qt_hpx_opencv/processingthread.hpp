#ifndef PROCESSING_THREAD_H
#define PROCESSING_THREAD_H
//
#include <hpx/config.hpp>
#include <hpx/parallel/execution.hpp>
#include <hpx/parallel/executors/pool_executor.hpp>
//
#include <QMutex>
#include <QWaitCondition>
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
   ProcessingThread(ImageBuffer buffer, cv::Size &size,
                    hpx::threads::executors::pool_executor exec);
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
  void setBlendRatios(double ratio1) { this->motionFilter->blendRatio = ratio1; }

  void run();
  bool startProcessing();
  bool stopProcessing();

  double getPSNR();
  cv::Scalar getMSSIM(const cv::Mat& i1, const cv::Mat& i2);

  //
  GraphUpdateFilter  *graphFilter;
  MotionFilter       *motionFilter;

private:
  void setAbort(bool a) { this->abort = a; }

  QMutex           stopLock;
  QWaitCondition   stopWait;
  bool             processingActive;
  bool             abort;
  //
  hpx::future<void> processingThreadFinished;
  hpx::threads::executors::pool_executor executor;
  //
  ImageBuffer  imageBuffer;
};

#endif
