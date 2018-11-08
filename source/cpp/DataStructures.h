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
struct Token {
public:
    Type type;

    Token(Type t) : type(t) {    };
};


using buffer_f = tbb::flow::opencl_buffer<cl_float>;
using buffer_i = tbb::flow::opencl_buffer<cl_int>;

using buffer = union {
    buffer_f buff_f;
    buffer_i buff_i;
};

using t_index = struct _t_index {
    int begin, end;
};

using type_gpu = tbb::flow::tuple<t_index, buffer_f , buffer_f , buffer_f>;


namespace dst {
    std::vector<tbb::flow::opencl_buffer<cl_float>*> dataPtrs;

    float *malloc_f(int vsize) {

        auto vector_float = new tbb::flow::opencl_buffer<cl_float> (vsize);
        dataPtrs.push_back(vector_float);
        return vector_float->data();
    }
}

template<std::size_t I = 1, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
try_put(std::tuple<Tp...>& t, tbb::flow::opencl_node<type_gpu> *node)
{
    tbb::flow::interface10::input_port<I>(*node).try_put(*dst::dataPtrs[I-1]);
}

template<std::size_t I = 1, typename... Tp>
inline typename std::enable_if<I < sizeof...(Tp), void>::type
try_put(std::tuple<Tp...>& t, tbb::flow::opencl_node<type_gpu> *node)
{
    tbb::flow::interface10::input_port<I>(*node).try_put(*dst::dataPtrs[I-1]);
    try_put<I + 1, Tp...>(t, node);
}


#endif //BARNESLOGFIT_DATASTRUCTURES_H
