//
// Created by juanp on 3/11/20.
//
#include "../../../include/utils/Utils.h"
#include <atomic>
namespace PipelineDatastructures {
    std::atomic<int> gpuStatus;

    class Bundle {
    public:
        int begin;
        int end;
        ProcessorUnit type; //GPU = 0, CPU=1

        Bundle() {
        };
    };

}
