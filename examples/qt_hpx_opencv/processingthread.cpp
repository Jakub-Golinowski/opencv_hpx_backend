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
ProcessingThread::ProcessingThread(ImageBuffer buffer, cv::Size &size,
                                   hpx::threads::executors::pool_executor exec) : QObject(nullptr)
{
  this->executor          = exec;
  this->imageBuffer       = buffer;
  this->motionFilter      = new MotionFilter();
  this->abort             = false;
}
//----------------------------------------------------------------------------
ProcessingThread::~ProcessingThread()
{
  delete this->motionFilter;
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
    this->motionFilter->process(cameracopy);
    // TODO: make sure what this was doing
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

    this->processingThreadFinished =
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
//----------------------------------------------------------------------------
cv::Scalar ProcessingThread::getMSSIM(const cv::Mat& i1, const cv::Mat& i2)
{
  const double C1 = 6.5025, C2 = 58.5225;
  /***************************** INITS **********************************/
  int d = CV_32F;

  cv::Mat I1, I2;
  i1.convertTo(I1, d);           // cannot calculate on one byte large values
  i2.convertTo(I2, d);

  cv::Mat I2_2   = I2.mul(I2);        // I2^2
  cv::Mat I1_2   = I1.mul(I1);        // I1^2
  cv::Mat I1_I2  = I1.mul(I2);        // I1 * I2

  /***********************PRELIMINARY COMPUTING ******************************/

  cv::Mat mu1, mu2;   //
  GaussianBlur(I1, mu1, cv::Size(11, 11), 1.5);
  GaussianBlur(I2, mu2, cv::Size(11, 11), 1.5);

  cv::Mat mu1_2   =   mu1.mul(mu1);
  cv::Mat mu2_2   =   mu2.mul(mu2);
  cv::Mat mu1_mu2 =   mu1.mul(mu2);

  cv::Mat sigma1_2, sigma2_2, sigma12;

  GaussianBlur(I1_2, sigma1_2, cv::Size(11, 11), 1.5);
  sigma1_2 -= mu1_2;

  GaussianBlur(I2_2, sigma2_2, cv::Size(11, 11), 1.5);
  sigma2_2 -= mu2_2;

  GaussianBlur(I1_I2, sigma12, cv::Size(11, 11), 1.5);
  sigma12 -= mu1_mu2;

  ///////////////////////////////// FORMULA ////////////////////////////////
  cv::Mat t1, t2, t3;

  t1 = 2 * mu1_mu2 + C1;
  t2 = 2 * sigma12 + C2;
  t3 = t1.mul(t2);              // t3 = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))

  t1 = mu1_2 + mu2_2 + C1;
  t2 = sigma1_2 + sigma2_2 + C2;
  t1 = t1.mul(t2);               // t1 =((mu1_2 + mu2_2 + C1).*(sigma1_2 + sigma2_2 + C2))

  cv::Mat ssim_map;
  divide(t3, t1, ssim_map);      // ssim_map =  t3./t1;

  cv::Scalar mssim = cv::mean( ssim_map ); // mssim = average of ssim map
  return mssim;
}
