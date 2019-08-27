
#ifndef _BODY_TASK_
#define _BODY_TASK_

#ifdef PJTRACER
	#include "pfortrace.h"
#endif

#if defined(__APPLE__)
	#include <OpenCL/cl.h>
#else
	#include <CL/cl.h>
#endif

#ifdef USEBARRIER
	#include "barrier.h"
#endif

#include "tbb/parallel_for.h"
#include "tbb/task.h"
#include "tbb/tick_count.h"
using namespace tbb;
using namespace BarnesHutDataStructures;

/*****************************************************************************
 * class Body
 * **************************************************************************/
class Body{
public:
	bool firsttime;
public:

	void sendObjectToGPU( int begin, int end, cl_event *event) {

		//tick_count t0 = tick_count::now();
		//ojo en lugar de copiar nbodies elementos se pueden copiar solo aquellos usados
		if(firsttime){
			firsttime=false;

			error = clEnqueueWriteBuffer(command_queue, d_tree, CL_BLOCKING /*blocking*/, 0, sizeof(OctTreeInternalNode) * nbodies, tree.data(), 0, NULL, NULL);
			if (error != CL_SUCCESS) {
				fprintf(stderr, "Fail copying d_tree to device!\n");
				exit(0);
			}

			error = clEnqueueWriteBuffer(command_queue, d_bodies, CL_BLOCKING/*blocking*/ /*No blocking*/, 0, sizeof(OctTreeLeafNode) * nbodies, bodies.data(), 0, NULL, NULL);
			if (error != CL_SUCCESS) {
				fprintf(stderr, "Fail copying  d_bodies to device!\n");
				exit(0);
			}
		}
		//tick_count t1 = tick_count::now();
		//memory_time += (t1 - t0).seconds() * 1000; // in order to use milliseconds
	}

	void OperatorGPU(int begin, int end, cl_event *event) {

		//cerr << "Inside Operator GPU: " << begin  << ", " << end << endl;

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
			ComputeForce(&bodies[i], bodies.data(), tree.data(), step, epssq, dthf);
		}
	}

	void AllocateMemoryObjects() {
	d_tree = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
			sizeof(OctTreeInternalNode) * nbodies, NULL, &error);
	if (error != CL_SUCCESS) {
		fprintf(stderr,
				"Allocation of tree structure into device has failed!\n");
		exit(0);
	}

	d_bodies = clCreateBuffer(context,
			CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
			sizeof(OctTreeLeafNode) * nbodies, NULL, &error);
	if (error != CL_SUCCESS) {
		fprintf(stderr,
				"Allocation of bodies structure into device has failed!\n");
		exit(0);
	}
}

};
//end class

/*****************************************************************************
 * Parallel For
 * **************************************************************************/

class ParallelFor {
	Body *body;
public:
	ParallelFor(Body *b){
		body = b;
	}

    void operator()( const blocked_range<int>& range ) const {

		#ifdef PJTRACER
		tracer->cpuStart();
		#endif

		body->OperatorCPU(range.begin(), range.end());

		#ifdef PJTRACER
		tracer->cpuStop();
		#endif
    }
};

// Note: Reads input[0..n] and writes output[1..n-1].
void ParallelFORCPUs(size_t start, size_t end, Body *body ) {
    ParallelFor pf(body);
	parallel_for( blocked_range<int>( start, end ), pf);
}

/*****************************************************************************
 * Raw Tasks
 * **************************************************************************/
/*
template <class T>
class GPUTask: public task {
public:
   T *body;
   int begin;
   int end;
   GPUTask<T>(T *b, int beg, int e ) : body(b), begin(beg), end(e){}

    // Overrides virtual function task::execute
    task* execute() {
		#ifdef USEBARRIER
			//cerr << "GPU antes de barrier1" << endl;
			getLockGPU();
			//cerr << "GPU despues de barrier1" << endl;
		#endif
		#ifdef PJTRACER
		tracer->gpuStart();
		#endif

		tick_count start = tick_count::now();
		body->sendObjectToGPU(begin, end, NULL);
		body->OperatorGPU(begin, end, NULL);
		body->getBackObjectFromGPU(begin, end, NULL);
		clFinish(command_queue);
		tick_count stop = tick_count::now();
		gpuThroughput = (end -  begin) / ((stop-start).seconds()*1000);

		#ifdef PJTRACER
		tracer->gpuStop();
		#endif
		#ifdef USEBARRIER
			//cerr << "GPU antes de barrier2" << endl;
			freeLockGPU();
			//cerr << "GPU despues de barrier2" << endl;
			getLockCPU();
			freeLockCPU();
		#endif
        return NULL;
    }
};

template <class T>
class CPUTask: public task {
public:
   T *body;
   int begin;
   int end;
   CPUTask<T>(T *b, int beg, int e ) : body(b), begin(beg), end(e){}

    // Overrides virtual function task::execute
    task* execute() {
		#ifdef USEBARRIER
			//cerr << "CPU antes de barrier1" << endl;
			getLockCPU();
			//cerr << "CPU despues de barrier1" << endl;
		#endif

		tick_count start = tick_count::now();
		ParallelFORCPUs(begin, end, body);
		tick_count stop = tick_count::now();
		cpuThroughput = (end -  begin) / ((stop-start).seconds()*1000);

		#ifdef USEBARRIER
			//cerr << "CPU antes de barrier2" << endl;
			freeLockCPU();
			//cerr << "CPU despues de barrier2" << endl;
			getLockGPU();
			freeLockGPU();
		#endif
        return NULL;
    }
};


template <class T>
class ROOTTask : public task{

	T *body;
	int begin;
	int end;

public:

	ROOTTask<T>(T *b, int beg, int e){
		body = b;
		begin = beg;
		end = e;
	}

	task* execute() {
        #ifdef USEBARRIER
			//cerr << "antes de barrier_init" << endl;
			barrier_init();
			//cerr << "despues de barrier_init" << endl;
		#endif
		GPUTask<T>& a = *new( allocate_child() ) GPUTask<T>(body, begin, begin + chunkGPU);
		CPUTask<T>& b = *new( allocate_child() ) CPUTask<T>(body, begin + chunkGPU, end);

		// Set ref_count to 'two children plus one for the wait".
        set_ref_count(3);
        // Start b running.
        spawn( b );
        // Start a running and wait for all children (a and b).
        spawn_and_wait_for_all(a);
		return NULL;
    }
};
*/
#endif
