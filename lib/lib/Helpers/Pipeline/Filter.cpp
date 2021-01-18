//
// Created by juanp on 26/10/20.
//

#ifndef HETEROGENEOUS_PARALLEL_FOR_FILTER_CPP
#define HETEROGENEOUS_PARALLEL_FOR_FILTER_CPP
#if defined(__APPLE__)
#include <OpenCL/cl.h>
#else

#include <CL/cl.h>

#endif

#include "../../Interfaces/Schedulers/IScheduler.cpp"
#include <tbb/pipeline.h>


cl_int error;
cl_uint num_max_platforms;
cl_uint num_max_devices;
cl_uint num_platforms;
cl_uint num_devices;
cl_platform_id platforms_id;
cl_device_id device_id;
cl_context context;
cl_program program;
cl_kernel kernel;
int computeUnits;
size_t vectorization;
cl_command_queue command_queue;


namespace PipelineDataStructures {

    class Bundle {
    public:
        int begin;
        int end;
        ProcessorUnit type; //GPU = 0, CPU=1

        Bundle() {
        };
    };

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
            if (item == NULL) return NULL;

            Bundle *bundle = (Bundle *) item;

            if (bundle->type == GPU) {
                // GPU WORK
                executeOnGPU(bundle);
                scheduler->gpuStatus++;
            } else {
                // CPU WORK
                executeOnCPU(bundle);
            }
            delete bundle;
            return NULL;
        }

        void executeOnGPU(Bundle *bundle) {
            scheduler->setStartGPU(tbb::tick_count::now());

            scheduler->getTypedBody()->sendObjectToGPU(bundle->begin, bundle->end, NULL);
            scheduler->getTypedBody()->OperatorGPU(bundle->begin, bundle->end, NULL);
            scheduler->getTypedBody()->getBackObjectFromGPU(bundle->begin, bundle->end, NULL);
            clFlush(command_queue);

            scheduler->setStopGPU(tbb::tick_count::now());
            float time = (scheduler->getStopGPU() - scheduler->getStartGPU()).seconds();
            scheduler->getTypedEngine()->recordGPUTh((bundle->end - bundle->begin), time);

        }

        void executeOnCPU(Bundle *bundle) {
            scheduler->setStartCPU(tbb::tick_count::now());
            scheduler->getTypedBody()->OperatorCPU(bundle->begin, bundle->end);
            scheduler->setStopCPU(tbb::tick_count::now());
            float time = (scheduler->getStopCPU() - scheduler->getStartCPU()).seconds();

            scheduler->getTypedEngine()->recordCPUTh((bundle->end - bundle->begin), time);
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
                if (--scheduler->gpuStatus >= 0) {
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
                    scheduler->gpuStatus++;
                    int auxEnd = begin + scheduler->getTypedEngine()->getCPUChunk(begin, end);
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
} // namespace PipelineDataStructures

#endif //HETEROGENEOUS_PARALLEL_FOR_FILTER_CPP
