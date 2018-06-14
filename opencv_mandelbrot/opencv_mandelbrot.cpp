#include <config.hpp>
//
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
//
#include <opencv2/core/parallel.hpp>

#ifndef BACKEND_STARTSTOP
#include <hpx/hpx_main.hpp>
#endif

#include <hpx/include/runtime.hpp>

#include "boost/program_options.hpp"
using namespace std;
using namespace cv;

namespace {
    //! [mandelbrot-escape-time-algorithm]
    int mandelbrot(const complex<float>& z0, const int max)
    {
        complex<float> z = z0;
        for (int t = 0; t < max; t++)
        {
            if (z.real() * z.real() + z.imag() * z.imag() > 4.0f)
                return t;
            z = z * z + z0;
        }

        return max;
    }
    //! [mandelbrot-escape-time-algorithm]

    //! [mandelbrot-grayscale-value]
    int mandelbrotFormula(const complex<float>& z0, const int maxIter = 500)
    {
        int value = mandelbrot(z0, maxIter);
        if (maxIter - value == 0)
        {
            return 0;
        }

        return cvRound(sqrt(value / (float) maxIter) * 255);
    }
    //! [mandelbrot-grayscale-value]

    //! [mandelbrot-sequential]
    void sequentialMandelbrot(Mat& img, const float x1, const float y1,
        const float scaleX, const float scaleY, const int maxIter = 500)
    {
        for (int i = 0; i < img.rows; i++)
        {
            for (int j = 0; j < img.cols; j++)
            {
                float x0 = j / scaleX + x1;
                float y0 = i / scaleY + y1;

                complex<float> z0(x0, y0);
                uchar value = (uchar) mandelbrotFormula(z0, maxIter);
                img.ptr<uchar>(i)[j] = value;
            }
        }
    }
    //! [mandelbrot-sequential]
}

#ifndef BACKEND_NON_HPX
void set_argc_argv(int argc, char* argv[])
{
    __argc = argc;
    __argv = argv;
}
#endif

int main(int argc, char* argv[])
{
#ifndef BACKEND_NON_HPX
    set_argc_argv(argc, argv);
#endif

    namespace po = boost::program_options;
    po::options_description desc_cmdline("Options");
    desc_cmdline.add_options()("height,h",
        po::value<int>()->default_value(1000),
        "Height of mandelbrot image")("width,w",
        po::value<int>()->default_value(2000), "Width of mandelbrot image")(
        "backend,b", po::value<std::string>()->default_value(""),
        "Name of backend used. (Should be in accordance with actually used "
        "backend as it has no real influence but is just used for logging)")(
        "hpx:threads,t", po::value<int>()->default_value(4),
        "name of backend used")("mandelbrot_iter,i",
        po::value<int>()->default_value(500),
        "Number of iterations for single pixel of mandelbrot image")(
        "nstripes,n", po::value<double>()->default_value(-1.),
        "Value of OpenCV nstripe parameter enforcing the chunkign on the "
        "backend.")("sequential,s", po::value<bool>()->default_value(false),
        "Run the mandelbrot sequentially, bypassing parallel backends");

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
        std::cerr << "ERROR: " << e.what() << "\n"
                  << "\n";
        std::cerr << desc_cmdline << "\n";
        return -1;
    }

    std::string backend = vm["backend"].as<std::string>();
    int mandelbrotHeight = vm["height"].as<int>();
    int mandelbrotWidth = vm["width"].as<int>();
    int mandelbrotMaxIter = vm["mandelbrot_iter"].as<int>();
    double nstripes = vm["nstripes"].as<double>();
    bool sequential = vm["sequential"].as<bool>();

#ifndef BACKEND_STARTSTOP
    int num_threads = hpx::get_num_worker_threads();
#else
    int num_threads = vm["hpx:threads"].as<int>();
#endif

    std::cout << "h=" << mandelbrotHeight << " w=" << mandelbrotWidth
              << " backend=" << backend << " num_threads=" << num_threads
              << " mandelbrot_iter=" << mandelbrotMaxIter
              << " nstripes=" << nstripes
              << " sequential=" << std::to_string(sequential) << std::endl;

    //! [mandelbrot-transformation]
    Mat mandelbrotImg(mandelbrotHeight, mandelbrotWidth, CV_8U);
    float x1 = -2.1f, x2 = 0.6f;
    float y1 = -1.2f, y2 = 1.2f;
    float scaleX = mandelbrotImg.cols / (x2 - x1);
    float scaleY = mandelbrotImg.rows / (y2 - y1);
    //! [mandelbrot-transformation]

    if (sequential)
    {
        double t_seq = (double) getTickCount();
        sequentialMandelbrot(
            mandelbrotImg, x1, y1, scaleX, scaleY, mandelbrotMaxIter);
        t_seq = ((double) getTickCount() - t_seq) / getTickFrequency();
        cout << "Sequential Mandelbrot Execution Time: " << t_seq << " s"
             << endl;
    }
    else
    {
        cv::setNumThreads(num_threads);

        double t1 = (double) getTickCount();
        parallel_for_(Range(0, mandelbrotImg.rows * mandelbrotImg.cols),
            [&](const Range& range) {
                for (int r = range.start; r < range.end; r++)
                {
                    int i = r / mandelbrotImg.cols;
                    int j = r % mandelbrotImg.cols;

                    float x0 = j / scaleX + x1;
                    float y0 = i / scaleY + y1;

                    complex<float> z0(x0, y0);
                    uchar value =
                        (uchar) mandelbrotFormula(z0, mandelbrotMaxIter);
                    mandelbrotImg.ptr<uchar>(i)[j] = value;
                }
            },
            nstripes);

        t1 = ((double) getTickCount() - t1) / getTickFrequency();
        cout << "Parallel Mandelbrot Execution Time: " << t1 << " s" << endl;
    }

    std::string im_name("Mandelbrot_h" + std::to_string(mandelbrotHeight) +
        "_w" + std::to_string(mandelbrotWidth) + "_t" +
        std::to_string(num_threads) + "_i" + std::to_string(mandelbrotMaxIter) +
        "_n" + std::to_string(static_cast<int>(nstripes)) + "_s" +
        std::to_string(sequential) + "_" + backend + ".png");

    imwrite(im_name, mandelbrotImg);

    return EXIT_SUCCESS;
}
