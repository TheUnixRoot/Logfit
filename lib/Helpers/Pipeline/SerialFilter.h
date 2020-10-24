//
// Created by juanp on 21/10/20.
//

#ifndef BARNESLOGFIT_SERIALFILTER_H
#define BARNESLOGFIT_SERIALFILTER_H

#include "Bundle.h"
#include <type_traits>
#include "tbb/pipeline.h"
#include "tbb/tick_count.h"
using namespace PipelineDatastructures;

template <typename TScheduler>
class SerialFilter : public tbb::filter {
private:
    int begin, end;
    TScheduler &scheduler;
public:
    SerialFilter(int begin, int end, TScheduler &scheduler) :
            filter(false), begin{ begin }, end{ end }, scheduler{ scheduler } { }

    void * operator()(void *) {
        Bundle *bundle = new Bundle();
        if (begin < end) {
            //Checking whether the GPU is idle or not.
            if (--gpuStatus >= 0) {
                // pedimos un trozo para la GPU entre begin y end
                int ckgpu = scheduler.engine->getGPUChunk(begin, end);
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
                    int auxEnd = begin + scheduler.engine->getCPUChunk(begin, end);
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
                int auxEnd = begin + scheduler.engine->getCPUChunk(begin, end);
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



#endif //BARNESLOGFIT_SERIALFILTER_H
