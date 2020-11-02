//
// Created by juanp on 11/8/18.
//

#ifndef BARNESLOGFIT_PROVIDEDDATASTRUCTURES_H
#define BARNESLOGFIT_PROVIDEDDATASTRUCTURES_H
#define TBB_PREVIEW_FLOW_GRAPH_NODES 1
#define TBB_PREVIEW_FLOW_GRAPH_FEATURES 1
#include <tbb/flow_graph_opencl_node.h>
#include <tbb/task_scheduler_init.h>
#include "../../include/utils/Utils.h"


enum ProcessorUnit : int;
namespace dataStructures {

    struct GpuDeviceSelector;

    template<std::size_t I = 0, typename ...Tg>
    inline typename std::enable_if<I == sizeof...(Tg), void>::type
    try_put(tbb::flow::opencl_node<tbb::flow::tuple<Tg...>> *node, tbb::flow::tuple<Tg...> &args);

    template<std::size_t I = 0, typename ...Tg>
    inline typename std::enable_if<I < sizeof...(Tg), void>::type
    try_put(tbb::flow::opencl_node<tbb::flow::tuple<Tg...>> *node, tbb::flow::tuple<Tg...> &args);

}

namespace PipelineDatastructures {
    tbb::atomic<int> gpuStatus;

    class Bundle {
    public:
        int begin;
        int end;
        ProcessorUnit type; //GPU = 0, CPU=1

        Bundle() {
        };
    };

}


enum ProcessorUnit : int {
    GPU = 0, CPU = 1
};
namespace dataStructures {

    struct GpuDeviceSelector {
    private:
        bool firstTime = true;
        tbb::flow::interface11::opencl_device device;
    public:
        template<typename DeviceFilter>
        tbb::flow::opencl_device operator()(tbb::flow::opencl_factory<DeviceFilter> &f) {
            if (this->firstTime) {
                this->firstTime = false;
                auto deviceIterator = std::find_if(f.devices().cbegin(), f.devices().cend(),
                                                   [](const tbb::flow::opencl_device &d) {
                                                       return d.type() == CL_DEVICE_TYPE_GPU;
                                                   }
                );
                this->device = deviceIterator == f.devices().cend() ? *f.devices().cbegin() : *deviceIterator;
            }
            return this->device;
        }
    };


    template<std::size_t I = 0, typename ...Tg>
    inline typename std::enable_if<I == sizeof...(Tg), void>::type
    try_put(tbb::flow::opencl_node<tbb::flow::tuple<Tg...>> *node, tbb::flow::tuple<Tg...> &args) {

    }

    template<std::size_t I = 0, typename ...Tg>
    inline typename std::enable_if<I < sizeof...(Tg), void>::type
    try_put(tbb::flow::opencl_node<tbb::flow::tuple<Tg...>> *node, tbb::flow::tuple<Tg...> &args) {
        size_t i = I;
        auto t = tbb::flow::get<I>(args);
        tbb::flow::interface11::input_port<I>(*node).try_put(
                t
        );
        try_put<I + 1, Tg...>(node, args);
    }

}

#endif //BARNESLOGFIT_PROVIDEDDATASTRUCTURES_H
