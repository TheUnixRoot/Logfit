
#ifndef _BODY_TASK_
#define _BODY_TASK_

#if defined(__APPLE__)
#include <OpenCL/cl.h>
#else

#include <CL/cl.h>

#endif

#include <body/IGraphBody.h>
#include <numeric>
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
    std::vector<float> Chost;
public:

    TriaddGraphBody(std::size_t vsize = 10000000) : vsize{vsize}, ndRange{vsize}, Adevice{vsize}, Bdevice{vsize},
                                                    Cdevice{vsize} {
        Chost = std::vector<float>(vsize);
        std::iota(Adevice.begin(), Adevice.begin() + vsize, 1);
        std::iota(Bdevice.begin(), Bdevice.begin() + vsize, 1);
        std::generate(Cdevice.begin(), Cdevice.begin() + vsize, [](){ return 0;});
        std::generate(Chost.begin(), Chost.end(), [](){return 0;});
    }

    void OperatorCPU(int begin, int end) {
        auto size{end - begin};
        auto asub{Adevice.subbuffer(begin, size)};
        auto bsub{Bdevice.subbuffer(begin, size)};
        for (int i = 0; i < size; i++) {
            Chost[begin + i] = asub[i] + bsub[i];
        }
    }

    void ShowCallback() {
        for (int i = 0; i < vsize; i++) {
            if (Chost[i] > 0 && Cdevice[i] == 0) ;
            else if (Cdevice[i] > 0 && Chost[i] == 0) ;
            else std::cout << "cdev" <<Cdevice[i] << " chos" << Chost[i] << std::endl;

            Chost[i] += Cdevice[i];
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
