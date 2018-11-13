# Logfit
## Intel TBB library based dynamic parallel computing scheduler
Conceptually, this implementation works like an abstraction of the Intel TBB graph library, providing to its users the capability of compute their code written in C++11 and OpenCL in parallel against CPU and integrated processor GPU.

## Implementation approach
The library is under development but at this point a sample 2-vector sum is computed in parallel. 

To develop the parallel executor, I applied some design patterns, such the Strategy one for the algorithm in charge of split the data that is running in each device (CPU & GPU) and interfaces to guide the development of the code that want to be parallel run by Logfit users.
