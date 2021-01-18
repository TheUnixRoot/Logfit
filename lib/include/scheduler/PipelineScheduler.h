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
#include "tbb/parallel_for.h"
#include <atomic>
#include "../../lib/Interfaces/Schedulers/IScheduler.cpp"
#include "../../lib/Helpers/Pipeline/Filter.cpp"
#include "../utils/Utils.h"

using namespace PipelineDataStructures;

template<typename TSchedulerEngine, typename TExecutionBody,
        typename ...TArgs>
class PipelineScheduler : public IScheduler {
private:
    TSchedulerEngine &engine;
    TExecutionBody &body;
public:
    std::atomic<int> gpuStatus;
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
