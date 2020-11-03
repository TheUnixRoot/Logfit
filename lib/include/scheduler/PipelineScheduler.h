//
// Created by juanp on 21/10/20.
//

#ifndef BARNESLOGFIT_PIPELINESCHEDULER_H
#define BARNESLOGFIT_PIPELINESCHEDULER_H

#include "tbb/parallel_pipeline.h"
#include "tbb/parallel_for.h"
#include "../../lib/Interfaces/Schedulers/IScheduler.cpp"
#include "../../lib/Helpers/Pipeline/Filter.cpp"


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

template<typename TSchedulerEngine, typename TExecutionBody,
        typename ...TArgs>
class PipelineScheduler : public IScheduler {
private:
    TSchedulerEngine &engine;
    TExecutionBody &body;
    cl_command_queue command_queue;

public:
    PipelineScheduler(Params p, TExecutionBody &body, TSchedulerEngine &engine) ;

    void StartParallelExecution() ;

    void *getEngine() ;

    void *getBody() ;

    ~PipelineScheduler() {}

private:

    void initializeOPENCL(char *kernelName) ;

    char *ReadSources(char *fileName) ;

    void createCommandQueue() ;

    void CreateAndCompileProgram(char *kname) ;

    void initializeHOSTPRI() ;
};

#include "../../lib/Implementations/Schedulers/PipelineScheduler.cpp"
#endif //BARNESLOGFIT_PIPELINESCHEDULER_H
