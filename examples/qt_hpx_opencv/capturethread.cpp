#include "capturethread.hpp"

#include <QTime>
//
#include <iostream>
#include <iomanip>
//
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio/videoio_c.h"
#include "opencv2/imgproc/imgproc_c.h"

#include <hpx/lcos/future.hpp>
#include <hpx/include/async.hpp>

typedef boost::shared_ptr< ConcurrentCircularBuffer<cv::Mat> > ImageBuffer;

// Image deinterlacing function for DV camera
cv::Mat Deinterlace(cv::Mat &src)
{
  cv::Mat res = src; // src.clone();
  uchar* linea;
  uchar* lineb;
  uchar* linec;

  for (int i = 1; i < res.size().height-1; i+=2)
  {
    linea = (uchar*)res.data + ((i-1) * res.step[0]);
    lineb = (uchar*)res.data + ((i) * res.step[0]);
    linec = (uchar*)res.data + ((i+1) * res.step[0]);

    for (int j = 0; j < res.size().width * res.channels(); j++)
    {
      lineb[j] = (uchar)((linea[j] + linec[j])/2);
    }
  }

  if (res.size().height > 1 && res.size().height % 2 == 0)
  {
    linea = (uchar*)res.data + ((res.size().height-2) * res.step[0]);
    lineb = (uchar*)res.data + ((res.size().height-1) * res.step[0]);
    memcpy(lineb, linea, res.size().width);
  }
  return res;
}

