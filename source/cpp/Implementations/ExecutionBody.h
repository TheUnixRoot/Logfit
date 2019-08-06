
#ifndef _BODY_TASK_
#define _BODY_TASK_

#if defined(__APPLE__)
#include <OpenCL/cl.h>
#else

#include <CL/cl.h>

#endif

#include "tbb/parallel_for.h"
#include "tbb/task.h"
#include "tbb/tick_count.h"
#include "../DataStructures/ClientDataStructures.h"
#include "../Interfaces/IBody.h"

using namespace tbb;

const int NUM_RAND = 256;

int RandomNumber() { return (std::rand() % NUM_RAND); }

/*****************************************************************************
 * class Body
 * **************************************************************************/
class ExecutionBody : IBody<dim_range, t_index, buffer_f, buffer_f, buffer_f> {
public:
    const size_t vsize = 10;
    dim_range ndRange;
    buffer_f Adevice;
    buffer_f Bdevice;
    buffer_f Cdevice;
    float *Ahost;
    float *Bhost;
    float *Chost;
public:

    ExecutionBody() : ndRange{1}, Adevice{vsize}, Bdevice{vsize}, Cdevice{vsize}, Ahost{Adevice.data()}, Bhost{Bdevice.data()},
             Chost{Cdevice.data()} {

        std::generate(Ahost, Ahost + vsize, RandomNumber);
        std::generate(Bhost, Bhost + vsize, RandomNumber);
        std::generate(Chost, Chost + vsize, []{return -1;});
    }

    void OperatorCPU(int begin, int end) {
        for (int i = begin; i < end; i++) {
            Chost[i] = Ahost[i] + Bhost[i];
        }
    }

    void ShowCallback() {
        for (int i = 0; i < vsize; i++) {
            std::cout << i << ": " << Ahost[i] << " + " << Bhost[i] << " = " << Chost[i] << std::endl;
        }
    }

    type_gpu_node GetGPUArgs(t_index indexes) {
        return std::make_tuple(indexes, Adevice, Bdevice, Cdevice);
    };

    int GetVsize() {
        return vsize;
    }

    dim_range GetNDRange() {
        return ndRange;
    }

};
//end class

#endif
