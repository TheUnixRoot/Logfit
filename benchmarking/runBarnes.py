#!/usr/bin/python

import os
import time

niter = [1, 2, 3, 4, 5]
numCpus = [0, 1, 2, 3, 4, 5, 6, 7, 8]
numGpus = [0, 1]
ratio = 0.5
barnesExecs = ["BarnesGraph", "BarnesOneApi", "BarnesPipeline", "BarnesOnePipeline"]
print("Running 1 step")
for iter in niter:
    for barnes in barnesExecs:
        for numCpu in numCpus:
            for numGpu in numGpus:
                args = f'-b {barnes} -f "/home/juanp/Desktop/TFG/Logfit/oneTBB2020/data/runC1.in" -c {numCpu} -g {numGpu} -r {ratio} -k ComputeForce -o "/home/juanp/Desktop/TFG/Logfit/oneTBB2020/source/cl/kernel.cl"'
                os.system(f'"/home/juanp/Desktop/TFG/Logfit/benchmarking/{barnes}" {args}')
print("Running 5 steps")
for iter in niter:
    for barnes in barnesExecs:
        for numCpu in numCpus:
            for numGpu in numGpus:
                args = f'-b {barnes} -f "/home/juanp/Desktop/TFG/Logfit/oneTBB2020/data/runC5.in" -c {numCpu} -g {numGpu} -r {ratio} -k ComputeForce -o "/home/juanp/Desktop/TFG/Logfit/oneTBB2020/source/cl/kernel.cl"'
                os.system(f'"/home/juanp/Desktop/TFG/Logfit/benchmarking/{barnes}" {args}')

print("Running 75 steps")
for iter in niter:
    for barnes in barnesExecs:
        for numCpu in numCpus:
            for numGpu in numGpus:
                args = f'-b {barnes} -f "/home/juanp/Desktop/TFG/Logfit/oneTBB2020/data/runC75.in" -c {numCpu} -g {numGpu} -r {ratio} -k ComputeForce -o "/home/juanp/Desktop/TFG/Logfit/oneTBB2020/source/cl/kernel.cl"'
                os.system(f'"/home/juanp/Desktop/TFG/Logfit/benchmarking/{barnes}" {args}')
# \*+\n([0-8]{1}) +([0-1]{1}) +[0-9\.]+\n[+]+\n([0-9\n]+)\n\* - $1-$2;\n$3 ---- getting chunks