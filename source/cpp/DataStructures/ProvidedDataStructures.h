//
// Created by juanp on 11/8/18.
//

#ifndef BARNESLOGFIT_PROVIDEDDATASTRUCTURES_H
#define BARNESLOGFIT_PROVIDEDDATASTRUCTURES_H

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


enum Type : int {
    GPU = 0, CPU = 1
};
namespace dataStructures {
    namespace {
        std::vector<tbb::flow::opencl_buffer<cl_float> *> dataPtrs;
        std::mutex dataMutex;
    }

    float *malloc_f(int vsize) {
        dataMutex.lock();
        auto vector_float = new tbb::flow::opencl_buffer<cl_float>(vsize);
        dataPtrs.push_back(vector_float);
        dataMutex.unlock();
        return vector_float->data();
    }

    void delete_f(float *vector) {
        dataMutex.lock();
        int i = 0;
        while (vector != dataPtrs[i]->data() && i < dataPtrs.size()) {
            i++;
        }
        if (i == dataPtrs.size()) {
            throw "Cannot delete non managed float pointers.";
        }
        delete (dataPtrs[i]);
        dataPtrs.erase(dataPtrs.begin() + i);
        dataMutex.unlock();
    }

    template<typename Tgpu,std::size_t I = 1, typename... Tp>
    inline typename std::enable_if<I == sizeof...(Tp), void>::type
    try_put(std::tuple<Tp...> &t, tbb::flow::opencl_node<Tgpu> *node) {
        tbb::flow::interface10::input_port<I>(*node).try_put(*dataPtrs[I - 1]);
    }

    template<typename Tgpu, std::size_t I = 1, typename... Tp>
    inline typename std::enable_if<I < sizeof...(Tp), void>::type
    try_put(std::tuple<Tp...> &t, tbb::flow::opencl_node<Tgpu> *node) {
        tbb::flow::interface10::input_port<I>(*node).try_put(*dataPtrs[I - 1]);
        try_put<Tgpu, I + 1, Tp...>(t, node);
    }

}

#endif //BARNESLOGFIT_PROVIDEDDATASTRUCTURES_H