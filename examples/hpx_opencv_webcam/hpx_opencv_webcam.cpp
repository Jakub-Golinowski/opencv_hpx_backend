#include <hpx/hpx_init.hpp>
//
#include <hpx/parallel/algorithms/for_loop.hpp>
#include <hpx/parallel/execution.hpp>
//
#include <hpx/runtime/resource/partitioner.hpp>
//
#include <hpx/include/iostreams.hpp>
//
#include "system_characteristics.hpp"
//
#include <iostream>
//
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
//
#include <config.h>

///////////////////////////////////////////////////////////////////////////
/// Global variables
static int opencv_tp_num_threads = 1;

static std::string opencv_tp_name("opencv");

///////////////////////////////////////////////////////////////////////////
/// Function Definitions
cv::Mat load_image(const std::string &path) {
    hpx::cout << "load_image from " << path << "\n";
    cv::Mat image = cv::imread(path, 1);
    if (!image.data)
        throw std::runtime_error("No image data \n");

    return image;
}

cv::Mat take_webcam_image() {
    hpx::cout << "Taking webcam image.";
    cv::VideoCapture cap;

    // arg=0 opens the default camera
    if(!cap.open(0))
        return cv::Mat();
    cv::Mat frame;
    cap >> frame;

    return frame;
}

int detect_face(cv::Mat& img, cv::CascadeClassifier& cascade,
                cv::CascadeClassifier& nestedCascade, double scale) {

    std::vector<cv::Rect> faces, faces2;
    cv::Mat gray, smallImg;

    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY); // Convert to Gray Scale
    double fx = 1 / scale;

    // Resize the Grayscale Image
    resize(gray, smallImg, cv::Size(), fx, fx, cv::INTER_LINEAR);
    equalizeHist(smallImg, smallImg);

    // Detect faces of different sizes using cascade classifier
    cascade.detectMultiScale(smallImg, faces, 1.1,
                              2, 0|cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30));

    // Draw circles around the faces
    for (size_t i = 0; i < faces.size(); i++)
    {
        cv::Rect r = faces[i];
        cv::Mat smallImgROI;
        std::vector<cv::Rect> nestedObjects;
        cv::Point center;
        cv::Scalar color = cv::Scalar(255, 0, 0); // Color for Drawing tool
        int radius;

        rectangle(img,
                  cv::Point(cvRound(r.x*scale), cvRound(r.y*scale)),
                  cv::Point(cvRound((r.x + r.width-1)*scale),
                  cvRound((r.y + r.height-1)*scale)),
                  color, 2, 8, 0);
        if(nestedCascade.empty())
            continue;
        smallImgROI = smallImg(r);

        // Detection of eyes int the input image
        nestedCascade.detectMultiScale(smallImgROI, nestedObjects, 1.1, 2,
                                        0|cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30));

        // Draw circles around eyes
        for (size_t j = 0; j < nestedObjects.size(); j++)
        {
            cv::Rect nr = nestedObjects[j];
            center.x = cvRound((r.x + nr.x + nr.width*0.5)*scale);
            center.y = cvRound((r.y + nr.y + nr.height*0.5)*scale);
            radius = cvRound((nr.width + nr.height)*0.25*scale);
            circle(img, center, radius, color, 2, 8, 0);
        }
    }

    return 0;
}

int start_webcam_capture(int num_frames, cv::CascadeClassifier cascade,
                         cv::CascadeClassifier nestedCascade, double scale,
                         hpx::threads::executors::pool_executor def_exec) {
    cv::VideoCapture capture;
    if(!capture.open(0)){
        hpx::cout << "Could not open camera...";
        return -1;
    }

    std::string curr_pool_name = hpx::threads::get_pool(
            hpx::threads::get_self_id())->get_pool_name();

    // Capture frames from video and detect faces
    hpx::cout << "Starting face detection... \n";
    cv::Mat captured_frame, processed_frame;
    while(true) {
        capture >> captured_frame;
        if(captured_frame.empty())
            break;

        processed_frame = captured_frame.clone();

        hpx::future<int> detection_flag_f =
                hpx::async(def_exec, detect_face, std::ref(processed_frame),
                           std::ref(cascade), std::ref(nestedCascade), scale);

        int detection_flag = detection_flag_f.get();

        if(detection_flag == 0)
        {
            cv::imshow("Face Recognition from " + curr_pool_name,
                       processed_frame);
            char c = (char)cv::waitKey(1000/num_frames);
            // Press q to exit from window
            if(c == 27 || c == 'q' || c == 'Q')
                break;
        }
        else{
            break;
        }
    }

    return 0;
}

