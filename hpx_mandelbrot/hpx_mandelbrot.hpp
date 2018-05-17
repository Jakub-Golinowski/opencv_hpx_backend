//
// Created by jakub on 17.05.18.
//

#ifndef HPX_IMAGE_LOAD_HPX_MANDELBROT_HPP
#define HPX_IMAGE_LOAD_HPX_MANDELBROT_HPP

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
//
#include <hpx/parallel/algorithms/for_loop.hpp>
#include <hpx/parallel/execution.hpp>
//
#include <hpx/runtime/resource/partitioner.hpp>
#include <hpx/runtime/threads/cpu_mask.hpp>
#include <hpx/runtime/threads/detail/scheduled_thread_pool_impl.hpp>
#include <hpx/runtime/threads/executors/pool_executor.hpp>
//
#include <hpx/include/iostreams.hpp>
#include <hpx/include/runtime.hpp>
//
#include "system_characteristics.hpp"
//
#include <cmath>
#include <cstddef>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <stdexcept>
//
#include <iostream>
#include <opencv2/opencv.hpp>

cv::Mat load_image(const std::string &path);
void show_image(const cv::Mat &image, std::string win_name);
void save_image(const cv::Mat &image, const std::string &);

int mandelbrot(const std::complex<float> &z0, const int max);


void print_system_params();

#endif //HPX_IMAGE_LOAD_HPX_MANDELBROT_HPP
