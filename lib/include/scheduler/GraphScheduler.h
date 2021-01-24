//
// Created by juanp on 6/4/18.
//

#ifndef BARNESLOGFIT_GRAPHLOGFIT_H
#define BARNESLOGFIT_GRAPHLOGFIT_H
#include "../../lib/Helpers/Graph/DataStructures.cpp"
#include "../../lib/Interfaces/Schedulers/IScheduler.cpp"
#include "../engine/EngineFactory.h"

template<typename TSchedulerEngine, typename TExecutionBody,
        typename t_index, typename ...TArgs>
class GraphScheduler : public IScheduler {
private:
    TExecutionBody &body;
    TSchedulerEngine &engine;
    dataStructures::GpuDeviceSelector gpuSelector;
    tbb::flow::graph graph;
    int begin, end, cpuCounter;
    tbb::flow::function_node<t_index, ProcessorUnit> cpuNode;
    tbb::flow::opencl_node<tbb::flow::tuple<t_index, TArgs ...>> gpuNode;
    tbb::flow::function_node<t_index, ProcessorUnit> gpuCallbackReceiver;
    tbb::flow::function_node<ProcessorUnit> processorSelectorNode;

public:
    GraphScheduler(Params p, TExecutionBody &body, TSchedulerEngine &engine) ;

    void StartParallelExecution() ;

    void *getEngine() ;

    TSchedulerEngine *getTypedEngine();

    void *getBody() ;

    ~GraphScheduler() {}
};
#include "../../lib/Implementations/Schedulers/GraphScheduler.cpp"
#endif //BARNESLOGFIT_GRAPHLOGFIT_H
