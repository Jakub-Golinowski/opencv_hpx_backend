//  Copyright (c) 2012-2014 Thomas Heller
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/thread_executors.hpp>
#include <hpx/lcos/future_wait.hpp>
#include <hpx/util/bind.hpp>

#include <hpx/include/iostreams.hpp>

#include <cstddef>
#include <vector>

using hpx::util::high_resolution_timer;

#include <QtWidgets/QApplication>
#include "widget.hpp"

#include <opencv2/opencv.hpp>

#include "capturethread.hpp"
#include "ConcurrentCircularBuffer.hpp"
#include  <boost/lockfree/queue.hpp>

#include <chrono>
#include <thread>

void run(widget * w, CaptureThread* captureThread)
{
    hpx::cout << "Hello from run.";
    hpx::threads::executors::pool_executor defaultExecutor("default");
    hpx::threads::executors::pool_executor blockingExecutor("blocking");

    captureThread->setAbort(false);
    captureThread->startCapture();
    hpx::future<void> captureThreadFinished =
            hpx::async(blockingExecutor, &CaptureThread::run, captureThread);

//    std::this_thread::__sleep_for(std::chrono::seconds(2),
//                                  std::chrono::nanoseconds(0));

    captureThread->setAbort(true);
    captureThreadFinished.wait();
    captureThread->stopCapture();

    cv::Mat frame;
//    captureThread->capture >> frame;
//    captureThread->imageBuffer->send()
        int buff_size = captureThread->imageBuffer->size();
    w->threadsafe_add_label(buff_size, 0.0);
    frame = captureThread->imageBuffer->receive();

//    captureThread->imageQueue->pop(frame);
    w->renderer->process(frame);

    w->threadsafe_run_finished();
}

void qt_main(int argc, char ** argv, CaptureThread* captureThread)
{
    QApplication app(argc, argv);

    using hpx::util::placeholders::_1;
    widget main(hpx::util::bind(run, _1, captureThread));
    main.show();

    app.exec();
}

int hpx_main(int argc, char ** argv)
{
    hpx::cout << "[hpx_main] starting hpx_main in "
              << hpx::threads::get_pool(hpx::threads::get_self_id())->get_pool_name()
              << "\n";

    ImageQueue imageQueue = ImageQueue(new boost::lockfree::spsc_queue<cv::Mat, boost::lockfree::capacity<IMAGE_QUEUE_LEN>>);
    ImageBuffer imageBuffer = ImageBuffer(new ConcurrentCircularBuffer<cv::Mat>);
    imageBuffer->set_capacity(5);
    cv::Size size(320,380);
    int deviceIdx = 0;

    CaptureThread* captureThread = new CaptureThread(imageQueue, imageBuffer, size, deviceIdx, "");

    {
        // Get a reference to one of the main thread
        hpx::threads::executors::main_pool_executor scheduler;
        // run an async function on the main thread to start the Qt application
        hpx::future<void> qt_application
            = hpx::async(scheduler, qt_main, argc, argv, captureThread);

        // do something else while qt is executing in the background ...

        qt_application.wait();
    }
    return hpx::finalize();
}

int main(int argc, char **argv)
{
    namespace po = boost::program_options;
    po::options_description desc_cmdline("Options");
    desc_cmdline.add_options()
            ("blocking_tp_num_threads,m",
             po::value<int>()->default_value(1),
             "Number of threads to assign to blocking pool");

    // HPX uses a boost program options variable map, but we need it before
    // hpx-main, so we will create another one here and throw it away after use
    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).allow_unregistered()
                          .options(desc_cmdline).run(), vm);
    }
    catch(po::error& e) {
        std::cerr << "ERROR: " << e.what() << "\n\n";
        std::cerr << desc_cmdline << "\n";
        return -1;
    }

    int blocking_tp_num_threads = vm["blocking_tp_num_threads"].as<int>();

    // Create the resource partitioner
    hpx::resource::partitioner rp(desc_cmdline, argc, argv);
    std::cout << "[main] obtained reference to the resource_partitioner\n";

    rp.create_thread_pool("default",
                          hpx::resource::scheduling_policy::local_priority_fifo);
    std::cout << "[main] " << "thread_pool default created\n";

    // Create a thread pool using the default scheduler provided by HPX
    rp.create_thread_pool("blocking",
                          hpx::resource::scheduling_policy::local_priority_fifo);

    std::cout << "[main] " << "thread_pool blocking created\n";

    // add PUs to opencv pool
    int count = 0;
    for (const hpx::resource::numa_domain& d : rp.numa_domains())
    {
        for (const hpx::resource::core& c : d.cores())
        {
            for (const hpx::resource::pu& p : c.pus())
            {
                if (count < blocking_tp_num_threads)
                {
                    std::cout << "[main] Added pu " << count++ << " to " <<
                              "blocking" << " thread pool\n";
                    rp.add_resource(p, "blocking");
                }
            }
        }
    }

    std::cout << "[main] resources added to thread_pools \n";

    return hpx::init(argc, argv);
}
