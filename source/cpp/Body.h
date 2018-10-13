
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
using namespace tbb;


/*****************************************************************************
 * class Body
 * **************************************************************************/
class Body{
    // TODO: Fill this class with logic and methods
private:
    int dataToRun;
public:

	void OperatorGPU(int begin, int end, cl_event *event) {

//		cout << "Inside Operator GPU: " << begin  << ", " << end << endl;

		// Associate the input and output buffers with the // kernel
		// using clSetKernelArg()
		error  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_bodies);
		error |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_tree);
		error |= clSetKernelArg(kernel, 2, sizeof(cl_uint), &step);
		error |= clSetKernelArg(kernel, 3, sizeof(cl_float), &epssq);
		error |= clSetKernelArg(kernel, 4, sizeof(cl_float), &dthf);
		error |= clSetKernelArg(kernel, 5, sizeof(cl_uint), &begin);
		error |= clSetKernelArg(kernel, 6, sizeof(cl_uint), &end);

		if (error != CL_SUCCESS) {
			fprintf(stderr, "Failed setting kernel Args!\n");
			exit(0);
		}

		// Define an index space (global work size) of work
		// items for
		// execution. A workgroup size (local work size) is not // required,
		// but can be used.
		size_t globalWorkSize[1];
		size_t localWorkSize[1];

		// There are �elements� work-items
		globalWorkSize[0] = (end-begin);
		//localWorkSize[0] = NULL;

		////////////////////////////////////////////////////
		// STEP 11: Enqueue the kernel for execution
		////////////////////////////////////////////////////
		// Execute the kernel by using
		// clEnqueueNDRangeKernel().
		// �globalWorkSize� is the 1D dimension of the // work-items
		error = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, globalWorkSize, NULL, 0, NULL, event);
		if (error != CL_SUCCESS) {
			fprintf(stderr, "Failed launching kernel!\n");
			fprintf(stderr, "Begin %d end %d\n", begin, end);
			exit(0);
		}
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
			cout << i << endl;
		}
	}

};
//end class

#endif
