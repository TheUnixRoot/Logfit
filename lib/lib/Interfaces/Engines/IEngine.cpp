//
// Created by juanp on 11/12/18.
//

#ifndef BARNESLOGFIT_IENGINE_H
#define BARNESLOGFIT_IENGINE_H
#include <vector>
#include "../../Helpers/ConsoleUtils.h"
class IEngine {
protected:
    std::vector<unsigned int> gpuChunkSizes;
    IEngine() : gpuChunkSizes() {
        gpuChunkSizes.reserve(100000);
    }
public:

    virtual unsigned int getGPUChunk(unsigned int begin, unsigned int end) = 0;

    virtual unsigned int getCPUChunk(unsigned int begin, unsigned int end) = 0;

    virtual void recordGPUTh(unsigned int chunk, float time) = 0;

    virtual void recordCPUTh(unsigned int chunk, float time) = 0;

    virtual void reStart() = 0;

    void printGPUChunks() {
        std::cout << CONSOLE_YELLOW << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++"
                  << CONSOLE_WHITE << std::endl;
        for (unsigned int i : gpuChunkSizes) {
            std::cout << CONSOLE_YELLOW << "GPU Chunk: " << i << std::endl;
        }
        std::cout << CONSOLE_WHITE << std::endl;
    }

    virtual ~IEngine() {}
};

#endif //BARNESLOGFIT_IENGINE_H
