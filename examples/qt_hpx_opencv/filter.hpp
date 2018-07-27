#ifndef FILTER_H
#define FILTER_H

#include "opencv2/core/core_c.h"
#include "opencv2/core/core.hpp"

//
// Base class for filter/processor objects
//
class Filter {
public:
  Filter();
  //
  virtual void setDelegate(Filter* list);
  virtual void process(const cv::Mat &image) = 0;

protected:
  Filter* delegate;
  void invokeDelegate(const cv::Mat &image);
};

#endif
