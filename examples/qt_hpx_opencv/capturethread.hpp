#ifndef CAPTURE_THREAD_H
#define CAPTURE_THREAD_H
//
#include <hpx/config.hpp>
//
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
//
#include <QMutex>
#include <QWaitCondition>
//
#include <boost/shared_ptr.hpp>
#include <boost/lockfree/spsc_queue.hpp>
//
#include <hpx/parallel/execution.hpp>
#include <hpx/parallel/executors/pool_executor.hpp>
//
#include "ConcurrentCircularBuffer.hpp"
#define IMAGE_QUEUE_LEN 1024

typedef boost::circular_buffer< int > FrameSpeedBuffer;
typedef boost::shared_ptr< ConcurrentCircularBuffer<cv::Mat> > ImageBuffer;
typedef boost::shared_ptr< boost::lockfree::spsc_queue<cv::Mat, boost::lockfree::capacity<IMAGE_QUEUE_LEN>> > ImageQueue;


class CaptureThread{
public: 
   CaptureThread(ImageBuffer imageBuffer, const cv::Size &size, int device, const std::string &URL,
                 hpx::threads::executors::pool_executor exec);
  ~CaptureThread() ;

  void run();
  //
  bool connectCamera(int index, const std::string &URL);
  bool startCapture();
  bool stopCapture();
  //
  void setResolution(const cv::Size &res);
  //
  double getFPS() { return fps; }
  bool isCapturing() { return captureActive; }
  
  int  GetFrameCounter() { return this->FrameCounter; }
  std::string getCaptureStatusString() { return this->CaptureStatus; }

  //
  // Motion film
  //
  void setWriteMotionAVI(bool write) { this->MotionAVI_Writing = write; }
  bool getWriteMotionAVI() { return this->MotionAVI_Writing; }
  void setWriteMotionAVIName(const char *name);

  //
  // Time Lapse film
  //
  void addNextFrameToTimeLapse(bool write) { this->MotionAVI_Writing = write; }
  void setWriteTimeLapseAVIName(const char *name);

  //
  // General
  //
  void setWriteMotionAVIDir(const char *dir);
  void saveAVI(const cv::Mat &image);
  void closeAVI();

  void saveTimeLapseAVI(const cv::Mat &image);
  void startTimeLapse(double fps); 
  void stopTimeLapse(); 
  void updateTimeLapse();

  void setRotation(int value);
  //
  void  rotateImage(const cv::Mat &source, cv::Mat &rotated);
  void  captionImage(cv::Mat &image);

  cv::Size getImageSize() { return this->imageSize; }
  cv::Size getRotatedSize() { return this->rotatedSize; }
  bool getTimeLapseAVI_Writing() {return this->TimeLapseAVI_Writing; }
private:
    void setAbort(bool a) { this->abort = a; }
    void updateFPS(int time);

  //
  QMutex           stopLock;
  QWaitCondition   stopWait;
  hpx::future<void> captureThreadFinished;
  hpx::threads::executors::pool_executor executor;
  //
  bool             abort; 
  ImageBuffer      imageBuffer;
  bool             captureActive;
  bool             deInterlace;
  int              requestedFPS; 
  cv::Size         imageSize;
  cv::Size         rotatedSize;
  cv::VideoCapture capture;
  double           fps;
  FrameSpeedBuffer frameTimes;
  int              deviceIndex;
  int              rotation;
  int              FrameCounter;
  ImageBuffer      aviBuffer;
  //
  cv::VideoWriter  MotionAVI_Writer;
  cv::VideoWriter  TimeLapseAVI_Writer;
  bool             MotionAVI_Writing;
  bool             TimeLapseAVI_Writing;
  std::string      AVI_Directory;
  std::string      MotionAVI_Name;
  std::string      TimeLapseAVI_Name;
  std::string      CaptureStatus;
  std::string      CameraURL;
  //
  cv::Size         text_size;
  cv::Mat          currentFrame;
  cv::Mat          rotatedImage;
};

#endif
