//
// Created by juanp on 7/11/20.
//

#ifndef BARNESLOGFIT_TRIADDPIPELINEBODY_H
#define BARNESLOGFIT_TRIADDPIPELINEBODY_H


#if defined(__APPLE__)
#include <OpenCL/cl.h>
#else

#include <CL/cl.h>

#endif

#include <body/IPipelineBody.h>
#include <scheduler/PipelineScheduler.h>
#include "../../DataStructures/TriaddPipelineDataStructures.h"

using namespace tbb;

const int NUM_RAND = 256;

inline int RandomNumber() { return (std::rand() % NUM_RAND); }

/*****************************************************************************
 * class Body
 * **************************************************************************/
class TriaddPipelineBody : public IPipelineBody<t_index, buffer_f, buffer_f, buffer_f> {
public:
    size_t vsize;
    buffer_f Ahost;
    buffer_f Bhost;
    buffer_f Chost;
public:

    TriaddPipelineBody(std::size_t vsize = 10000000) : vsize{vsize},
    Ahost(vsize), Bhost(vsize), Chost(vsize){

        std::generate(Ahost.begin(), Ahost.begin() + vsize, RandomNumber);
        std::generate(Bhost.begin(), Bhost.begin() + vsize, RandomNumber);
        std::generate(Chost.begin(), Chost.begin() + vsize, RandomNumber);
    }

    int GetVsize() {
        return vsize;
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

    void sendObjectToGPU( int begin, int end, cl_event *event) {

        //tick_count t0 = tick_count::now();
        //ojo en lugar de copiar nbodies elementos se pueden copiar solo aquellos usados
        if(firsttime){
            firsttime=false;

            error = clEnqueueWriteBuffer(command_queue, Adevice, CL_BLOCKING /*blocking*/, 0, sizeof(buffer_f) * vsize, Ahost.data(), 0, NULL, NULL);
            if (error != CL_SUCCESS) {
                fprintf(stderr, "Fail copying A to device!\n");
                exit(0);
            }
            error = clEnqueueWriteBuffer(command_queue, Bdevice, CL_BLOCKING /*blocking*/, 0, sizeof(buffer_f) * vsize, Bhost.data(), 0, NULL, NULL);
            if (error != CL_SUCCESS) {
                fprintf(stderr, "Fail copying B to device!\n");
                exit(0);
            }
            error = clEnqueueWriteBuffer(command_queue, Cdevice, CL_BLOCKING /*blocking*/, 0, sizeof(buffer_f) * vsize, Chost.data(), 0, NULL, NULL);
            if (error != CL_SUCCESS) {
                fprintf(stderr, "Fail copying C to device!\n");
                exit(0);
            }
        }
        //tick_count t1 = tick_count::now();
        //memory_time += (t1 - t0).seconds() * 1000; // in order to use milliseconds
    }

    void OperatorGPU(int begin, int end, cl_event *event) {
        t_index indexes{begin, end};
        //cerr << "Inside Operator GPU: " << begin  << ", " << end << endl;

        // Associate the input and output buffers with the // kernel
        // using clSetKernelArg()
        error  = clSetKernelArg(kernel, 0, sizeof(t_index), &indexes);
        error |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &Adevice);
        error |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &Bdevice);
        error |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &Cdevice);

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
        globalWorkSize[0] = (indexes.end-indexes.begin);
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
            fprintf(stderr, "Begin %d end %d\n", indexes.begin, indexes.end);
            exit(0);
        }
    }

    void getBackObjectFromGPU(int begin, int end, cl_event *event) {

        //tick_count t0 = tick_count::now();

        //fprintf(stderr, "GetBack: %d-%d\n",begin,end);
        error = clEnqueueReadBuffer(command_queue, Adevice, CL_NON_BLOCKING, begin * sizeof(buffer_f),
                                    sizeof(buffer_f) * (end-begin), &Ahost[begin], 0, NULL, NULL);

        if (error != CL_SUCCESS) {
            fprintf(stderr, "Failed copying  Adevice to host!\n");
            exit(0);
        }
        error = clEnqueueReadBuffer(command_queue, Bdevice, CL_NON_BLOCKING, begin * sizeof(buffer_f),
                                    sizeof(buffer_f) * (end-begin), &Bhost[begin], 0, NULL, NULL);

        if (error != CL_SUCCESS) {
            fprintf(stderr, "Failed copying  Bdevice to host!\n");
            exit(0);
        }
        error = clEnqueueReadBuffer(command_queue, Cdevice, CL_NON_BLOCKING, begin * sizeof(buffer_f),
                                    sizeof(buffer_f) * (end-begin), &Chost[begin], 0, NULL, NULL);

        if (error != CL_SUCCESS) {
            fprintf(stderr, "Failed copying  Cdevice to host!\n");
            exit(0);
        }
        /*if(error == CL_SUCCESS){
            fprintf(stderr, "GetBack success\n");
        }*/

        //tick_count t1 = tick_count::now();
        //memory_time += (t1 - t0).seconds() * 1000; // in order to use milliseconds
    }

    void AllocateMemoryObjects() {
        Adevice = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                sizeof(buffer_f) * vsize, NULL, &error);
        if (error != CL_SUCCESS) {
            fprintf(stderr,
                    "Allocation of A into device has failed!\n");
            exit(0);
        }
        Bdevice = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                 sizeof(buffer_f) * vsize, NULL, &error);
        if (error != CL_SUCCESS) {
            fprintf(stderr,
                    "Allocation of B into device has failed!\n");
            exit(0);
        }
        Cdevice = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                                 sizeof(buffer_f) * vsize, NULL, &error);
        if (error != CL_SUCCESS) {
            fprintf(stderr,
                    "Allocation of C into device has failed!\n");
            exit(0);
        }
    }



    friend class TriaddPipelineBodyTest;
};
//end class

#endif //BARNESLOGFIT_TRIADDPIPELINEBODY_H
