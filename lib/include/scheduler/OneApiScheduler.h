//
// Created by juanp on 2/11/20.
//

#ifndef HETEROGENEOUS_PARALLEL_FOR_ONEAPISCHEDULER_H
#define HETEROGENEOUS_PARALLEL_FOR_ONEAPISCHEDULER_H

#include <tbb/flow_graph.h>
#include "../../lib/Interfaces/Schedulers/IScheduler.cpp"
#include "../engine/EngineFactory.h"
using execution_range = struct {
    int begin, end;
};
template<typename TSchedulerEngine, typename TExecutionBody>
class OneApiScheduler : public IScheduler {
private:
    TSchedulerEngine &engine;
    TExecutionBody &body;

    flow::graph graph;
    int begin, end;
    flow::function_node<execution_range, ProcessorUnit> cpuNode;
    flow::function_node<execution_range, ProcessorUnit> gpuNode;
    flow::function_node<ProcessorUnit> processorSelectorNode;

public:
    OneApiScheduler(Params p, TExecutionBody &body, TSchedulerEngine &engine) ;

    void StartParallelExecution() ;

    void *getEngine() ;

    void *getBody() ;

    ~OneApiScheduler() {}

};

#include "../../lib/Implementations/Schedulers/OneApiScheduler.cpp"
#endif //HETEROGENEOUS_PARALLEL_FOR_ONEAPISCHEDULER_H
