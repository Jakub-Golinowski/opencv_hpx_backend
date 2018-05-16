//
// Created by jakub on 16.05.18.
//

#ifndef HPX_IMAGE_LOAD_HPX_IMAGE_LOAD_HPP
#define HPX_IMAGE_LOAD_HPX_IMAGE_LOAD_HPP

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
#include <cmath>
#include <cstddef>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <stdexcept>
//
#include "shared_priority_scheduler.hpp"
#include "system_characteristics.hpp"
//
#include <iostream>
#include <opencv2/opencv.hpp>


// dummy function we will call using async
void do_stuff(std::size_t n, bool printout);

void print_system_params();

void test_with_parallel_for(std::size_t loop_count,
                            const hpx::threads::executors::default_executor &
                            high_priority_executor,
                            const hpx::threads::scheduled_executor
                            &opencv_executor,
                            const hpx::threads::executors::default_executor &
                            normal_priority_executor);

cv::Mat load_image(const std::string &path);

void show_image(const cv::Mat &image, std::string win_name);

void save_image(const cv::Mat &image, const std::string &);

cv::Mat transform_to_grey(cv::Mat image);

#endif //HPX_IMAGE_LOAD_HPX_IMAGE_LOAD_HPP
