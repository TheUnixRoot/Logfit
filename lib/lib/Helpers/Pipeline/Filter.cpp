//
// Created by juanp on 26/10/20.
//

#ifndef HETEROGENEOUS_PARALLEL_FOR_FILTER_CPP
#define HETEROGENEOUS_PARALLEL_FOR_FILTER_CPP

#include "../../Interfaces/Schedulers/IScheduler.cpp"
#include "tbb/pipeline.h"
#include "tbb/tick_count.h"

using namespace PipelineDatastructures;

template<typename TScheduler>
class ParallelFilter : public tbb::filter {
private:
    int begin, end;
    TScheduler *scheduler;
public:
    ParallelFilter(int begin, int end, TScheduler *scheduler) :
            filter(false), begin{begin}, end{end}, scheduler{scheduler} {}

    void *operator()(void *item) {
        //variables
        Bundle *bundle = (Bundle *) item;

        if (bundle->type == GPU) {
            // GPU WORK
            executeOnGPU(bundle);
            gpuStatus++;
        } else {
            // CPU WORK
            executeOnCPU(bundle);
        }
        delete bundle;
        return NULL;
    }

    void executeOnGPU(Bundle *bundle) {
        static bool firstmeasurement = true;
        scheduler->startGpu = tbb::tick_count::now();

        scheduler->getBody()->sendObjectToGPU(bundle->begin, bundle->end, NULL);
        scheduler->getBody()->OperatorGPU(bundle->begin, bundle->end, NULL);
        scheduler->getBody()->getBackObjectFromGPU(bundle->begin, bundle->end, NULL);
        clFinish(scheduler->command_queue);

        scheduler->stopGpu = tbb::tick_count::now();
        float time = (scheduler->stopGpu - scheduler->startGpu).seconds() * 1000;

        scheduler->engine->recordGPUTh((bundle->end - bundle->begin), time);

    }

    void executeOnCPU(Bundle *bundle) {
        scheduler->startGpu = tbb::tick_count::now();
        scheduler->getBody()->OperatorCPU(bundle->begin, bundle->end);
        scheduler->stopGpu = tbb::tick_count::now();
        float time = (scheduler->stopGpu - scheduler->startGpu).seconds() * 1000;

        scheduler->getEngine()->recordCPUTh((bundle->end - bundle->begin), time);
    }
};

template<typename TScheduler>
class SerialFilter : public tbb::filter {
private:
    int begin, end;
    TScheduler *scheduler;
public:
    SerialFilter(int begin, int end, TScheduler *scheduler) :
            filter(false), begin{begin}, end{end}, scheduler{scheduler} {}

    void *operator()(void *) {
        Bundle *bundle = new Bundle();
        if (begin < end) {
            //Checking whether the GPU is idle or not.
            if (--gpuStatus >= 0) {
                // pedimos un trozo para la GPU entre begin y end
                int ckgpu = scheduler->getEngine()->getGPUChunk(begin, end);
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
                    int auxEnd = begin + scheduler->getEngine()->getCPUChunk(begin, end);
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
                int auxEnd = begin + scheduler->getEngine()->getCPUChunk(begin, end);
                auxEnd = (auxEnd > end) ? end : auxEnd;
                bundle->begin = begin;
                bundle->end = auxEnd;
                begin = auxEnd;
                bundle->type = CPU;

                return bundle;
            }
        }
        return NULL;
    }
};


#endif //HETEROGENEOUS_PARALLEL_FOR_FILTER_CPP
