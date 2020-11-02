//
// Created by juanp on 6/4/18.
//

#ifndef BARNESLOGFIT_GRAPHLOGFIT_H
#define BARNESLOGFIT_GRAPHLOGFIT_H

#include "../../lib/Interfaces/Schedulers/IScheduler.cpp"
#include "../../lib/DataStructures/ProvidedDataStructures.h"
using namespace tbb;

template<typename TSchedulerEngine, typename TExecutionBody,
        typename t_index, typename ...TArgs>
class GraphScheduler : public IScheduler {
private:
    TExecutionBody &body;
    TSchedulerEngine &engine;

    flow::graph graph;
    int begin, end, cpuCounter;
    flow::function_node<t_index, ProcessorUnit> cpuNode;
    dataStructures::GpuDeviceSelector gpuSelector;
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
//#include "../../lib/Implementations/Schedulers/GraphScheduler.cpp"
#endif //BARNESLOGFIT_GRAPHLOGFIT_H
