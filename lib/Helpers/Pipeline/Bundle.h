//
// Created by juanp on 21/10/20.
//

#ifndef BARNESLOGFIT_BUNDLE_H
#define BARNESLOGFIT_BUNDLE_H
#include "tbb/atomic.h"
namespace PipelineDatastructures {
    tbb::atomic<int> gpuStatus;

    class Bundle {
    public:
        int begin;
        int end;
        int type; //GPU = 0, CPU=1

        Bundle() {
        };
    };

    const int GPU = 1;
    const int CPU = 0;
}
#endif //BARNESLOGFIT_BUNDLE_H
