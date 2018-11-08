
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
class Body : IBody {
public:
    const int vsize = 10;
//    buffer_f Adevice;
//    buffer_f Bdevice;
//    buffer_f Cdevice;
    float *Ahost;
    float *Bhost;
    float *Chost;
public:

    Body() : Ahost(dataStructures::malloc_f(vsize)), Bhost(dataStructures::malloc_f(vsize)),
             Chost(dataStructures::malloc_f(vsize)) {

        std::generate(Ahost, Ahost + vsize, RandomNumber);
        std::generate(Bhost, Bhost + vsize, RandomNumber);
    }

    void OperatorCPU(int begin, int end) {
        for (int i = begin; i < end; i++) {
            Chost[i] = Ahost[i] + Bhost[i];
        }
        std::cout << "\033[0;33m" << "CPU computing from: " << begin << " to: " << end << "\033[0m" << std::endl;
    }

    void ShowCallback() {
        for (int i = 0; i < 10; i++) {
            std::cout << i << ": " << Ahost[i] << " + " << Bhost[i] << " = " << Chost[i] << std::endl;
        }
    }

    ~Body() {
        dataStructures::delete_f(Ahost);
        dataStructures::delete_f(Bhost);
        dataStructures::delete_f(Chost);
    }

};
//end class

#endif
