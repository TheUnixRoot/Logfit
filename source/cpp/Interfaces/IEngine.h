//
// Created by juanp on 11/12/18.
//

#ifndef BARNESLOGFIT_ISCHEDULER_H
#define BARNESLOGFIT_ISCHEDULER_H
class IEngine {
public:
    virtual ~IEngine() = default;

    virtual int GetGPUChunk(int startFrom, int to) = 0;

    virtual int GetCPUChunk(int startFrom, int to) = 0;

    virtual void SetStartCpu(tick_count t) = 0;

    virtual void SetStartGpu(tick_count t) = 0;

    virtual tick_count GetStartCpu() = 0;

    virtual tick_count GetStartGpu() = 0;

    virtual void SetStopCpu(tick_count t) = 0;

    virtual void SetStopGpu(tick_count t) = 0;

    virtual tick_count GetStopCpu() = 0;

    virtual tick_count GetStopGpu() = 0;
};

#endif //BARNESLOGFIT_ISCHEDULER_H
