//  Copyright (c) 2017 Shoshana Jakobovits
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

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
//
#include "shared_priority_scheduler.hpp"
#include "system_characteristics.hpp"
//
#include <iostream>
#include <opencv2/opencv.hpp>

static bool use_opencv_pool = false;
static int opencv_tp_num_threads = 1;

static std::string opencv_tp_name("opencv");

// this is our custom scheduler type
using high_priority_sched = hpx::threads::policies::shared_priority_scheduler<>;
using namespace hpx::threads::policies;

// Force an instantiation of the pool type templated on our custom scheduler
// we need this to ensure that the pool has the generated member functions needed
// by the linker for this pool type
// template class hpx::threads::detail::scheduled_thread_pool<high_priority_sched>;

// dummy function we will call using async
void do_stuff(std::size_t n, bool printout)
{
    if (printout)
        hpx::cout << "[do stuff] " << n << "\n";
    for (std::size_t i(0); i < n; ++i)
    {
        double f = std::sin(2 * M_PI * i / n);
        if (printout)
            hpx::cout << "sin(" << i << ") = " << f << ", ";
    }
    if (printout)
        hpx::cout << "\n";
}

void print_system_params(){
    // print partition characteristics
    std::cout << "\n\n[hpx_main] print resource_partitioner characteristics : "
              << "\n";
    hpx::resource::get_partitioner().print_init_pool_data(std::cout);

    // print partition characteristics
    std::cout << "\n\n[hpx_main] print thread-manager pools : "
              << "\n";
    hpx::threads::get_thread_manager().print_pools(std::cout);

    // print system characteristics
    std::cout << "\n\n[hpx_main] print System characteristics : "
              << "\n";
    print_system_characteristics();
}

void test_with_parallel_for(std::size_t loop_count,
                            const hpx::threads::executors::default_executor &
                            high_priority_executor,
                            const hpx::threads::scheduled_executor
                            &opencv_executor,
                            const hpx::threads::executors::default_executor &
                            normal_priority_executor){
    hpx::lcos::local::mutex m;
    std::set<std::thread::id> thread_set;

    // test a parallel algorithm on custom pool with high priority
    hpx::cout << "Test a parallel algorithm on a custom pool with a high priority.\n";
    hpx::parallel::execution::static_chunk_size fixed(1);
    hpx::parallel::for_loop_strided(
        hpx::parallel::execution::par.with(fixed).on(high_priority_executor), 0,
        loop_count, 1, [&](std::size_t i) {
            std::lock_guard<hpx::lcos::local::mutex> lock(m);
            if (thread_set.insert(std::this_thread::get_id()).second)
            {
                hpx::cout << std::hex << hpx::this_thread::get_id() << " "
                          << std::hex << std::this_thread::get_id()
                          << " high priority i " << std::dec << i << std::endl;
            }
        });
    hpx::cout << "thread set contains " << std::dec << thread_set.size()
              << std::endl;
    thread_set.clear();

    // test a parallel algorithm on custom pool with normal priority
    hpx::parallel::for_loop_strided(
            hpx::parallel::execution::par.with(fixed).on(normal_priority_executor),
            0, loop_count, 1, [&](std::size_t i) {
                std::lock_guard<hpx::lcos::local::mutex> lock(m);
                if (thread_set.insert(std::this_thread::get_id()).second)
                {
                    hpx::cout << std::hex << hpx::this_thread::get_id() << " "
                              << std::hex << std::this_thread::get_id()
                              << " normal priority i " << std::dec << i
                              << std::endl;
                }
            });
    hpx::cout << "thread set contains " << std::dec << thread_set.size()
              << std::endl;
    thread_set.clear();

    // test a parallel algorithm on opencv_executor
    hpx::parallel::for_loop_strided(
            hpx::parallel::execution::par.with(fixed).on(opencv_executor), 0,
            loop_count, 1, [&](std::size_t i) {
                std::lock_guard<hpx::lcos::local::mutex> lock(m);
                if (thread_set.insert(std::this_thread::get_id()).second)
                {
                    hpx::cout << std::hex << hpx::this_thread::get_id() << " "
                              << std::hex << std::this_thread::get_id()
                              << " opencv pool i " << std::dec << i << std::endl;
                }
            });
    hpx::cout << "thread set contains " << std::dec << thread_set.size()
              << std::endl;
    thread_set.clear();

//     auto high_priority_async_policy =
//         hpx::launch::async_policy(hpx::threads::thread_priority_critical);
//     auto normal_priority_async_policy = hpx::launch::async_policy();

    // test a parallel algorithm on custom pool with high priority
    hpx::parallel::for_loop_strided(
            hpx::parallel::execution::par
                    .with(fixed /*, high_priority_async_policy*/)
                    .on(opencv_executor),
            0, loop_count, 1,
            [&](std::size_t i)
            {
                std::lock_guard<hpx::lcos::local::mutex> lock(m);
                if (thread_set.insert(std::this_thread::get_id()).second)
                {
                    hpx::cout << std::hex << hpx::this_thread::get_id() << " "
                              << std::hex << std::this_thread::get_id()
                              << " high priority opencv i " << std::dec << i
                              << std::endl;
                }
            });
    hpx::cout << "thread set contains " << std::dec << thread_set.size()
              << std::endl;
    thread_set.clear();
}

