#include "filter.hpp"
#include "FaceRecogFilter.hpp"
//
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "opencv2/videoio/videoio_c.h"
#include "opencv2/imgproc/imgproc_c.h"
//
#include <QDateTime>
#include <QString>
//
#include <config.h>
//
//----------------------------------------------------------------------------
FaceRecogFilter::FaceRecogFilter()
        : cascade(),
          nestedCascade(),
          scale(1.0),
          renderer(nullptr),
          imageSize(cv::Size(-1,-1)),
          eyesRecogState(false),
          inputImage(),
          outputImage() {
  // Load classifiers
  std::string nestedCascadePath = DATA_PATH +
          std::string("/models/haarcascade_eye_tree_eyeglasses.xml");
  std::string cascadePath = DATA_PATH +
          std::string("/models/haarcascade_frontalface_default.xml");

  if(eyesRecogState)
    nestedCascade.load(nestedCascadePath);
  cascade.load(cascadePath);
}
//----------------------------------------------------------------------------
FaceRecogFilter::FaceRecogFilter(FaceRecogFilterParams frfp)
        : cascade(),
          nestedCascade(),
          scale(frfp.scale),
          renderer(nullptr),
          imageSize(cv::Size(-1,-1)),
          eyesRecogState(frfp.detectEyes),
          inputImage(),
          outputImage() {
  // Load classifiers
  std::string nestedCascadePath = DATA_PATH +
                                  std::string("/models/haarcascade_eye_tree_eyeglasses.xml");
  std::string cascadePath = DATA_PATH +
                            std::string("/models/haarcascade_frontalface_default.xml");


  nestedCascade.load(nestedCascadePath);
  cascade.load(cascadePath);
}
//----------------------------------------------------------------------------
FaceRecogFilter::~FaceRecogFilter()
{
  this->DeleteTemporaryStorage();
}
//----------------------------------------------------------------------------
void FaceRecogFilter::DeleteTemporaryStorage()
{
  this->inputImage.release();
  this->outputImage.release();
}
//----------------------------------------------------------------------------
void FaceRecogFilter::process(const cv::Mat &img)
{

  std::vector<cv::Rect> faces, faces2;
  cv::Mat gray, smallImg;

  cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY); // Convert to Gray Scale
  double fx = 1 / scale;

  // Resize the Grayscale Image
  resize(gray, smallImg, cv::Size(), fx, fx, cv::INTER_LINEAR);
  equalizeHist(smallImg, smallImg);

  // Detect faces of different sizes using cascade classifier
  cascade.detectMultiScale(smallImg, faces, 1.1,
                           2, 0|cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30));

  // Draw squares around the faces
  for (size_t i = 0; i < faces.size(); i++)
  {
    cv::Rect r = faces[i];
    cv::Mat smallImgROI;
    std::vector<cv::Rect> nestedObjects;
    cv::Point center;
    cv::Scalar color = cv::Scalar(255, 0, 0); // Color for Drawing tool
    int radius;

    rectangle(img,
              cv::Point(cvRound(r.x*scale), cvRound(r.y*scale)),
              cv::Point(cvRound((r.x + r.width-1)*scale),
                        cvRound((r.y + r.height-1)*scale)),
              color, 2, 8, 0);
    if(!eyesRecogState)
      continue;
    smallImgROI = smallImg(r);

    // Detection of eyes int the input image
    nestedCascade.detectMultiScale(smallImgROI, nestedObjects, 1.1, 2,
                                   0|cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30));

    // Draw circles around eyes
    for (size_t j = 0; j < nestedObjects.size(); j++)
    {
      cv::Rect nr = nestedObjects[j];
      center.x = cvRound((r.x + nr.x + nr.width*0.5)*scale);
      center.y = cvRound((r.y + nr.y + nr.height*0.5)*scale);
      radius = cvRound((nr.width + nr.height)*0.25*scale);
      circle(img, center, radius, color, 2, 8, 0);
    }
  }

  //
  // Add time and data to image
  //
  QString timestring = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
  this->text_size = cv::getTextSize( timestring.toLatin1().data(), CV_FONT_HERSHEY_PLAIN, 1.0, 1, NULL);

  cv::putText(img, timestring.toLatin1().data(),
  cvPoint(img.size().width - text_size.width - 4, text_size.height+4),
    CV_FONT_HERSHEY_PLAIN, 1.0, cv::Scalar(255, 255, 255, 0), 1);
  //
  // Pass final image to GUI
  //
  if (renderer) {
    renderer->process(img);
  }
  //    std::cout << "Processed frame " << framenum++ << std::endl;
}
void FaceRecogFilter::setDecimationCoeff(int val){
  this->scale = static_cast<double>(100) / val;
//
//  float decimationFraction = static_cast<float>(val)/100;
//  this->scale =
}
