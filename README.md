Mandelbrot and Julia sets parallel generation using pthreads.

In the implementation I've used a barrier for P+1 threads, so that I generate
the two sets and write their output creating the threads only one time.
