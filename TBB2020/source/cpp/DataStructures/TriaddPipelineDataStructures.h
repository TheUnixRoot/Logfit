//
// Created by juanp on 7/11/20.
//

#ifndef BARNESLOGFIT_TRIADDPIPELINEDATASTRUCTURES_H
#define BARNESLOGFIT_TRIADDPIPELINEDATASTRUCTURES_H
#if defined(__APPLE__)
#include <OpenCL/cl.h>
#else

#include <CL/cl.h>

#endif

#include <vector>
using t_index = struct _t_index {
    int begin, end;
};
using buffer_f = std::vector<float>;
cl_mem Adevice;
cl_mem Bdevice;
cl_mem Cdevice;
#endif //BARNESLOGFIT_TRIADDPIPELINEDATASTRUCTURES_H
