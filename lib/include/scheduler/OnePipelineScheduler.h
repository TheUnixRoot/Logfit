//
// Created by juanp on 4/11/20.
//

#ifndef HETEROGENEOUS_PARALLEL_FOR_ONEPIPELINESCHEDULER_H
#define HETEROGENEOUS_PARALLEL_FOR_ONEPIPELINESCHEDULER_H

#include "tbb/parallel_pipeline.h"
#include "tbb/parallel_for.h"
#include "../../lib/Interfaces/Schedulers/IScheduler.cpp"
#include "../../lib/Helpers/Pipeline/OneFilter.cpp"
using namespace OnePipelineDataStructures;

template<typename TSchedulerEngine, typename TExecutionBody>
class OnePipelineScheduler : public IScheduler {
private:
    TSchedulerEngine &engine;
    TExecutionBody &body;
public:
    OnePipelineScheduler(Params p, TExecutionBody &body, TSchedulerEngine &engine) ;

    void StartParallelExecution() ;

    void *getEngine() ;

    void *getBody() ;

    tbb::tick_count getStartGPU() ;

    tbb::tick_count getStopGPU() ;

    tbb::tick_count getStartCPU() ;

    tbb::tick_count getStopCPU() ;

    void setStartGPU(tbb::tick_count val) ;

    void setStopGPU(tbb::tick_count val) ;

    void setStartCPU(tbb::tick_count val) ;

    void setStopCPU(tbb::tick_count val) ;

    TSchedulerEngine* getTypedEngine() ;

    TExecutionBody* getTypedBody() ;

    ~OnePipelineScheduler() {}
};

#include "../../lib/Implementations/Schedulers/OnePipelineScheduler.cpp"
#endif //HETEROGENEOUS_PARALLEL_FOR_ONEPIPELINESCHEDULER_H

