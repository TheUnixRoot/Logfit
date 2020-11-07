!/bin/bash

/home/juanp/logfit/Graph/cmake-build-debug/Logfit -f /home/juanp/logfit/Graph/data/runA.in -c 0 -g 1 -r 0.5 -k ComputeForce -o /home/juanp/logfit/Graph/source/cl/kernel.cl > ./BarnesWithoutCPU.txt
/home/juanp/logfit/Graph/cmake-build-debug/Logfit -f /home/juanp/logfit/Graph/data/runA.in -c 2 -g 0 -r 0.5 -k ComputeForce -o /home/juanp/logfit/Graph/source/cl/kernel.cl > ./BarnesWithoutGPU.txt
/home/juanp/logfit/Graph/cmake-build-debug/Logfit -f /home/juanp/logfit/Graph/data/runA.in -c 2 -g 1 -r 0.5 -k ComputeForce -o /home/juanp/logfit/Graph/source/cl/kernel.cl > ./BarnesWithGPU.txt