//----------------------------------------------------------------------------
//TODO make behaviour consistent. For now I think it would be best that the CaptureThread
// constructor connects to camera and enforces the given resolution or at least stores tha actual resolutions in the settings
CaptureThread::CaptureThread(ImageBuffer imageBuffer, const cv::Size &size, int device, const std::string &URL,
                             hpx::threads::executors::pool_executor exec) : frameTimes(50)
{
  this->executor = exec;
  this->abort                = false;
  this->captureActive        = false;
  this->deInterlace          = false;
  this->requestedFPS         = 15;
  this->fps                  = 0.0 ;
  this->FrameCounter         = 0;
  this->deviceIndex          =-1;
  this->MotionAVI_Writing    = false;
  this->capture           = NULL;
  this->imageBuffer       = imageBuffer;
  this->deviceIndex       = device;
  this->rotation          = 0;
  this->rotatedImage      = NULL;
  // initialize font and precompute text size
  QString timestring = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
  this->text_size = cv::getTextSize( timestring.toUtf8().constData(), CV_FONT_HERSHEY_PLAIN, 1.0, 1, NULL);
  // TODO: for now this connects to camera and uses its default resolution
  this->connectCamera(this->deviceIndex, this->CameraURL);
  // TODO Here we overwrite the default resolution from the camera for the arbitrary one - not safe when the resolution is not supported!
  this->setResolution(size);
  this->imageSize         = cv::Size(size.width, size.height);
  this->rotatedSize       = cv::Size(size.height, size.width);
  // code for getting the current resolution
//  int w = this->capture.get(CV_CAP_PROP_FRAME_WIDTH);
//  int h = this->capture.get(CV_CAP_PROP_FRAME_HEIGHT);

}
//----------------------------------------------------------------------------
CaptureThread::~CaptureThread() 
{  
  this->closeAVI();
  // Release our stream capture object, not necessary with openCV 2
  this->capture.release();
}
//----------------------------------------------------------------------------
//TODO it seems that after opening the camera these values in caputure are overriden by what was actually opened by camera?
// This function is HPX-free - such that it does not uses HPX API
// BUT! I plan to have stopCapture to be HPX-dependent
bool CaptureThread::connectCamera(int index, const std::string &URL)
{
  bool wasActive = this->stopCapture();
  if (this->capture.isOpened()) {
    this->capture.release();
  }

  //
  // start this->capture device driver
  //
  if (URL=="NULL") {
    // null camera, dummy
  }
  else {
    if (URL.size()>0) {
      this->CameraURL = URL;
      std::cout << "Attempting IP camera connection " << this->CameraURL.c_str() << std::endl;
      // using an IP camera, assume default string to access martycam
      // this->capture = cvCaptureFromFile("http://192.168.1.21/videostream.asf?user=admin&pwd=1234");
      // this->capture = cvCaptureFromFile("http://admin:1234@192.168.1.21/videostream.cgi?req_fps=30&.mjpg");
      this->capture.open(URL);
    }
    else {
      std::cout << "opening device " << index << std::endl;
#ifdef _WIN32
      this->capture.open(CV_CAP_DSHOW + index );
#else
//      CvCapture* camera = cvCaptureFromCAM(CV_CAP_ANY);
      capture.open(index );
#endif
    }
    if (!this->capture.isOpened()) {
      std::cout << "Camera connection failed" << std::endl;
      return false;
    }
  }
  if (wasActive) {
    return this->startCapture();
  }
  return true;
}
//----------------------------------------------------------------------------
void CaptureThread::setResolution(const cv::Size &res)
{
  if (this->imageSize==res) {
    return;
  }
  bool wasActive = this->stopCapture();
  this->imageBuffer->clear();
  this->imageSize = res;
  this->rotatedSize = cv::Size(this->imageSize.height, this->imageSize.width);
  if (wasActive) this->startCapture(); 
}
//----------------------------------------------------------------------------
void CaptureThread::setRotation(int value) 
{ 
  if (this->rotation==value) {
    return;
  }
  bool wasActive = this->stopCapture();
  this->rotation = value; 
  if (this->rotation==1 || this->rotation==2) {
    cv::Size workingSize = cv::Size(this->imageSize.height, this->imageSize.width);
    this->rotatedImage = cv::Mat( workingSize, CV_8UC3);
  }
  if (wasActive) this->startCapture(); 
}
//----------------------------------------------------------------------------
void CaptureThread::run() 
{  
  QTime time;
  time.start();

  while (!this->abort) {
    if (!captureActive) {
      std::cout << "WARN: CaptureThread::run() still running even though captureActive=false";
      continue;
    }

    // get latest frame from webcam
    cv::Mat frame;
    this->capture >> frame;

    if (frame.empty()) {
      this->setAbort(true);
      std::cout << "Empty camera image, aborting this->capture " <<std::endl;
      continue;
    }

    if (this->deInterlace) {
      // de-interlace image
      frame = Deinterlace(frame);
    }

    // rotate image if necessary, makes a copy which we can pass to queue
    this->rotateImage(frame, this->rotatedImage);

    // always write the frame out if saving movie or in the process of closing AVI
    if (this->MotionAVI_Writing || this->MotionAVI_Writer.isOpened()) {
      // add date time stamp if enabled
      this->captionImage(this->rotatedImage);
      this->saveAVI(this->rotatedImage);
    }

    this->currentFrame = frame;

    // add to buffer if space is available,
    imageBuffer->send(this->rotatedImage);


    this->FrameCounter++;
      
    updateFPS(time.elapsed());
  }

  // The run() task is exiting -> wake the threads waiting for that.
  this->stopWait.wakeAll();

}
//----------------------------------------------------------------------------
bool CaptureThread::startCapture() 
{
  if (!captureActive) {
    if (this->imageSize.width>0) {
      this->capture.set(CV_CAP_PROP_FRAME_WIDTH, this->imageSize.width);
      this->capture.set(CV_CAP_PROP_FRAME_HEIGHT, this->imageSize.height);
    }
    else {
      this->capture.set(CV_CAP_PROP_FRAME_WIDTH, 2048);
      this->capture.set(CV_CAP_PROP_FRAME_HEIGHT, 2048);
    }
    this->capture.set(CV_CAP_PROP_FPS, this->requestedFPS);
    std::ostringstream output;
    output << "CV_CAP_PROP_FRAME_WIDTH\t"   << this->capture.get(CV_CAP_PROP_FRAME_WIDTH) << std::endl;
    output << "CV_CAP_PROP_FRAME_HEIGHT\t"  << this->capture.get(CV_CAP_PROP_FRAME_HEIGHT) << std::endl;
    output << "CV_CAP_PROP_FPS\t"           << this->capture.get(CV_CAP_PROP_FPS) << std::endl;
    output << "CV_CAP_PROP_FOURCC\t"        << this->capture.get(CV_CAP_PROP_FOURCC) << std::endl;
    output << "CV_CAP_PROP_BRIGHTNESS\t"    << this->capture.get(CV_CAP_PROP_BRIGHTNESS) << std::endl;
    output << "CV_CAP_PROP_CONTRAST\t"      << this->capture.get(CV_CAP_PROP_CONTRAST) << std::endl;
    output << "CV_CAP_PROP_SATURATION\t"    << this->capture.get(CV_CAP_PROP_SATURATION) << std::endl;
    output << "CV_CAP_PROP_HUE\t"           << this->capture.get(CV_CAP_PROP_HUE) << std::endl;

    captureActive = true;
    abort = false;

    this->captureThreadFinished =
    hpx::async(this->executor, &CaptureThread::run, this);

    this->CaptureStatus += output.str();

    return true;
  }
  return false;
}
//----------------------------------------------------------------------------
bool CaptureThread::stopCapture() {
  bool wasActive = this->captureActive;
  if (wasActive) {
    this->stopLock.lock();
    captureActive = false;
    abort = true;
    this->stopWait.wait(&this->stopLock);
    this->stopLock.unlock();
  }
  return wasActive;
}
//----------------------------------------------------------------------------
void CaptureThread::updateFPS(int time) {
  frameTimes.push_back(time);
  if (frameTimes.size() > 1) {
    fps = frameTimes.size()/((double)time-frameTimes.front())*1000.0;
    fps = (static_cast<int>(fps*10))/10.0;
  }
  else {
    fps = 0;
  }
}
//----------------------------------------------------------------------------
void CaptureThread::saveAVI(const cv::Mat &image) 
{
  //CV_FOURCC('M', 'J', 'P', 'G'),
  //CV_FOURCC('M', 'P', '4', '2') = MPEG-4.2 codec
  //CV_FOURCC('D', 'I', 'V', '3') = MPEG-4.3 codec
  //CV_FOURCC('D', 'I', 'V', 'X') = MPEG-4 codec
  //CV_FOURCC('X', 'V', 'I', 'D')  
  if (!this->MotionAVI_Writer.isOpened()) {
    std::string path = this->AVI_Directory + "/" + this->MotionAVI_Name + std::string(".avi");
    this->MotionAVI_Writer.open(
      path.c_str(),
      CV_FOURCC('X', 'V', 'I', 'D'),  
      this->getFPS(),
      image.size()
    );
//    emit(RecordingState(true));
  }
  if (this->MotionAVI_Writer.isOpened()) {
    this->MotionAVI_Writer.write(image);
    // if CloseAvi has been called, stop writing.
    if (!this->MotionAVI_Writing) {
      this->MotionAVI_Writer.release();
//      emit(RecordingState(false));
    }
  }
  else {
    std::cout << "Failed to create AVI writer" << std::endl;
    return;
  }
}
//----------------------------------------------------------------------------
void CaptureThread::closeAVI() 
{
  this->MotionAVI_Writing = false;
}
//----------------------------------------------------------------------------
void CaptureThread::setWriteMotionAVIDir(const char *dir)
{
  this->AVI_Directory = dir;
}
//----------------------------------------------------------------------------
void CaptureThread::setWriteMotionAVIName(const char *name)
{
  this->MotionAVI_Name = name;
}
//----------------------------------------------------------------------------
void CaptureThread::rotateImage(const cv::Mat &source, cv::Mat &rotated)
{
  switch (this->rotation) {
    case 0:
      source.copyTo(rotated);
      break;
    case 1:
      cv::flip(source, rotated, 1);
      cv::transpose(rotated, rotated);
      break;
    case 2:
      cv::transpose(source, rotated);
      cv::flip(rotated, rotated, 1);
      break;
    case 3:
      source.copyTo(rotated);
      cv::flip(rotated, rotated, -1);
      break;
  }
}
//----------------------------------------------------------------------------
void CaptureThread::captionImage(cv::Mat &image)
{
  QString timestring = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
  cv::putText(image, timestring.toLatin1().data(),
    cvPoint(image.size().width - text_size.width - 4, text_size.height+4), 
    CV_FONT_HERSHEY_PLAIN, 1.0, cv::Scalar(255, 255, 255, 0), 1);
}
//----------------------------------------------------------------------------
