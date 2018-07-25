//  Copyright (c) 2012-2014 Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/thread_executors.hpp>
#include <hpx/lcos/future_wait.hpp>
#include <hpx/util/bind.hpp>

#include <cstddef>
#include <vector>

using hpx::util::high_resolution_timer;

#include <QtWidgets/QApplication>
#include "widget.hpp"

#include <opencv2/opencv.hpp>

typedef std::shared_ptr<hpx::lcos::local::mutex> MutexSharedPtr_t;

double runner(double now, int i, MutexSharedPtr_t m)
{
    high_resolution_timer t(now);
    // do something cool here
    cv::Mat frame;
    {
        std::lock_guard<hpx::lcos::local::mutex> lock(*m);
        cv::VideoCapture cap;

        // arg=0 opens the default camera
        if(!cap.open(0))
            return -1;

        cap >> frame;
    }

    std::string imageName = "./Gray_Image_" + std::to_string(i) + ".jpg";

    cv::imwrite(imageName, frame);

    return t.elapsed();
}

void run(widget * w, std::size_t num_threads)
{
    std::vector<hpx::lcos::future<double> > futures(num_threads);

    MutexSharedPtr_t m_sptr(new hpx::lcos::local::mutex());

    for(std::size_t i = 0; i < num_threads; ++i)
    {
        futures[i] = hpx::async(runner, high_resolution_timer::now(), i, m_sptr);
    }

    hpx::lcos::wait(
                futures,
                [w](std::size_t i, double t){ w->threadsafe_add_label(i, t); }
    );
    w->threadsafe_run_finished();
}

void qt_main(int argc, char ** argv)
{
    QApplication app(argc, argv);

    using hpx::util::placeholders::_1;
    using hpx::util::placeholders::_2;
    widget main(hpx::util::bind(run, _1, _2));
    main.show();

    app.exec();
}

int hpx_main(int argc, char ** argv)
{
    {
        // Get a reference to one of the main thread
        hpx::threads::executors::main_pool_executor scheduler;
        // run an async function on the main thread to start the Qt application
        hpx::future<void> qt_application
            = hpx::async(scheduler, qt_main, argc, argv);

        // do something else while qt is executing in the background ...

        qt_application.wait();
    }
    return hpx::finalize();
}

int main(int argc, char **argv)
{
    return hpx::init(argc, argv);
}
