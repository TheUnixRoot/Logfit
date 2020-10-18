//
// Created by juanp on 11/12/18.
//

#ifndef BARNESLOGFIT_IENGINE_H
#define BARNESLOGFIT_IENGINE_H
class IEngine {
public:

    virtual unsigned int getGPUChunk(unsigned int begin, unsigned int end) = 0;

    virtual unsigned int getCPUChunk(unsigned int begin, unsigned int end) = 0;

    virtual void recordGPUTh(unsigned int chunk, float time) = 0;

    virtual void recordCPUTh(unsigned int chunk, float time) = 0;

    virtual void reStart() = 0;
};

#endif //BARNESLOGFIT_IENGINE_H