// this is called on an hpx thread after the runtime starts up
int hpx_main(boost::program_options::variables_map& vm)
{
    {
        cv::Mat image;
        std::string path = vm["img-path"].as<std::string>();
        image = cv::imread( path, 1 );
        if ( !image.data )
        {
            printf("No image data \n");
            return -1;
        }
        cv::namedWindow("Display Image", cv::WINDOW_AUTOSIZE );
        imshow("Display Image", image);
        cv::waitKey(0);
    }

    if (vm.count("use-opencv-pool"))
        use_opencv_pool = true;

    std::cout << "[hpx_main] starting ...\n"
              << "use_opencv_pool " << use_opencv_pool << "\n";

    std::size_t num_threads = hpx::get_num_worker_threads();
    std::cout << "HPX using threads = " << num_threads << std::endl;

    std::size_t loop_count = num_threads * 1;
    std::size_t async_count = num_threads * 1;

    // create an executor with high priority for important tasks
    hpx::threads::executors::default_executor high_priority_executor(
        hpx::threads::thread_priority_critical);
    hpx::threads::executors::default_executor normal_priority_executor;

    hpx::threads::scheduled_executor opencv_executor;
    // create an executor on the opencv pool
    if (use_opencv_pool)
    {
        // get executors
        hpx::threads::executors::pool_executor opencv_exec(opencv_tp_name);
        opencv_executor = opencv_exec;
        hpx::cout << "\n[hpx_main] got opencv executor " << std::endl;
    }
    else
    {
        opencv_executor = high_priority_executor;
    }

    print_system_params();

    // use executor to schedule work on custom pool
    hpx::future<void> future_1 = hpx::async(opencv_executor, &do_stuff, 5, true);

    hpx::future<void> future_2 = future_1.then(
        opencv_executor, [](hpx::future<void>&& f) { do_stuff(5, true); });

    hpx::future<void> future_3 = future_2.then(opencv_executor,
        [opencv_executor, high_priority_executor, async_count](
            hpx::future<void>&& f) mutable {
            hpx::future<void> future_4, future_5;
            for (std::size_t i = 0; i < async_count; i++)
            {
                if (i % 2 == 0)
                {
                    future_4 =
                        hpx::async(opencv_executor, &do_stuff, async_count, false);
                }
                else
                {
                    future_5 = hpx::async(
                        high_priority_executor, &do_stuff, async_count, false);
                }
            }
            // the last futures we made are stored in here
            if (future_4.valid())
                future_4.get();
            if (future_5.valid())
                future_5.get();
        });

    future_3.get();

//    test_with_parallel_for(loop_count, high_priority_executor,
//                           opencv_executor, normal_priority_executor);

    return hpx::finalize();
}

