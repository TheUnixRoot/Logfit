//
// Created by juanp on 8/7/19.
//

#ifndef _BODY_TASK_
#define _BODY_TASK_

#if defined(__APPLE__)
#include <OpenCL/cl.h>
#else

#include <CL/cl.h>

#endif

#include <body/IGraphBody.h>
#include "../../DataStructures/BarnesDataStructures.h"

using namespace tbb;
using namespace BarnesHutDataStructures;

/*****************************************************************************
 * class Body
 * **************************************************************************/
class BarnesBody
        : public IGraphBody<dim_range, t_index, buffer_OctTreeLeafNode, buffer_OctTreeInternalNode, int, float, float> {
public:
    bool firsttime;
    dim_range globalWorkSize;
public:
    BarnesBody(size_t nbodies) : globalWorkSize{nbodies}, firsttime{true} {}

    void OperatorCPU(int begin, int end) {
        auto size{end-begin};
        for (int i = begin; i < end; i++) {
            ComputeForce(&bodies[i], bodies, tree, step, epssq, dthf);
        }
    }

    int GetVsize() {
        return nbodies;
    }

    dim_range GetNDRange() {
        return globalWorkSize;
    }

    type_gpu_node GetGPUArgs(t_index indexes) {
        globalWorkSize[0] = (indexes.end - indexes.begin);
        return std::make_tuple(indexes,/*bodies_gpuidx,*/ d_bodies, d_tree, step, epssq, dthf);
    }

    void ShowCallback() {
        for (int i = 0; i < nbodies; i++) { // print result
            Printfloat(bodies[i].posx);
            printf(" ");
            Printfloat(bodies[i].posy);
            printf(" ");
            Printfloat(bodies[i].posz);
            printf("\n");
        }
        fflush(stdout);
    }
};
//end class

#endif