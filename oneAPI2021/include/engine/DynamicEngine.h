//
// Created by juanp on 26/10/20.
//

#ifndef HETEROGENEOUS_PARALLEL_FOR_DYNAMICENGINE_H
#define HETEROGENEOUS_PARALLEL_FOR_DYNAMICENGINE_H

#include "../../lib/Interfaces/Engines/IEngine.cpp"

class DynamicEngine : public IEngine {
    unsigned int numCPUs, numGPUs, fG, chunkGPU, chunkCPU;
    float cpuThroughput, gpuThroughput;
public:
    DynamicEngine(unsigned int cpus, unsigned int gpus, unsigned int gpuchunk);

    unsigned int getGPUChunk(unsigned int begin, unsigned int end);

    unsigned int getCPUChunk(unsigned int begin, unsigned int end);

    void recordCPUTh(unsigned int chunk, float time);

    void recordGPUTh(unsigned int chunk, float time);

    // inicializacion para un TS nuevo
    void reStart();

    ~DynamicEngine();
};

#endif //HETEROGENEOUS_PARALLEL_FOR_DYNAMICENGINE_H
