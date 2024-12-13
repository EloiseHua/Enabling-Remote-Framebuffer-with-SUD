Testing the Setup

To test the implementation, follow these steps:

Compile the Source Files: Navigate to the directory containing the source files and the Makefile, then run:

    make

Start the VNC Server: Run the start-vnc.sh script to start the VNC server on display :1:

    ./start-vnc.sh

Run the Application (e.g., mystify.c): With the framebuffer server and syscall interception in place, run the mystify application. It will use the modified mmap and ioctl calls to communicate with the VNC server:

    ./mystify

Connect to the VNC Server: Use a VNC client (e.g., vncviewer) to connect to the VNC server running on localhost:1. You should see the graphical output of the program, such as the bouncing lines animation, being rendered remotely.

    vncviewer localhost:1