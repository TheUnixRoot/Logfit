#!/usr/bin/python

import os
import time

niter = [1]
numCpus = [1]
numGpus = [0, 1]
ratio = 0.5
barnesExecs = ["BarnesPipeline", "BarnesOnePipeline"]
for barnes in barnesExecs:
    for iter in niter:
        args = f'-b {barnes} -f "/home/juanp/Desktop/TFG/Logfit/oneTBB2020/data/runC.in" -c 0 -g 1 -r {ratio} -k ComputeForce -o "/home/juanp/Desktop/TFG/Logfit/oneTBB2020/source/cl/kernel.cl"'
        os.system(f'"/home/juanp/Desktop/TFG/benchmarking/{barnes}" {args}')
    for numCpu in numCpus:
        for numGpu in numGpus:
            for iter in niter:
                args = f'-b {barnes} -f "/home/juanp/Desktop/TFG/Logfit/oneTBB2020/data/runC.in" -c {numCpu} -g {numGpu} -r {ratio} -k ComputeForce -o "/home/juanp/Desktop/TFG/Logfit/oneTBB2020/source/cl/kernel.cl"'
                os.system(f'"/home/juanp/Desktop/TFG/benchmarking/{barnes}" {args}')
