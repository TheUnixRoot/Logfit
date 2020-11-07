//
// Created by juanp on 8/7/19.
//

#ifndef BARNESLOGFIT_DYNAMICENGINE_CPP
#define BARNESLOGFIT_DYNAMICENGINE_CPP
#define DEEP_GPU_REPORT
//#define DEEP_CPU_REPORT
#define GPU_LAST  //ultimo trozo entero para GPU, si no, ultimo trozo para CPUs guided

#include "../../../include/engine/DynamicEngine.h"
#include "tbb/parallel_pipeline.h"
#include "tbb/tick_count.h"
#include <math.h>

#ifdef ENERGYCOUNTERS
#ifdef Win32
#include "PCM_Win/windriver.h"
#else
#include "cpucounters.h"
#endif
#define EPROFTSTEP  // mide energia para cada time step
#endif


using namespace std;
using namespace tbb;


/*Oracle Class: This scheduler version let us to split the workload in two subranges, one for GPU and one for CPUs*/
DynamicEngine::DynamicEngine(unsigned int cpus, unsigned int gpus, unsigned int gpuchunk) :
        numCPUs{cpus}, numGPUs{gpus}, fG{5}, chunkGPU{gpuchunk},
        chunkCPU{chunkGPU / fG}, cpuThroughput{0}, gpuThroughput{0} {}

unsigned int DynamicEngine::getGPUChunk(unsigned int begin, unsigned int end) {
    return chunkGPU;
}

unsigned int DynamicEngine::getCPUChunk(unsigned int begin, unsigned int end) {
    return chunkCPU;
}

void DynamicEngine::recordCPUTh(unsigned int chunk, float time) {
    cpuThroughput = (float) chunk / time;

    /*If GPU has already computed some chunk, then we update fG (factor GPU)*/
    if (gpuThroughput > 0) {
        fG = gpuThroughput / cpuThroughput;
    }
}

void DynamicEngine::recordGPUTh(unsigned int chunk, float time) {
    gpuThroughput = (float) chunk / time;

    /*If CPU has already computed some chunk, then we update fG (factor GPU)*/
    if (cpuThroughput > 0) {
        fG = gpuThroughput / cpuThroughput;
    }
}

// inicializacion para un TS nuevo
void DynamicEngine::reStart() {
    fG = 5;
    chunkCPU = chunkGPU / fG;
    cpuThroughput = 0;
    gpuThroughput = 0;
}

DynamicEngine::~DynamicEngine() {

}

#endif //BARNESLOGFIT_DYNAMICENGINE_CPP
