Note that all the runs were performed for modified number of iterations per single parallel_for body. In all previous experiments the default value from Mandelbrot example was used (500). In this runs I decided to change this value to 10 in order to make single atom of work smaller and therefore make effects of suboptimal chunking visible.

Also to avoid effects connected with hyper-threading the runs were done with 4 threads. The full configuration is as below:

h=4800 w=5400 backend=tbb num_threads=4 mandelbrot_iter=10 nstripes=!VARIED! sequential=0