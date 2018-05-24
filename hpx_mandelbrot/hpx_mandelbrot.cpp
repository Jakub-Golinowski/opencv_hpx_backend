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
#include <stdexcept>
#include <string>
#include <utility>
//
#include <iostream>
#include <opencv2/opencv.hpp>

///////////////////////////////////////////////////////////////////////////
/// Global Variables (Parameters)

static int opencv_tp_num_threads = 1;
static const std::string opencv_tp_name("opencv");
static int dflt_mandelbrot_size = 500;

///////////////////////////////////////////////////////////////////////////
/// Function Declarations

cv::Mat load_image(const std::string& path);
void show_image(const cv::Mat& image, std::string win_name);
void save_image(const cv::Mat& image, const std::string&);

int mandelbrot(const std::complex<float>& z0, const int max);
int mandelbrotFormula(const std::complex<float>& z0, const int maxIter = 500);

void print_system_params();

///////////////////////////////////////////////////////////////////////////
/// Function Definitions

cv::Mat load_image(const std::string& path)
{
    hpx::cout << "load_image from " << path << "\n";
    cv::Mat image = cv::imread(path, 1);
    if (!image.data)
        throw std::runtime_error("No image data \n");

    return image;
}

// Warning - for now it is blocking forever (until user closes the window).
// Should be run in a separate thread?
void show_image(const cv::Mat& image, std::string win_name)
{
    hpx::cout << "show_image in " << win_name << "\n";
    cv::namedWindow(win_name, cv::WINDOW_AUTOSIZE);
    imshow(win_name, image);
    cv::waitKey(0);
}

void save_image(const cv::Mat& image, const std::string& path)
{
    cv::imwrite(path, image);
}

int mandelbrot(const std::complex<float>& z0, const int max)
{
    std::complex<float> z = z0;
    for (int t = 0; t < max; t++)
    {
        if (z.real() * z.real() + z.imag() * z.imag() > 4.0f)
            return t;
        z = z * z + z0;
    }

    return max;
}

void print_system_params()
{
    // print partition characteristics
    hpx::cout << "\n\n"
              << "[hpx_main] print resource_partitioner characteristics : "
              << "\n";
    hpx::resource::get_partitioner().print_init_pool_data(std::cout);

    // print partition characteristics
    hpx::cout << "\n\n[hpx_main] print thread-manager pools : "
              << "\n";
    hpx::threads::get_thread_manager().print_pools(std::cout);

    // print system characteristics
    hpx::cout << "\n\n[hpx_main] print System characteristics : "
              << "\n";
    print_system_characteristics();
}

int mandelbrotFormula(const std::complex<float>& z0, const int maxIter)
{
    int value = mandelbrot(z0, maxIter);
    if (maxIter - value == 0)
    {
        return 0;
    }

    return cvRound(sqrt(value / (float) maxIter) * 255);
}

///////////////////////////////////////////////////////////////////////////
using namespace hpx::threads::policies;

///////////////////////////////////////////////////////////////////////////
int hpx_main(boost::program_options::variables_map& vm)
{
    hpx::cout << "[hpx_main] starting ..."
              << "\n";

    std::size_t num_work_threads = hpx::get_num_worker_threads();
    hpx::cout << "HPX using threads = " << num_work_threads << "\n";

    hpx::threads::executors::default_executor default_executor;
    hpx::threads::scheduled_executor opencv_executor;
    hpx::threads::executors::pool_executor opencv_exec(opencv_tp_name);
    opencv_executor = opencv_exec;
    hpx::cout << "[hpx_main] got opencv executor "
              << "\n";

    print_system_params();

    cv::Mat mandelbrotImg(
        vm["mb-size"].as<int>(), vm["mb-size"].as<int>(), CV_8U);
    float x1 = -2.1f, x2 = 0.6f;
    float y1 = -1.2f, y2 = 1.2f;
    float scaleX = mandelbrotImg.cols / (x2 - x1);
    float scaleY = mandelbrotImg.rows / (y2 - y1);

    int num_pixels = mandelbrotImg.rows * mandelbrotImg.cols;

    hpx::parallel::execution::static_chunk_size fixed(
        4 * num_pixels / num_work_threads);
    hpx::parallel::for_loop_strided(
        hpx::parallel::execution::par.with(fixed).on(default_executor), 0,
        num_pixels, 1, [&](std::size_t r) {
            // std::lock_guard<hpx::lcos::local::mutex> lock(m);
            std::size_t i = r / mandelbrotImg.cols;
            std::size_t j = r % mandelbrotImg.cols;
            float x0 = j / scaleX + x1;
            float y0 = i / scaleY + y1;
            std::complex<float> z0(x0, y0);
            uchar value = (uchar) mandelbrotFormula(z0);
            mandelbrotImg.ptr<uchar>(i)[j] = value;
        });

    hpx::future<void> f_save_image = hpx::async(
        opencv_executor, &save_image, mandelbrotImg, "mandelbrot.bmp");
    hpx::async(opencv_executor, &show_image, mandelbrotImg, "Mandelbrot");

    return hpx::finalize();
}

///////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    namespace po = boost::program_options;
    po::options_description desc_cmdline("Options");
    desc_cmdline.add_options()("opencv_tp_num_threads,m",
        po::value<int>()->default_value(1),
        "Number of threads to assign to opencv pool")("mb-size",
        po::value<int>()->default_value(500),
        "Specify the edge length of square mandelbrot image");

    // HPX uses a boost program options variable map, but we need it before
    // hpx-main, so we will create another one here and throw it away after use
    po::variables_map vm;
    try
    {
        po::store(po::command_line_parser(argc, argv)
                      .allow_unregistered()
                      .options(desc_cmdline)
                      .run(),
            vm);
    }
    catch (po::error& e)
    {
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
        std::cerr << desc_cmdline << std::endl;
        return -1;
    }

    opencv_tp_num_threads = vm["opencv_tp_num_threads"].as<int>();

    // Create the resource partitioner
    hpx::resource::partitioner rp(desc_cmdline, argc, argv);
    std::cout << "[main] obtained reference to the resource_partitioner"
              << std::endl;

    rp.create_thread_pool(
        opencv_tp_name, hpx::resource::scheduling_policy::local_priority_fifo);
    std::cout << "[main] "
              << "thread_pool " << opencv_tp_name << " created" << std::endl;

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
                    std::cout << "[main] Added pu " << count++ << " to "
                              << opencv_tp_name << "thread pool" << std::endl;
                    rp.add_resource(p, opencv_tp_name);
                }
            }
        }
    }

    std::cout << "[main] resources added to thread_pools" << std::endl;

    return hpx::init();
}