//
// Created by juanp on 4/11/20.
//

//
// Created by juanp on 26/10/20.
//

#ifndef HETEROGENEOUS_PARALLEL_FOR_FILTER_CPP
#define HETEROGENEOUS_PARALLEL_FOR_FILTER_CPP

#include "../../Interfaces/Schedulers/IScheduler.cpp"
#include <tbb/parallel_pipeline.h>
#include <tbb/tick_count.h>
#include <atomic>

namespace OnePipelineDataStructures {
    std::atomic<int> gpuStatus;

    class Bundle {
    public:
        int begin;
        int end;
        ProcessorUnit type; //GPU = 0, CPU=1

        Bundle() {
        };
    };

    template<typename TScheduler>
    class ParallelFilter : public tbb::detail::d1::filter<Bundle*, void> {
    private:
        int begin, end;
        TScheduler *scheduler;
    public:
        ParallelFilter(int begin, int end, TScheduler *scheduler) :
                begin{begin}, end{end}, scheduler{scheduler} {}

        void operator()(Bundle* bundle) {
            //variables
            if (bundle == NULL) {
	        return;
	    }
            if (bundle->type == GPU) {
                // GPU WORK
                executeOnGPU(*bundle);
                gpuStatus++;
            } else {
                // CPU WORK
                executeOnCPU(*bundle);
            }
	    delete(bundle);
        }

        void executeOnGPU(Bundle& bundle) {
            static bool firstmeasurement = true;
            scheduler->setStartGPU(tbb::tick_count::now());

            scheduler->getTypedBody()->OperatorGPU(bundle.begin, bundle.end);

            scheduler->setStopGPU(tbb::tick_count::now());
            float time = (scheduler->getStopGPU() - scheduler->getStartGPU()).seconds() * 1000;

            scheduler->getTypedEngine()->recordGPUTh((bundle.end - bundle.begin), time);

        }

        void executeOnCPU(Bundle& bundle) {
            scheduler->setStartCPU(tbb::tick_count::now());
            scheduler->getTypedBody()->OperatorCPU(bundle.begin, bundle.end);
            scheduler->setStopCPU(tbb::tick_count::now());
            float time = (scheduler->getStopCPU() - scheduler->getStartCPU()).seconds() * 1000;

            scheduler->getTypedEngine()->recordCPUTh((bundle.end - bundle.begin), time);
        }
    };

    template<typename TScheduler>
    class SerialFilter : public tbb::detail::d1::filter<void, Bundle*> {
    private:
        int begin, end;
        TScheduler *scheduler;
    public:
        SerialFilter(int begin, int end, TScheduler *scheduler) :
                begin{begin}, end{end}, scheduler{scheduler} {}

        Bundle* operator()(flow_control& fc) {
            Bundle* bundle = new Bundle();
	    if (begin < end) {
                //Checking whether the GPU is idle or not.
                if (--gpuStatus >= 0) {
                    // pedimos un trozo para la GPU entre begin y end
                    int ckgpu = scheduler->getTypedEngine()->getGPUChunk(begin, end);
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
                        int auxEnd = begin + scheduler->getTypedEngine()->getCPUChunk(begin, end);
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
                    int auxEnd = begin + scheduler->getTypedEngine()->getCPUChunk(begin, end);
                    auxEnd = (auxEnd > end) ? end : auxEnd;
                    bundle->begin = begin;
                    bundle->end = auxEnd;
                    begin = auxEnd;
                    bundle->type = CPU;

                    return bundle;
                }
            }
	    fc.stop();
	    delete(bundle);
            return NULL;
        }
    };
} //namespace OnePipelineDataStructures

#endif //HETEROGENEOUS_PARALLEL_FOR_FILTER_CPP

