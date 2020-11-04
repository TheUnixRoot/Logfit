//
// Created by juanp on 4/11/20.
//

#include "../../../include/scheduler/OnePipelineScheduler.h"

template<typename TSchedulerEngine, typename TExecutionBody>
OnePipelineScheduler<TSchedulerEngine, TExecutionBody>::OnePipelineScheduler(Params p, TExecutionBody &body, TSchedulerEngine &engine) :
        IScheduler(p), body{body}, engine{engine} { }

template<typename TSchedulerEngine, typename TExecutionBody>
void OnePipelineScheduler<TSchedulerEngine, TExecutionBody>::StartParallelExecution() {
    int begin = 0;
    int end = body.GetVsize();
    engine.reStart();

    if (parameters.numgpus < 1) {  // solo CPUs
        tbb::parallel_for(tbb::blocked_range<int>(begin, end),
                          [&](const tbb::blocked_range<int> &range) {
                              startCpu = tbb::tick_count::now();
                              body.OperatorCPU(range.begin(), range.end());
                              stopCpu = tbb::tick_count::now();
                          },
                          tbb::auto_partitioner());
    } else {  // ejecucion con GPU
        tbb::pipeline pipe;
        SerialFilter<OnePipelineScheduler> serial_filter(begin, end, this);
        ParallelFilter<OnePipelineScheduler> parallel_filter(begin, end, this);
        pipe.add_filter(serial_filter);
        pipe.add_filter(parallel_filter);

        pipe.run(parameters.numcpus + parameters.numgpus);
        pipe.clear();
    }
}
template<typename TSchedulerEngine, typename TExecutionBody>
tbb::tick_count OnePipelineScheduler<TSchedulerEngine, TExecutionBody>::getStartGPU() {
    return startGpu;
}
template<typename TSchedulerEngine, typename TExecutionBody>
tbb::tick_count OnePipelineScheduler<TSchedulerEngine, TExecutionBody>::getStopGPU() {
    return stopGpu;
}
template<typename TSchedulerEngine, typename TExecutionBody>
tbb::tick_count OnePipelineScheduler<TSchedulerEngine, TExecutionBody>::getStartCPU() {
    return startCpu;
}
template<typename TSchedulerEngine, typename TExecutionBody>
tbb::tick_count OnePipelineScheduler<TSchedulerEngine, TExecutionBody>::getStopCPU() {
    return stopCpu;
}
template<typename TSchedulerEngine, typename TExecutionBody>
void OnePipelineScheduler<TSchedulerEngine, TExecutionBody>::setStartGPU(tbb::tick_count val) {
    startGpu = val;
}
template<typename TSchedulerEngine, typename TExecutionBody>
void OnePipelineScheduler<TSchedulerEngine, TExecutionBody>::setStopGPU(tbb::tick_count val) {
    stopGpu = val;
}
template<typename TSchedulerEngine, typename TExecutionBody>
void OnePipelineScheduler<TSchedulerEngine, TExecutionBody>::setStartCPU(tbb::tick_count val) {
    startCpu = val;
}
template<typename TSchedulerEngine, typename TExecutionBody>
void OnePipelineScheduler<TSchedulerEngine, TExecutionBody>::setStopCPU(tbb::tick_count val) {
    stopCpu = val;
}

template<typename TSchedulerEngine, typename TExecutionBody>
TSchedulerEngine* OnePipelineScheduler<TSchedulerEngine, TExecutionBody>::getTypedEngine() {
    return (TSchedulerEngine*)&engine;
}

template<typename TSchedulerEngine, typename TExecutionBody>
TExecutionBody* OnePipelineScheduler<TSchedulerEngine, TExecutionBody>::getTypedBody() {
    return (TExecutionBody*)&body;
}

template<typename TSchedulerEngine, typename TExecutionBody>
void* OnePipelineScheduler<TSchedulerEngine, TExecutionBody>::getEngine() {
    return &engine;
}

template<typename TSchedulerEngine, typename TExecutionBody>
void* OnePipelineScheduler<TSchedulerEngine, TExecutionBody>::getBody() {
    return &body;
}