//
// Created by juanp on 2/11/20.
//

#ifndef HETEROGENEOUS_PARALLEL_FOR_ONEAPISCHEDULER_H
#define HETEROGENEOUS_PARALLEL_FOR_ONEAPISCHEDULER_H
#include "../../lib/Interfaces/Schedulers/IScheduler.cpp"
#include "../engine/EngineFactory.h"

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

#include "../../lib/Implementations/Schedulers/OneApiScheduler.cpp"
#endif //HETEROGENEOUS_PARALLEL_FOR_ONEAPISCHEDULER_H
