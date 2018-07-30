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
#include "martycam.hpp"

#include <opencv2/opencv.hpp>

#include "capturethread.hpp"
#include "ConcurrentCircularBuffer.hpp"
#include  <boost/lockfree/queue.hpp>

#include <chrono>
#include <thread>

void qt_main(int argc, char ** argv)
{
    //  QApplication app(nCmdShow, NULL);
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("MartyCam");
    QCoreApplication::setOrganizationDomain("MartyCam");
    QCoreApplication::setApplicationName("MartyCam");
    //
    //
    //
    hpx::threads::executors::pool_executor defaultExecutor("default");
    hpx::threads::executors::pool_executor blockingExecutor("blocking");
    hpx::cout << "[hpx_main] Created default and " << "blocking"
              << " pool_executors \n";
    MartyCam *tracker = new MartyCam(defaultExecutor, blockingExecutor);
    tracker->show();
    app.exec();
}

int hpx_main(int argc, char ** argv)
{
    hpx::cout << "[hpx_main] starting hpx_main in "
              << hpx::threads::get_pool(hpx::threads::get_self_id())->get_pool_name()
              << " thread pool\n";

    hpx::threads::thread_id_type id = hpx::threads::get_self_id();

    hpx::cout << "[hpx_main] Thread Id = " << id;

    // Get a reference to one of the main thread
    hpx::threads::executors::main_pool_executor scheduler;
    // run an async function on the main thread to start the Qt application
    hpx::future<void> qt_application = hpx::async(scheduler, &qt_main, argc, argv);

    hpx::future<void> test_future
            = hpx::async(scheduler, [&](){
                hpx::cout << "Hello from the lambda in the main pool executor.";

                hpx::threads::thread_id_type id = hpx::threads::get_self_id();

                hpx::cout << "Thread Id = " << id;
            });

    // do something else while qt is executing in the background ...
    qt_application.wait();

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
