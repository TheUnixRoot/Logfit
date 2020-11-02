//
// Created by juanp on 1/11/20.
//

#ifndef HETEROGENEOUS_PARALLEL_FOR_ONEAPISCHEDULER_H
#define HETEROGENEOUS_PARALLEL_FOR_ONEAPISCHEDULER_H

#include "lib/IScheduler.cpp"

using Params = struct _params {
    unsigned int numcpus;
    unsigned int numgpus;
    char benchName[256];
    char kernelName[50];
    char openclFile[256];
    char inputData[256];
    float ratioG;
};

template<typename TSchedulerEngine, typename TExecutionBody,
        typename ...TArgs>
class OneApiScheduler : public IScheduler {
private:
    TSchedulerEngine &engine;
    TExecutionBody &body;


public:
    OneApiScheduler(Params p, TExecutionBody &body, TSchedulerEngine &engine) ;

    void StartParallelExecution() ;

    void *getEngine() ;

    void *getBody() ;

    ~OneApiScheduler() {}

};


#endif //HETEROGENEOUS_PARALLEL_FOR_ONEAPISCHEDULER_H
