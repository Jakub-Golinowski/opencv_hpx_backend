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

int main(int argc, char* argv[])
{

    int my_argc = 1;
    char str[] = "backend_launch";
    char* str_ptr = str;

    char * my_argv[] = {str_ptr};

    hpx::start(nullptr, my_argc, my_argv);

    // Wait for runtime to start
    hpx::runtime* rt = hpx::get_runtime_ptr();
    hpx::util::yield_while([rt]()
                           { return rt->get_state() < hpx::state_running; });

    int counter = 0;
    hpx::lcos::local::mutex m;

    hpx::apply( [&]() {
        hpx::parallel::for_loop(
            hpx::parallel::execution::par,
            0, 10,
            [&](std::size_t i) {
                std::lock_guard<hpx::lcos::local::mutex> lock(m);
                ++counter;
                hpx::cout << std::hex << hpx::this_thread::get_id() << " "
                          << std::hex << std::this_thread::get_id()
                          << " i = " << std::dec << i << std::endl;
            });
    });


    hpx::apply([]() { hpx::finalize(); });
    hpx::stop();

    std::cout << "Counter: " << counter;

    return 0;
}

