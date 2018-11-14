//
// Created by juanp on 11/12/18.
//

#ifndef BARNESLOGFIT_LOGFIT_H
#define BARNESLOGFIT_LOGFIT_H

#include "../Interfaces/IEngine.h"

class LogFitEngine : IEngine {
private:
    tick_count startCpu, stopCpu, startGpu, stopGpu;
public:

    LogFitEngine() { }

    int GetGPUChunk(int startFrom, int to) {
        if (to - startFrom == 1) return 1;
        return (to - startFrom) / 2;
    }

    int GetCPUChunk(int startFrom, int to) {
        if (to - startFrom == 1) return 1;
        return (to - startFrom) / 2;
    }

    virtual void SetStartCpu(tick_count t) {
        startCpu = t;
    }

    virtual void SetStartGpu(tick_count t) {
        startGpu = t;
    }

    virtual tick_count GetStartCpu() {
        return startCpu;
    }

    virtual tick_count GetStartGpu() {
        return startGpu;
    }

    virtual void SetStopCpu(tick_count t) {
        stopCpu = t;
    }

    virtual void SetStopGpu(tick_count t) {
        stopGpu = t;
    }

    virtual tick_count GetStopCpu() {
        return stopCpu;
    }

    virtual tick_count GetStopGpu() {
        return stopGpu;
    }
};

#endif //BARNESLOGFIT_LOGFIT_H
