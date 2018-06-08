#include <hpx/hpx_start.hpp>
#include <hpx/hpx_suspend.hpp>
//
#include <hpx/include/apply.hpp>
//
#include <hpx/util/yield_while.hpp>
#include <hpx/include/threadmanager.hpp>
//
#include <hpx/parallel/algorithms/for_loop.hpp>
//
#include <hpx/include/iostreams.hpp>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    int my_argc = 1;
    char app[] = "backend_launch";
    char* app_ptr = app;
    char * my_argv[] = {app_ptr, nullptr};

    int num_cores = 6;
    std::string cfg_string;

    cfg_string =  "hpx.os_threads=" + std::to_string(num_cores);
    std::vector<std::string> const cfg = {
            cfg_string
    };

    hpx::start(nullptr, my_argc, my_argv, cfg);

    // Wait for runtime to start
    hpx::runtime* rt = hpx::get_runtime_ptr();
    hpx::util::yield_while([rt]()
                           { return rt->get_state() < hpx::state_running; });

    int counter = 0;
    hpx::lcos::local::mutex m;

    std::size_t num_work_threads = -1;

    hpx::apply( [&]() {
        num_work_threads = hpx::get_num_worker_threads();
    });

    hpx::apply( [&]() {
        hpx::parallel::for_loop(
            hpx::parallel::execution::par,
            0, 10,
            [&](std::size_t i) {
                std::lock_guard<hpx::lcos::local::mutex> lock(m);
                ++counter;

                hpx::get_num_worker_threads();

                hpx::cout << std::hex << hpx::this_thread::get_id() << " "
                          << std::hex << std::this_thread::get_id()
                          << " i = " << std::dec << i << std::endl;
            });
    });


    hpx::apply([]() { hpx::finalize(); });
    hpx::stop();

    std::cout << "Counter: " << counter << std::endl;
    std::cout << "Num worker threads: " << num_work_threads
              << " (expected: " + std::to_string(num_cores) + ")";

    return 0;
}

