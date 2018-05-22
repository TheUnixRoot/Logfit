//
// Created by juanp on 4/22/18.
//

#ifndef BARNESLOGFIT_DATASTRUCTURES_H
#define BARNESLOGFIT_DATASTRUCTURES_H
#define MAX_NUMBER_GPU_SUPPORTED tbb::flow::interface10::opencl_info::available_devices().size()
#define MAX_NUMBER_CPU_SUPPORTED thread::hardware_concurrency()
#define TBB_PREVIEW_FLOW_GRAPH_NODES 1
#define TBB_PREVIEW_FLOW_GRAPH_FEATURES 1
#include <tbb/flow_graph.h>
#include <tbb/flow_graph_opencl_node.h>

typedef struct {
    int numcpus;
    int numgpus;
    char benchName[256];
    char kernelName[50];
    char openclFile[256];
    char inputData[256];
    float ratioG;
} Params;

#endif //BARNESLOGFIT_DATASTRUCTURES_H
