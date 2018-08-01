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
#include <QtCore/QObject>
//
#include <boost/shared_ptr.hpp>
#include "ConcurrentCircularBuffer.hpp"
typedef boost::shared_ptr< ConcurrentCircularBuffer<cv::Mat> > ImageBuffer;
//
class Filter;
class PSNRFilter;
class GraphUpdateFilter;
#include "MotionFilter.hpp"
#include "FaceRecogFilter.hpp"

struct processingThreadParams{

};

enum class ProcessingType: int
{
    motionDetection = 0,
    faceRecognition =1,
};

class ProcessingThread : public QObject{
Q_OBJECT;
public:
   ProcessingThread(ImageBuffer buffer,
                    hpx::threads::executors::pool_executor exec,
                    ProcessingType processingType,
                    MotionFilterParams mfp);
  ~ProcessingThread();
  //
  void CopySettings(ProcessingThread *thread);
  //
  void setRootFilter(Filter* filter) {   this->motionFilter->renderer = filter;
                                         this->faceRecogFilter->setRenderer(filter); }
  void setThreshold(int val) { this->motionFilter->threshold = val; }
  void setAveraging(double val) { this->motionFilter->average = val; }
  void setErodeIterations(int val) { this->motionFilter->erodeIterations = val; }
  void setDilateIterations(int val) { this->motionFilter->dilateIterations = val; }
  void setDisplayImage(int image) { this->motionFilter->displayImage = image; }
  void setBlendRatios(double ratio1) { this->motionFilter->blendRatio = ratio1; }

  void run();
  bool startProcessing();
  bool stopProcessing();

  void setMotionDetectionProcessing();
  void setFaceRecognitionProcessing();

  MotionFilter       *motionFilter;
  FaceRecogFilter    *faceRecogFilter;

signals:
    void NewData();

private:

  //
  QMutex           stopLock;
  QWaitCondition   stopWait;
  bool             processingActive;
  bool             abort;
  hpx::threads::executors::pool_executor executor;
  //
  ImageBuffer  imageBuffer;
  ProcessingType processingType;
};

#endif
