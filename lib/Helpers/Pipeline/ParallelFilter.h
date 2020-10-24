//
// Created by juanp on 21/10/20.
//

#ifndef BARNESLOGFIT_PARALLELFILTER_H
#define BARNESLOGFIT_PARALLELFILTER_H

#include "Bundle.h"
#include <type_traits>
#include "tbb/pipeline.h"
#include "tbb/tick_count.h"

using namespace PipelineDatastructures;
template <typename TScheduler>
class ParallelFilter : public tbb::filter {
private:
    int begin, end;
    TScheduler &scheduler;
public:
    ParallelFilter(int begin, int end, TScheduler &scheduler) :
    filter(false), begin{ begin }, end{ end }, scheduler{ scheduler } { }

    void * operator()(void * item) {
        //variables
        Bundle *bundle = (Bundle*) item;

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
        scheduler.startGpu = tbb::tick_count::now();

        scheduler.body->sendObjectToGPU(bundle->begin, bundle->end, NULL);
        scheduler.body->OperatorGPU(bundle->begin, bundle->end, NULL);
        scheduler.body->getBackObjectFromGPU(bundle->begin, bundle->end, NULL);
        clFinish(scheduler.command_queue);

        scheduler.stopGpu = tbb::tick_count::now();
        float time = (scheduler.stopGpu - scheduler.startGpu).seconds()*1000;

        scheduler.engine.recordGPUTh((bundle->end - bundle->begin), time);

    }

    void executeOnCPU(Bundle *bundle) {
        scheduler.startGpu = tbb::tick_count::now();
        scheduler.body->OperatorCPU(bundle->begin, bundle->end);
        scheduler.stopGpu = tbb::tick_count::now();
        float time = (scheduler.stopGpu - scheduler.startGpu).seconds()*1000;

        scheduler.engine.recordCPUTh((bundle->end - bundle->begin), time);
    }
};


#endif //BARNESLOGFIT_PARALLELFILTER_H
