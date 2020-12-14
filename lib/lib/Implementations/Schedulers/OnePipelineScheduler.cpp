//
// Created by juanp on 4/11/20.
//

#include "../../../include/scheduler/OnePipelineScheduler.h"
#include <tbb/parallel_for.h>
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
        tbb::parallel_pipeline(parameters.numcpus + parameters.numgpus,
           tbb::make_filter<void, Bundle*>(tbb::filter::mode::serial,
               [&](tbb::flow_control& fc)->Bundle* {
                   Bundle *bundle = new Bundle();
                   if (begin < end) {
                       //Checking whether the GPU is idle or not.
                       if (--gpuStatus >= 0) {
                           // pedimos un trozo para la GPU entre begin y end
                           int ckgpu = this->getTypedEngine()->getGPUChunk(begin, end);
                           if (ckgpu > 0) {    // si el trozo es > 0 se genera un token de GPU
                               int auxEnd = begin + ckgpu;
                               auxEnd = (auxEnd > end) ? end : auxEnd;
                               bundle->begin = begin;
                               bundle->end = auxEnd;
                               begin = auxEnd;
                               bundle->type = GPU;
                               return bundle;
                           } else { // paramos la GPU si el trozo es <= 0, generamos token CPU
                               // no incrementamos gpuStatus para dejar pillada la GPU
                               int auxEnd = begin + this->getTypedEngine()->getCPUChunk(begin, end);
                               auxEnd = (auxEnd > end) ? end : auxEnd;
                               bundle->begin = begin;
                               bundle->end = auxEnd;
                               begin = auxEnd;
                               bundle->type = CPU;
                               return bundle;
                           }
                       } else {
                           //CPU WORK
                           gpuStatus++;
                           int auxEnd = begin + this->getTypedEngine()->getCPUChunk(begin, end);
                           auxEnd = (auxEnd > end) ? end : auxEnd;
                           bundle->begin = begin;
                           bundle->end = auxEnd;
                           begin = auxEnd;
                           bundle->type = CPU;

                           return bundle;
                       }
                   }
                   fc.stop();
                   return NULL;
               })
           &
           tbb::make_filter<Bundle*, void>(tbb::filter::mode::serial,
               [&](Bundle* bundle)-> void {
                   if (bundle->type == GPU) {
                       // GPU WORK
                       executeOnGPU(bundle);
                       gpuStatus++;
                   } else {
                       // CPU WORK
                       executeOnCPU(bundle);
                   }
                   delete bundle;
               })
        );
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
template<typename TSchedulerEngine, typename TExecutionBody>
void OnePipelineScheduler<TSchedulerEngine, TExecutionBody>::
        executeOnGPU(Bundle *bundle) {
    static bool firstmeasurement = true;
    this->setStartGPU(tbb::tick_count::now());

    this->getTypedBody()->OperatorGPU(bundle->begin, bundle->end);

    this->setStopGPU(tbb::tick_count::now());
    float time = (this->getStopGPU() - this->getStartGPU()).seconds() * 1000;

    this->getTypedEngine()->recordGPUTh((bundle->end - bundle->begin), time);

}
template<typename TSchedulerEngine, typename TExecutionBody>
void OnePipelineScheduler<TSchedulerEngine, TExecutionBody>::
        executeOnCPU(Bundle *bundle) {
    this->setStartCPU(tbb::tick_count::now());
    this->getTypedBody()->OperatorCPU(bundle->begin, bundle->end);
    this->setStopCPU(tbb::tick_count::now());
    float time = (this->getStopCPU() - this->getStartCPU()).seconds() * 1000;

    this->getTypedEngine()->recordCPUTh((bundle->end - bundle->begin), time);
}