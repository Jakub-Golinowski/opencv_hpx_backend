#include "renderwidget.hpp"
//
#include <QPainter>
#include <QDebug>
#include <QTime>
#include <QMouseEvent>
//
#include <iostream>
//
#include <opencv2/imgproc/imgproc.hpp>
//----------------------------------------------------------------------------
/*
cv::Mat QImage2IplImage(QImage *qimg)
{
  IplImage *imgHeader = cvCreateImageHeader( cv::Size(qimg->width(), qimg->width()), IPL_DEPTH_8U, 4);
  imgHeader->imageData = (char*) qimg->bits();

  uchar* newdata = (uchar*) malloc(sizeof(uchar) * qimg->byteCount());
  memcpy(newdata, qimg->bits(), qimg->byteCount());
  imgHeader->imageData = (char*) newdata;
  //cvClo
  return imgHeader;
}
*/
//----------------------------------------------------------------------------
// this->movingAverage  = cvCreateImage( imageSize, IPL_DEPTH_32F, 3);
// this->thresholdImage = cvCreateImage( imageSize, IPL_DEPTH_8U, 1);
//----------------------------------------------------------------------------
template <typename T>
QImage *cvMat2QImage(const cv::Mat &mat)
{
  int h = mat.size().height;
  int w = mat.size().width;
  int channels = mat.channels();
  QImage *qimg = new QImage(w, h, QImage::Format_ARGB32);
  const T *data = reinterpret_cast<const T *>(mat.data);
  //
  for (int y=0; y<h; y++, data+=mat.step[0]/sizeof(T)) {
    for (int x = 0; x < w; x++) {
      char r, g, b, a = 0;
      if (channels == 1) {
        r = g = b = data[x];
        qimg->setPixel(x, y, qRgb(r, g, b));
      }
      else if (channels == 3 || channels == 4) {
        r = data[x * channels + 2];
        g = data[x * channels + 1];
        b = data[x * channels + 0];
        if (channels == 4) {
          a = data[x * channels + 3];
          qimg->setPixel(x, y, qRgba(r, g, b, a));
        }
        else {
          qimg->setPixel(x, y, qRgb(r, g, b));
        }
      }
    }
  }

  return qimg;
}
//----------------------------------------------------------------------------
RenderWidget::RenderWidget(QWidget* parent) : QWidget(parent), Filter(), imageValid(1)
{
  setAttribute(Qt::WA_OpaquePaintEvent, true); // don't clear the area before the paintEvent
//  setAttribute(Qt::WA_PaintOnScreen, true); // disable double buffering
  setFixedSize(720, 576);
  connect(this, SIGNAL(frameSizeChanged(int, int)), this, SLOT(onFrameSizeChanged(int, int)));
  connect(this, SIGNAL(update_signal(bool, int)), this, SLOT(UpdateTrigger(bool, int)), Qt::QueuedConnection);
  //
  this->bufferImage = NULL;
}
//----------------------------------------------------------------------------
void RenderWidget::setCVSize(const cv::Size &size)
{
  this->setFixedSize(size.width, size.height);
}
//----------------------------------------------------------------------------
void RenderWidget::onFrameSizeChanged(int width, int height) {
  this->setFixedSize(width, height);
}
//----------------------------------------------------------------------------
void RenderWidget::updatePixmap(const cv::Mat &frame)
{
  QImage *temp = this->bufferImage;
  imageValid.acquire();

  int bytes = frame.elemSize();
  int bytes_per_channel = bytes/frame.channels();

  if (bytes_per_channel==1) {
    this->bufferImage = cvMat2QImage<unsigned char>(frame);
  }
  else if (bytes_per_channel==2) {
    this->bufferImage = cvMat2QImage<short>(frame);
  }
  else if (bytes_per_channel==4) {
    this->bufferImage = cvMat2QImage<float>(frame);
  }
  else if (bytes_per_channel==8) {
    this->bufferImage = cvMat2QImage<double>(frame);
  }
  imageValid.release();
  delete temp;
}
//----------------------------------------------------------------------------
void RenderWidget::process(const cv::Mat &image) {
  // copy the image to the local pixmap and update the display
  this->updatePixmap(image);
  emit(update_signal(true, 12));
}
//----------------------------------------------------------------------------
void RenderWidget::paintEvent(QPaintEvent*) {
  QPainter painter(this);
  if (this->bufferImage) {
    imageValid.acquire();
    painter.drawImage(QPoint(0,0), *this->bufferImage);
    imageValid.release();
  }
  else {
    painter.setBrush(Qt::black);
    painter.drawRect(rect());
  }
}
//----------------------------------------------------------------------------
void RenderWidget::UpdateTrigger(bool, int) {
  this->repaint();
//  std::cout << "Rendered frame " << std::endl;
}
//----------------------------------------------------------------------------
void RenderWidget::mouseDoubleClickEvent ( QMouseEvent * event )
{
  const QPoint p = event->pos();
  emit mouseDblClicked( p );
}
