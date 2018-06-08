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

namespace
{
//! [mandelbrot-escape-time-algorithm]
int mandelbrot(const complex<float> &z0, const int max)
{
    complex<float> z = z0;
    for (int t = 0; t < max; t++)
    {
        if (z.real()*z.real() + z.imag()*z.imag() > 4.0f) return t;
        z = z*z + z0;
    }

    return max;
}
//! [mandelbrot-escape-time-algorithm]

//! [mandelbrot-grayscale-value]
int mandelbrotFormula(const complex<float> &z0, const int maxIter=500) {
    int value = mandelbrot(z0, maxIter);
    if(maxIter - value == 0)
    {
        return 0;
    }

    return cvRound(sqrt(value / (float) maxIter) * 255);
}
//! [mandelbrot-grayscale-value]

//! [mandelbrot-parallel]
class ParallelMandelbrot : public ParallelLoopBody
{
public:
    ParallelMandelbrot (Mat &img, const float x1, const float y1, const float scaleX, const float scaleY)
        : m_img(img), m_x1(x1), m_y1(y1), m_scaleX(scaleX), m_scaleY(scaleY)
    {
    }

    virtual void operator ()(const Range& range) const
    {
        for (int r = range.start; r < range.end; r++)
        {
            int i = r / m_img.cols;
            int j = r % m_img.cols;

            float x0 = j / m_scaleX + m_x1;
            float y0 = i / m_scaleY + m_y1;

            complex<float> z0(x0, y0);
            uchar value = (uchar) mandelbrotFormula(z0);
            m_img.ptr<uchar>(i)[j] = value;
        }
    }

    ParallelMandelbrot& operator=(const ParallelMandelbrot &) {
        return *this;
    };

private:
    Mat &m_img;
    float m_x1;
    float m_y1;
    float m_scaleX;
    float m_scaleY;
};
//! [mandelbrot-parallel]

//! [mandelbrot-sequential]
void sequentialMandelbrot(Mat &img, const float x1, const float y1, const float scaleX, const float scaleY)
{
    for (int i = 0; i < img.rows; i++)
    {
        for (int j = 0; j < img.cols; j++)
        {
            float x0 = j / scaleX + x1;
            float y0 = i / scaleY + y1;

            complex<float> z0(x0, y0);
            uchar value = (uchar) mandelbrotFormula(z0);
            img.ptr<uchar>(i)[j] = value;
        }
    }
}
//! [mandelbrot-sequential]
}

void set_argc_argv(int argc, char* argv[])
{
    __argc = argc;
    __argv = argv;
}

int main(int argc, char* argv[])
{

    set_argc_argv(argc, argv);

    namespace po = boost::program_options;
    po::options_description desc_cmdline("Options");
    desc_cmdline.add_options()
            ("height,h", po::value<int>()->default_value(1000),
             "Height of mandelbrot image")
            ("width,w", po::value<int>()->default_value(2000),
             "width of mandelbrot image")
            ("backend,b", po::value<std::string>()->default_value(""),
            "name of backend used")
            ("hpx:threads,t", po::value<int>()->default_value(2),
             "name of backend used");

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
        std::cerr << "ERROR: " << e.what() << "\n" << "\n";
        std::cerr << desc_cmdline << "\n";
        return -1;
    }

    std::string backend = vm["backend"].as<std::string>();
    int num_pus = vm["hpx:threads"].as<int>();
    int mandelbrotHeight = vm["height"].as<int>();
    int mandelbrotWidth = vm["width"].as<int>();

    std::cout << "imsize: h=" << mandelbrotHeight << " w=" << mandelbrotWidth
              << " backend=" << backend
              << " num_pus=" << num_pus
              << std::endl;
    
    //! [mandelbrot-transformation]
    Mat mandelbrotImg(mandelbrotHeight, mandelbrotWidth, CV_8U);
    float x1 = -2.1f, x2 = 0.6f;
    float y1 = -1.2f, y2 = 1.2f;
    float scaleX = mandelbrotImg.cols / (x2 - x1);
    float scaleY = mandelbrotImg.rows / (y2 - y1);
    //! [mandelbrot-transformation]

    double t1 = (double) getTickCount();

    #ifdef CV_CXX11
//    std::cout << "C++11 defined." << std::endl;
    //! [mandelbrot-parallel-call-cxx11]
    parallel_for_(Range(0, mandelbrotImg.rows*mandelbrotImg.cols), [&](const Range& range){
        for (int r = range.start; r < range.end; r++)
        {
            int i = r / mandelbrotImg.cols;
            int j = r % mandelbrotImg.cols;

            float x0 = j / scaleX + x1;
            float y0 = i / scaleY + y1;

            complex<float> z0(x0, y0);
            uchar value = (uchar) mandelbrotFormula(z0);
            mandelbrotImg.ptr<uchar>(i)[j] = value;
        }
    });
    //! [mandelbrot-parallel-call-cxx11]

    #else
	std::cout << "C++11 *NOT* defined."
    //! [mandelbrot-parallel-call]
    ParallelMandelbrot parallelMandelbrot(mandelbrotImg, x1, y1, scaleX, scaleY);
    parallel_for_(Range(0, mandelbrotImg.rows*mandelbrotImg.cols), parallelMandelbrot);
    //! [mandelbrot-parallel-call]

    #endif

    t1 = ((double) getTickCount() - t1) / getTickFrequency();
    cout << "Parallel Mandelbrot: " << t1 << " s" << endl;

    Mat mandelbrotImgSequential(mandelbrotHeight, mandelbrotWidth, CV_8U);
    double t2 = (double) getTickCount();
    sequentialMandelbrot(mandelbrotImgSequential, x1, y1, scaleX, scaleY);
    t2 = ((double) getTickCount() - t2) / getTickFrequency();
    cout << "Sequential Mandelbrot: " << t2 << " s" << endl;
    cout << "Speed-up: " << t2/t1 << " X" << endl;

    std::string par_name("Mandelbrot_h" + std::to_string(mandelbrotHeight)
                + "_w" + std::to_string(mandelbrotWidth) + "_" +
                         backend + "_parallel.png");

    std::string seq_name("Mandelbrot_h" + std::to_string(mandelbrotHeight)
                         + "_w" + std::to_string(mandelbrotWidth) + "_" +
                         backend + "_sequential.png");

    imwrite(par_name, mandelbrotImg);
    imwrite(seq_name, mandelbrotImgSequential);

    return EXIT_SUCCESS;
}
