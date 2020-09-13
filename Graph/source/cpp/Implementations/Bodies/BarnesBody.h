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

#include "../../Interfaces/Bodies/IBody.h"
#include "../../DataStructures/BarnesDataStructures.h"

using namespace tbb;
using namespace BarnesHutDataStructures;

/*****************************************************************************
 * class Body
 * **************************************************************************/
class BarnesBody : IBody<dim_range, t_index, buffer_OctTreeLeafNode, buffer_OctTreeInternalNode, int, float, float>{
public:
    bool firsttime;
    dim_range globalWorkSize;
public:
    BarnesBody(size_t nbodies) : globalWorkSize{nbodies}, firsttime{true} { }

    void OperatorCPU(int begin, int end) {
        for (int i = begin; i < end; i++) {
            ComputeForce(&bodies[i], bodies.data(), tree, step, epssq, dthf);
        }
    }

    int GetVsize() {
        return nbodies;
    }

    dim_range GetNDRange() {
        return globalWorkSize;
    }

    type_gpu_node GetGPUArgs(t_index indexes) {
        globalWorkSize[0] = (indexes.end-indexes.begin);
        return std::make_tuple(indexes, bodies, d_tree, step, epssq, dthf);
    }
};
//end class

#endif