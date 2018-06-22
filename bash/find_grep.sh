#/bin/bash

#Find all directories that are named "test"
find ./ -type d \
        -not \( -path */cmake-build-release/* -prune \) \
        -not \( -path */cmake-build-debug/* -prune \)  \
        -not \( -path */build/* -prune \)  \
        -name test \
        -print0 \
        | xargs -0 \
         grep -rH "parallel_for_"				

# Search result
./modules/cudaimgproc/test/test_canny.cpp:        cv::parallel_for_(cv::Range(0, NUM_STREAMS), CannyAsyncParallelLoopBody(d_img_roi, edges, low_thresh, high_thresh, apperture_size, useL2gradient));
./modules/videoio/test/test_ffmpeg.cpp:    parallel_for_(range, invoker1);
./modules/videoio/test/test_ffmpeg.cpp:    parallel_for_(range, WriteVideo_Invoker(writers));
./modules/videoio/test/test_ffmpeg.cpp:    parallel_for_(range, invoker2);
./modules/videoio/test/test_ffmpeg.cpp:    parallel_for_(range, ReadImageAndTest(readers, ts));
./modules/cudafeatures2d/test/test_features2d.cpp:        cv::parallel_for_(cv::Range(0, 2), FastAsyncParallelLoopBody(image, d_keypoints, d_fast));
./modules/core/test/test_misc.cpp:        parallel_for_(cv::Range(0, dst1.rows), ThrowErrorParallelLoopBody(dst1, -1));
./modules/core/test/test_misc.cpp:        parallel_for_(cv::Range(0, dst2.rows), ThrowErrorParallelLoopBody(dst2, dst2.rows / 2));
./modules/core/test/test_umat.cpp:        parallel_for_(cv::Range(0, 2), TestParallelLoopBody(u));
./modules/core/test/test_rand.cpp:TEST(Core_Rand, parallel_for_stable_results)
./modules/core/test/test_rand.cpp:    parallel_for_(cv::Range(0, dst1.rows), RandRowFillParallelLoopBody(dst1));
./modules/core/test/test_rand.cpp:    parallel_for_(cv::Range(0, dst2.rows), RandRowFillParallelLoopBody(dst2));

find ./ -type d \
        -not \( -path */cmake-build-release/* -prune \) \
        -not \( -path */cmake-build-debug/* -prune \)  \
        -not \( -path */build/* -prune \)  \
        -not \( -name "*build*" \) \
        -not \( -name "" \) \
        -print0 | xargs -0 \
        grep -H "parallel_for_"
     	