// Warning - for now it is blocking forever (until user closes the window).
// Should be run in a separate thread?
void show_image(const cv::Mat &image, std::string win_name) {
    hpx::cout << "show_image in " << win_name << "\n";
    cv::namedWindow(win_name, cv::WINDOW_AUTOSIZE);
    imshow(win_name, image);
    cv::waitKey(0);
}

void save_image(const cv::Mat &image, const std::string &path) {
    cv::imwrite("Gray_Image.jpg", image);
}

cv::Mat transform_to_grey(cv::Mat image) {
    hpx::cout << "transform to grey" << "\n";
    cv::Mat grey_image;
    cv::cvtColor(image, grey_image, cv::COLOR_RGB2GRAY);
    return grey_image;
}

void print_system_params() {
    // print partition characteristics
    hpx::cout << "\n\n[hpx_main] print resource_partitioner characteristics : "
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

///////////////////////////////////////////////////////////////////////////
/// hpx_main is called on an hpx thread after the runtime starts up
int hpx_main(boost::program_options::variables_map& vm)
{
    // ======================== CONFIGURATION ========================
    hpx::cout << "[hpx_main] starting hpx_main in "
              << hpx::threads::get_pool(hpx::threads::get_self_id())->get_pool_name()
              << "\n";

    std::size_t num_work_threads = hpx::get_num_worker_threads();
    hpx::cout << "[hpx_main] HPX using threads = " << num_work_threads << "\n";

    hpx::threads::executors::pool_executor def_executor("default");
    hpx::threads::executors::pool_executor opencv_executor(opencv_tp_name);
    hpx::cout << "[hpx_main] Created default and " << opencv_tp_name
              << " pool_executors \n";

    print_system_params();

    int num_frames = vm["num_frames"].as<int>();
    bool detect_eyes = vm["detect_eyes"].as<bool>();

    // ======================== ACTUAL MAIN ========================

    // PreDefined trained XML classifiers with facial features
    cv::CascadeClassifier cascade, nestedCascade;
    double scale=1;

    // Load classifiers
    std::string nestedCascadePath = DATA_PATH + std::string("/models/haarcascade_eye_tree_eyeglasses.xml");
    std::string cascadePath = DATA_PATH + std::string("/models/haarcascade_frontalface_default.xml");
    
    if(detect_eyes)
        nestedCascade.load(nestedCascadePath);
    cascade.load(cascadePath);

    hpx::future<int> webcam_closed =
            hpx::async<>(opencv_executor, &start_webcam_capture, num_frames,
                         cascade, nestedCascade, scale, def_executor);

    cv::Mat webcam_img;
    cv::Mat grey_webcam_img;
    // schedule taking webcam image on the opencv pool
    hpx::future<cv::Mat> f_image =
            webcam_closed.then([&](hpx::future<int>&& wc){
                hpx::cout << "Webcam closed\n";
                return take_webcam_image();
            });

    // schedule transforming webcam image to grey-scale on opencv pool
    hpx::future<cv::Mat> f_grey_image =
            f_image.then([&](hpx::future<cv::Mat>&& fi){
                hpx::cout << "Image taken\n";
                webcam_img = fi.get();
                return transform_to_grey(webcam_img);
            });

    f_grey_image.then([&](hpx::future<cv::Mat>&& fgi){
            hpx::cout << "Image transformed to grey\n";
            grey_webcam_img = fgi.get();
            return save_image(grey_webcam_img, "");
        });

    return hpx::finalize();
}

///////////////////////////////////////////////////////////////////////////
// Normal int main function that is called at startup and runs on an OS thread
// the user must call hpx::init to start the hpx runtime which will execute
// hpx_main on an hpx thread
int main(int argc, char* argv[])
{
    namespace po = boost::program_options;
    po::options_description desc_cmdline("Options");
    desc_cmdline.add_options()
        ("opencv_tp_num_threads,m",
          po::value<int>()->default_value(1),
          "Number of threads to assign to custom pool")
        ("num_frames,f",
         po::value<int>()->default_value(25),
         "Number of frames per second in the video stream.")
        ("detect_eyes,e",
         po::value<bool>()->default_value(false),
         "If set to true the application will detect eyes.");

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

    opencv_tp_num_threads = vm["opencv_tp_num_threads"].as<int>();

    // Create the resource partitioner
    hpx::resource::partitioner rp(desc_cmdline, argc, argv);
    std::cout << "[main] obtained reference to the resource_partitioner\n";

    rp.create_thread_pool("default",
                      hpx::resource::scheduling_policy::local_priority_fifo);
    std::cout << "[main] " << "thread_pool default created\n";

    // Create a thread pool using the default scheduler provided by HPX
    rp.create_thread_pool(opencv_tp_name,
        hpx::resource::scheduling_policy::local_priority_fifo);

    std::cout << "[main] " << "thread_pool "
              << opencv_tp_name << " created\n";

    // add PUs to opencv pool
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

    return hpx::init();
}
