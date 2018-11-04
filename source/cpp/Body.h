
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
#include "DataStructures.h"

using namespace tbb;

const int NUM_RAND = 256;
int RandomNumber () { return (std::rand()%NUM_RAND); }
/*****************************************************************************
 * class Body
 * **************************************************************************/
class Body{
    // TODO: Fill this class with logic and methods
public:
    const int vsize = 10;
//    buffer_f Adevice;
//    buffer_f Bdevice;
//    buffer_f Cdevice;
    float* Ahost;
    float* Bhost;
    float* Chost;
public:

    Body() : Ahost(dst::malloc_f(vsize)), Bhost(dst::malloc_f(vsize)), Chost(dst::malloc_f(vsize)){

        std::generate(Ahost, Ahost+vsize, RandomNumber);
        std::generate(Bhost, Bhost+vsize, RandomNumber);
    }

	void OperatorGPU(tbb::flow::opencl_node<type_gpu> *gpuNode, t_index indexes) {

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

};
//end class

#endif
