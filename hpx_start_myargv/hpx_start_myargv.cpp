#include <hpx/hpx_start.hpp>
#include <hpx/hpx_suspend.hpp>
//
#include <hpx/include/apply.hpp>
//
#include <hpx/util/yield_while.hpp>
#include <hpx/include/threadmanager.hpp>
//
#include <iostream>

#define WORKAROUND

int main(int argc, char* argv[])
{
#ifdef WORKAROUND
    //THis is the necessary workaround
    int my_argc = 3;
    char app[] = "backend_launch";
    char opt[] = "--hpx:threads";
    char val[]= "4";
    char* app_ptr = app;
    char* opt_ptr = opt;
    char* val_ptr = val;

    char * my_argv[] = {app_ptr, opt_ptr, val_ptr};
#else
    //This is the expected code
    int my_argc = 2;
    char opt[] = "--hpx:threads";
    char val[]= "4";
    char* opt_ptr = opt;
    char* val_ptr = val;

    char * my_argv[] = {opt_ptr, val_ptr};
#endif

    hpx::start(nullptr, my_argc, my_argv);

    // Wait for runtime to start
    hpx::runtime* rt = hpx::get_runtime_ptr();
    hpx::util::yield_while([rt]()
                           { return rt->get_state() < hpx::state_running; });


    std::size_t num_work_threads = -1;

    hpx::apply( [&]() {
        num_work_threads = hpx::get_num_worker_threads();
    });

    hpx::apply([]() { hpx::finalize(); });
    hpx::stop();

    std::cout << "Num worker threads: " << num_work_threads;
    std::cout << " (shoudl be: " << val_ptr << ")";

    return 0;
}

