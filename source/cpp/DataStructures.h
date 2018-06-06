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

enum Type : int { GPU = 0, CPU = 1 };

/*Bundle class: This class is used to store the information that items need while walking throught pipeline's stages.*/
class Bundle {
public:
    int begin;
    int end;
    Type type;

    Bundle() {
    };
};
using type_0 = cl_int;
//using type_1 = cl_int;
//using type_2 = cl_int;
//using type_3 = cl_int;
//using type_4 = cl_int;
//using type_5 = cl_int;
//using type_6 = cl_int;
//using type_7 = cl_int;
//using type_8 = cl_int;



#endif //BARNESLOGFIT_DATASTRUCTURES_H
