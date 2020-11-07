
#ifndef _BODY_TASK_
#define _BODY_TASK_

#if defined(__APPLE__)
#include <OpenCL/cl.h>
#else

#include <CL/cl.h>

#endif

#include <body/IGraphBody.h>
#include "../../DataStructures/TriaddGraphDataStructures.h"

using namespace tbb;

const int NUM_RAND = 256;

inline int RandomNumber() { return (std::rand() % NUM_RAND); }

/*****************************************************************************
 * class Body
 * **************************************************************************/
class TriaddGraphBody : public IGraphBody<dim_range, t_index, buffer_f, buffer_f, buffer_f> {
public:
    size_t vsize;
    dim_range ndRange;
    buffer_f Adevice;
    buffer_f Bdevice;
    buffer_f Cdevice;
    float *Ahost;
    float *Bhost;
    float *Chost;
public:

    TriaddGraphBody(std::size_t vsize = 10000000) : vsize{vsize}, ndRange{vsize}, Adevice{vsize}, Bdevice{vsize},
                                                    Cdevice{vsize},
                                                    Ahost{Adevice.data()}, Bhost{Bdevice.data()},
                                                    Chost{Cdevice.data()} {

        std::generate(Ahost, Ahost + vsize, RandomNumber);
        std::generate(Bhost, Bhost + vsize, RandomNumber);
        std::generate(Chost, Chost + vsize, RandomNumber);
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
    friend class TriaddGraphBodyTest;
};
//end class

#endif