// the normal int main function that is called at startup and runs on an OS thread
// the user must call hpx::init to start the hpx runtime which will execute hpx_main
// on an hpx thread
int main(int argc, char* argv[])
{
    namespace po = boost::program_options;
    po::options_description desc_cmdline("Options");
    desc_cmdline.add_options()
        ("img-path",
         po::value<std::string>()->default_value(("./")),
         "Specify path to the image")
        ("use-opencv-pool,u", "Enable advanced HPX thread pools and executors")
        ("opencv_tp_num_threads,m",
          po::value<int>()->default_value(1),
          "Number of threads to assign to custom pool");

    // HPX uses a boost program options variable map, but we need it before
    // hpx-main, so we will create another one here and throw it away after use
    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).allow_unregistered()
                          .options(desc_cmdline).run(),
                  vm);
    }
    catch(po::error& e) {
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
        std::cerr << desc_cmdline << std::endl;
        return -1;
    }

    if (vm.count("use-opencv-pool"))
    {
        use_opencv_pool = true;
    }

    opencv_tp_num_threads = vm["opencv_tp_num_threads"].as<int>();

    // Create the resource partitioner
    hpx::resource::partitioner rp(desc_cmdline, argc, argv);
    std::cout << "[main] obtained reference to the resource_partitioner\n";

    // create a thread pool and supply a lambda that returns a new pool with
    // the user supplied scheduler attached
    rp.create_thread_pool("default",
        [](hpx::threads::policies::callback_notifier& notifier,
            std::size_t num_threads, std::size_t thread_offset,
            std::size_t pool_index, std::string const& pool_name)
        -> std::unique_ptr<hpx::threads::thread_pool_base>
        {
            std::cout << "User defined scheduler creation callback "
                      << std::endl;

            std::unique_ptr<high_priority_sched> scheduler(
                new high_priority_sched(
                    num_threads, 1, false, false, "shared-priority-scheduler"));

            auto mode = scheduler_mode(scheduler_mode::do_background_work |
                scheduler_mode::delay_exit);

            std::unique_ptr<hpx::threads::thread_pool_base> pool(
                new hpx::threads::detail::scheduled_thread_pool<
                        high_priority_sched
                    >(std::move(scheduler), notifier,
                        pool_index, pool_name, mode, thread_offset));
            return pool;
        });
    std::cout << "[main] " << "thread_pool default created\n";

    if (use_opencv_pool)
    {
        // Create a thread pool using the default scheduler provided by HPX
        rp.create_thread_pool(opencv_tp_name,
            hpx::resource::scheduling_policy::local_priority_fifo);
        std::cout << "[main] " << "thread_pool "
                  << opencv_tp_name << " created\n";

        // create a thread pool and supply a lambda that returns a new pool with
        // the a user supplied scheduler attached
//        rp.create_thread_pool(opencv_tp_name,
//            [](hpx::threads::policies::callback_notifier& notifier,
//                std::size_t num_threads, std::size_t thread_offset,
//                std::size_t pool_index, std::string const& pool_name)
//            -> std::unique_ptr<hpx::threads::thread_pool_base>
//            {
//                std::cout << "User defined scheduler creation callback "
//                          << std::endl;
//                std::unique_ptr<high_priority_sched> scheduler(
//                    new high_priority_sched(num_threads, 1, false, false,
//                        "shared-priority-scheduler"));
//
//                auto mode = scheduler_mode(scheduler_mode::delay_exit);
//
//                std::unique_ptr<hpx::threads::thread_pool_base> pool(
//                    new hpx::threads::detail::scheduled_thread_pool<
//                            high_priority_sched
//                        >(std::move(scheduler), notifier,
//                            pool_index, pool_name, mode, thread_offset));
//                return pool;
//            });

        // rp.add_resource(rp.numa_domains()[0].cores()[0].pus(), opencv_tp_name);
        // add N cores to opencv pool
        int count = 0;
        for (const hpx::resource::numa_domain& d : rp.numa_domains())
        {
            for (const hpx::resource::core& c : d.cores())
            {
                for (const hpx::resource::pu& p : c.pus())
                {
                    if (count < opencv_tp_num_threads)
                    {
                        std::cout << "[main] Added pu " << count++ << " to " <<
                                  opencv_tp_name << "thread pool\n";
                        rp.add_resource(p, opencv_tp_name);
                    }
                }
            }
        }

        std::cout << "[main] resources added to thread_pools \n";
    }

    return hpx::init();
}
