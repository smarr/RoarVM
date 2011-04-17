Installation and Usage Instructions for the RoarVM
==================================================

This file briefly describes how to compile and use the RoarVM.

Installation
------------

Requirements:

 - gcc, g++ (also tested with icc v11 on Linux)
 - Ubuntu: libx11-dev, libxext-dev
 - Mac OS X: Xcode Developer Tools

Compilation:

In the standard case, calling ./configure ; make in the build directory
should be sufficient to compile the rvm executable::

  $ cd build
  $ ./configure
  $ make

Compilation for Debugging::

  $ cd build
  $ ./configure --debug
  $ make

Known Issues:

The standard compiler on modern Linux systems is currently not fully
supported. Some of its optimizations lead to crashes in the RoarVM. Configure
supports a workaround which reduces the optimization level for affected files.
Until the bug in the RoarVM is fixed, please use::

  $ ./configure --opt-workaround

Usage
-----

The RoarVM executable supports the following command-line interface::

  ./rvm [options] <image-file> [app-params]
  
 [options]       optional command-line parameters as detailed below 
 <image-file>    a relative path to a Smalltalk image
 [app-params]    parameters given the application executed by the RoarVM

Command-line Parameters::

 -headless       initializes the RoarVM with a dummy display to avoid opening
                 an X11 session, useful for command-line applications or
                 benchmarks
               
 -num_core N     starts the RoarVM with N interpreter instances, each running
                 on a dedicated processor core
               
 -geom N,M       starts the RoarVM with N*M interpreter instances, each
                 running on a dedicated processor core. REMARK: this option is
                 meant for TILE64 processors, where the interpreter instances
                 are distributed on the 2D mesh of cores in an N*M layout

 -min_heap_MB N  sets the lower limit for the overall heap size
                 

Filing in RoarVM Changes
''''''''''''''''''''''''

To use your current image on top of the RoarVM, a few changes need to be
applied to the image.

  1. Chose the suitable support file from /image.st/
  2. Open you image (depending on the image, you need to use the SqueakVM)
  3. Open a file list in your image
  4. Pick the chosen support file and install it or file it in
  5. Acknowledge the change to the Process class by pressing proceed in the
     warning dialog window
  6. Save and quit the image
  7. Run RoarVM with a num_core setting > 1

Remark:

  The support for Squeak and Pharo has its limitations. Be aware that those
  Smalltalks have not been developed with hardware parallelism in mind. It is
  very likely that you will run into problems that are caused by the
  assumption that only a single Smalltalk Process is active at a time, and
  that the scheduler has certain properties like switching between processes
  only at known places.

