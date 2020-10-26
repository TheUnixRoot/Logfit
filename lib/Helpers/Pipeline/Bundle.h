//
// Created by juanp on 21/10/20.
//

#ifndef BARNESLOGFIT_BUNDLE_H
#define BARNESLOGFIT_BUNDLE_H
#include "tbb/atomic.h"
#include "../../DataStructures/ProvidedDataStructures.h"

namespace PipelineDatastructures {
    tbb::atomic<int> gpuStatus;

    class Bundle {
    public:
        int begin;
        int end;
        ProcessorUnit type; //GPU = 0, CPU=1

        Bundle() {
        };
    };

}
#endif //BARNESLOGFIT_BUNDLE_H
