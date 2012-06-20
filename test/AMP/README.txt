boltProject:

This directory contains the "Bolt" project.  
  * Bolt is a C++ template function library similar in spirit to the STL 
    "algorithm" header.  It includes a hybrid of functions from STL (ie 
    transform, reduce, sort, etc) as well as functions optimized for use 
    on HSA APUS (ie pipeline, parallel_do).
  * Bolt is optimized for HSA APUs and leverages features including 
    Shared Virual Memory (smooth programming model and performance), 
    GPU and CPU, and the advanced queueing features of HSA GPUs.
  * Bolt currently runs on C++AMP and eventually will run on OpenCL. 
    To run the samples, you need the "Developer Preview" version of 
    Visual Studio Dev11, available via MSDN or here:
    http://msdn.microsoft.com/en-us/vstudio/hh127353
    

* Directory Structure:
    * /bolt : Header files for bolt template function library
    * /tests : Projects that demonstrate simple use cases and functional tests.
