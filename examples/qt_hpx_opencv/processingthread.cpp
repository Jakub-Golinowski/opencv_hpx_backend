#include <QDebug>
#include <QTime>
//
#include <iostream>
//
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
//
#include "processingthread.hpp"
//
#include "filter.hpp"
//
#include <hpx/lcos/future.hpp>
#include <hpx/include/async.hpp>
//----------------------------------------------------------------------------
ProcessingThread::ProcessingThread(ImageBuffer buffer,
                                   hpx::threads::executors::pool_executor exec,
                                   ProcessingType processingType,
                                   MotionFilterParams mfp)
        : imageBuffer(buffer), executor(exec),
          motionFilter(new MotionFilter(mfp)), faceRecogFilter(new FaceRecogFilter),
          abort(false), processingType(processingType),
          QObject(nullptr) {}
//----------------------------------------------------------------------------
ProcessingThread::~ProcessingThread()
{
  delete this->motionFilter;
  delete this->faceRecogFilter;
}
//----------------------------------------------------------------------------
void ProcessingThread::CopySettings(ProcessingThread *thread)
{
  this->motionFilter->triggerLevel = thread->motionFilter->triggerLevel;
  this->motionFilter->threshold = thread->motionFilter->threshold;
  this->motionFilter->average =  thread->motionFilter->average;
  this->motionFilter->erodeIterations =  thread->motionFilter->erodeIterations;
  this->motionFilter->dilateIterations =  thread->motionFilter->dilateIterations;
  this->motionFilter->displayImage =  thread->motionFilter->displayImage;
  this->motionFilter->blendRatio =  thread->motionFilter->blendRatio;
}
//----------------------------------------------------------------------------
void ProcessingThread::setMotionDetectionProcessing(){
  this->processingType = ProcessingType::motionDetection;
}
void ProcessingThread::setFaceRecognitionProcessing(){
  this->processingType = ProcessingType::faceRecognition;
}
//----------------------------------------------------------------------------
void ProcessingThread::run() {
  int framenum = 0;
  while (!this->abort) {
    // blocking : waits until next image is available if necessary
    cv::Mat cameraImage = imageBuffer->receive();
    // if camera not working or disconnected, abort
    if (cameraImage.empty()) {
      boost::this_thread::sleep(boost::posix_time::milliseconds(100));
      continue;
    }
    cv::Mat cameracopy = cameraImage.clone();
    //
    switch(this->processingType){
        case ProcessingType::motionDetection :
          this->motionFilter->process(cameracopy);
          break;
        case ProcessingType ::faceRecognition :
          this->faceRecogFilter->process(cameracopy);
          break;
    }
    emit (NewData());
  }

  this->abort = false;
  this->stopWait.wakeAll();
}
//----------------------------------------------------------------------------
bool ProcessingThread::startProcessing()
{
  if (!processingActive) {
    processingActive = true;
    abort = false;

    hpx::async(this->executor, &ProcessingThread::run, this);

    return true;
  }
  return false;
}
//----------------------------------------------------------------------------
bool ProcessingThread::stopProcessing() {
  bool wasActive = this->processingActive;
  if (wasActive) {
    this->stopLock.lock();
    processingActive = false;
    abort = true;
    this->stopWait.wait(&this->stopLock);
    this->stopLock.unlock();
  }
  return wasActive;
}