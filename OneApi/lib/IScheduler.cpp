//
// Created by juanp on 17/10/20.
//

#ifndef BARNESLOGFIT_ISCHEDULER_H
#define BARNESLOGFIT_ISCHEDULER_H

#include "tbb/tick_count.h"
#include "../../Helpers/ConsoleUtils.cpp"

using namespace tbb;

class IScheduler {
protected:
    Params parameters;
    tick_count startBench, stopBench, startCpu, stopCpu, startGpu, stopGpu;
    double runtime;

    IScheduler(Params p) : parameters{p} {}

public:

    virtual void StartParallelExecution() = 0;

    /*Sets the start mark of energy and time*/
    void startTimeAndEnergy() {
#ifdef ENERGYCOUNTERS
        pcm->getAllCounterStates(sstate1, sktstate1, cstates1);
#endif
        startBench = tick_count::now();
    }

    /*Sets the end mark of energy and time*/
    void endTimeAndEnergy() {
        stopBench = tick_count::now();
#ifdef ENERGYCOUNTERS
        pcm->getAllCounterStates(sstate2, sktstate2, cstates2);
#endif
        runtime = (stopBench - startBench).seconds() * 1000;
    }

    /*this function print info to a Log file*/

    void saveResultsForBench() {
        ConsoleUtils::saveResultsForBench(parameters, runtime);
    }

    virtual void *getEngine() = 0;

    virtual void *getBody() = 0;

    virtual ~IScheduler() {}
};

#endif //BARNESLOGFIT_ISCHEDULER_H
