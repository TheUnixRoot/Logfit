//
// Created by juanp on 6/4/18.
//

#ifndef BARNESLOGFIT_GRAPHLOGFIT_H
#define BARNESLOGFIT_GRAPHLOGFIT_H
#define TBB_PREVIEW_FLOW_GRAPH_NODES 1
#define TBB_PREVIEW_FLOW_GRAPH_FEATURES 1

#include <tbb/flow_graph_opencl_node.h>

#include "../../lib/Interfaces/Schedulers/IScheduler.cpp"
#include "../engine/EngineFactory.h"

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
};
using namespace tbb;

template<typename TSchedulerEngine, typename TExecutionBody,
        typename t_index, typename ...TArgs>
class GraphScheduler : public IScheduler {
private:
    TExecutionBody &body;
    TSchedulerEngine &engine;
    dataStructures::GpuDeviceSelector gpuSelector;
    flow::graph graph;
    int begin, end, cpuCounter;
    flow::function_node<t_index, ProcessorUnit> cpuNode;
    flow::opencl_node<tbb::flow::tuple<t_index, TArgs ...>> gpuNode;
    flow::function_node<t_index, ProcessorUnit> gpuCallbackReceiver;
    flow::function_node<ProcessorUnit> processorSelectorNode;

public:
    GraphScheduler(Params p, TExecutionBody &body, TSchedulerEngine &engine) ;

    void StartParallelExecution() ;

    void *getEngine() ;

    void *getBody() ;

    ~GraphScheduler() {}
};
#include "../../lib/Implementations/Schedulers/GraphScheduler.cpp"
#endif //BARNESLOGFIT_GRAPHLOGFIT_H
