
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
private:
    const int vsize = 10;
public:
    buffer_f Adevice;
    buffer_f Bdevice;
    buffer_f Cdevice;
    float* Ahost;
    float* Bhost;
    float* Chost;
public:

    Body() : Adevice(vsize), Bdevice(vsize), Cdevice(vsize), Ahost(Adevice.data()),
            Bhost(Bdevice.data()), Chost(Cdevice.data()) {

        std::generate(Ahost, Ahost+vsize, RandomNumber);
        std::generate(Bhost, Bhost+vsize, RandomNumber);
    }

	void OperatorGPU(tbb::flow::opencl_node<type_gpu> *gpuNode, t_index indexes) {
        flow::interface10::input_port<0>(*gpuNode).try_put(indexes);
		flow::interface10::input_port<1>(*gpuNode).try_put(Adevice);
		flow::interface10::input_port<2>(*gpuNode).try_put(Bdevice);
		flow::interface10::input_port<3>(*gpuNode).try_put(Cdevice);
		cout << "\033[0;33m" << "GPU computing from: " << indexes.begin << " to: " << indexes.end << "\033[0m" << endl;
	}

	void getBackObjectFromGPU(int begin, int end, cl_event *event) {

		//tick_count t0 = tick_count::now();

		//fprintf(stderr, "GetBack: %d-%d\n",begin,end);
		error = clEnqueueReadBuffer(command_queue, d_bodies, CL_NON_BLOCKING, begin * sizeof(OctTreeLeafNode),
			sizeof(OctTreeLeafNode) * (end-begin), &bodies[begin], 0, NULL, NULL);

		if (error != CL_SUCCESS) {
			fprintf(stderr, "Failed copying  d_bodies to host!\n");
			exit(0);
		}
		/*if(error == CL_SUCCESS){
			fprintf(stderr, "GetBack success\n");
		}*/

		//tick_count t1 = tick_count::now();
		//memory_time += (t1 - t0).seconds() * 1000; // in order to use milliseconds
	}

	void OperatorCPU(int begin, int end) {
		for (int i = begin; i < end; i++) {
            Chost[i] = Ahost[i] + Bhost[i];
        }
        cout << "\033[0;33m" << "CPU computing from: " << begin << " to: " << end << "\033[0m" << endl;
	}

};
//end class

#endif
