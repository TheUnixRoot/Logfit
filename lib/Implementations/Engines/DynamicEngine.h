//
// Created by juanp on 8/7/19.
//

#ifndef BARNESLOGFIT_DYNAMIC_H
#define BARNESLOGFIT_DYNAMIC_H
#define DEEP_GPU_REPORT
//#define DEEP_CPU_REPORT
#define GPU_LAST  //ultimo trozo entero para GPU, si no, ultimo trozo para CPUs guided

#include <cstdlib>
#include <iostream>
#include <fstream>
#include "../../DataStructures/ProvidedDataStructures.h"
#include "../../Interfaces/Engines/IEngine.h"
#include "tbb/pipeline.h"
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
class DynamicEngine : public IEngine {
    unsigned int numCPUs, numGPUs, fG, chunkGPU, chunkCPU;
    float cpuThroughput, gpuThroughput;
public:
    DynamicEngine(unsigned int cpus, unsigned int gpus, unsigned int gpuchunk) :
            numCPUs{cpus}, numGPUs{gpus}, fG{5}, chunkGPU{gpuchunk},
            chunkCPU{chunkGPU/fG}, cpuThroughput{0}, gpuThroughput{0} { }

    unsigned int getGPUChunk(unsigned int begin, unsigned int end) {
        return chunkGPU;
    }

    unsigned int getCPUChunk(unsigned int begin, unsigned int end) {
        return chunkCPU;
    }

    void recordCPUTh(unsigned int chunk, float time) {
        cpuThroughput = (float)chunk / time;

        /*If GPU has already computed some chunk, then we update fG (factor GPU)*/
        if (gpuThroughput > 0){
            fG = gpuThroughput/cpuThroughput;
        }
    }

    void recordGPUTh(unsigned int chunk, float time) {
        gpuThroughput = (float)chunk / time;

        /*If CPU has already computed some chunk, then we update fG (factor GPU)*/
        if (cpuThroughput > 0){
            fG = gpuThroughput/cpuThroughput;
        }
    }

    // inicializacion para un TS nuevo
    void reStart() {
        fG = 5;
        chunkCPU = chunkGPU/fG;
        cpuThroughput = 0;
        gpuThroughput = 0;
    }

    ~DynamicEngine() {

    }
};
#endif //BARNESLOGFIT_DYNAMIC_H
