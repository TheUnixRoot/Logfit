//
// Created by juanp on 4/11/20.
//

#ifndef HETEROGENEOUS_PARALLEL_FOR_ONEPIPELINESCHEDULER_H
#define HETEROGENEOUS_PARALLEL_FOR_ONEPIPELINESCHEDULER_H

#include "tbb/pipeline.h"
#include <atomic>
#include "../../lib/Interfaces/Schedulers/IScheduler.cpp"

template<typename TSchedulerEngine, typename TExecutionBody>
class OnePipelineScheduler : public IScheduler {
private:
    TSchedulerEngine &engine;
    TExecutionBody &body;
    std::atomic<int> gpuStatus;
    class Bundle {
    public:
        int begin;
        int end;
        ProcessorUnit type; //GPU = 0, CPU=1
        Bundle() { };
    };
    void executeOnGPU(Bundle *bundle) ;

    void executeOnCPU(Bundle *bundle) ;

    void ParallelFilter(int begin, int end, Bundle *bundle) ;

    Bundle *SerialFilter(int begin, int end) ;
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
