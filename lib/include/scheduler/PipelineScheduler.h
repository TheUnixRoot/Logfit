//
// Created by juanp on 21/10/20.
//

#ifndef BARNESLOGFIT_PIPELINESCHEDULER_H
#define BARNESLOGFIT_PIPELINESCHEDULER_H
#if defined(__APPLE__)
#include <OpenCL/cl.h>
#else

#include <CL/cl.h>

#endif
#include "tbb/pipeline.h"
#include <atomic>
#include "../../lib/Interfaces/Schedulers/IScheduler.cpp"
#include "../utils/Utils.h"
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

template<typename TSchedulerEngine, typename TExecutionBody,
        typename ...TArgs>
class PipelineScheduler : public IScheduler {
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
    PipelineScheduler(Params p, TExecutionBody &body, TSchedulerEngine &engine) ;

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

    ~PipelineScheduler() {}

private:

    void initializeOPENCL(char *filename, char *kernelName) ;

    char *ReadSources(char *fileName) ;

    void createCommandQueue() ;

    void CreateAndCompileProgram(char *filename, char *kernelName) ;

    void initializeHOSTPRI() ;
};

#include "../../lib/Implementations/Schedulers/PipelineScheduler.cpp"
#endif //BARNESLOGFIT_PIPELINESCHEDULER_H
