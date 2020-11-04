//
// Created by juanp on 2/11/20.
//

#ifndef BARNESLOGFIT_TESTONEAPIBODY_H
#define BARNESLOGFIT_TESTONEAPIBODY_H
#include <vector>
#include <algorithm>
#include "body/IOneApiBody.h"
#include <CL/sycl.hpp>


class TestOneApiBody : public IOneApiBody {
private:
    size_t vsize;
    float *A, *B, *C;
    cl::sycl::queue gpu_queue;
public:

    TestOneApiBody(size_t vsize = 10000000) : vsize{vsize} {
        using namespace cl::sycl;
        gpu_queue = sycl::queue(cl::sycl::cpu_selector{});
        std::cout << gpu_queue.get_device().get_info<info::device::name>() << std::endl;
        auto context = gpu_queue.get_context();
        auto device = gpu_queue.get_device();
        A = (float*) malloc_shared(vsize * sizeof(float), device, context);
        B = (float*) malloc_shared(vsize * sizeof(float), device, context);
        C = (float*) malloc_shared(vsize * sizeof(float), device, context);
        std::generate(A, A + vsize, [] { return 1.0; });
        std::generate(B, B + vsize, [] { return 1.0; });
        std::generate(C, C + vsize, [] { return 0.0; });
    }

    void OperatorCPU(int begin, int end) {
        for (int i = begin; i < end; i++) {
            C[i] = A[i] + B[i];
        }
    }

    void ShowCallback() {
        for (int i = 0; i < vsize; i++) {
            std::cout << i << ": " << A[i] << " + " << B[i] << " = " << C[i] << std::endl;
        }
    }

    void OperatorGPU(int begin, int end) {
        using namespace cl::sycl;
        gpu_queue.submit([&](handler& handler){
            handler.parallel_for(range<1>{vsize}, [=](id<1> id){
                auto idx = id[0];
                C[idx] = A[idx] + B[idx];
            });
        });
        gpu_queue.wait();
    }

    int GetVsize(){
        return vsize;
    }

    ~TestOneApiBody() {
        auto context = gpu_queue.get_context();
        free(A, context);
        free(B, context);
        free(C, context);
    }

};
#endif //BARNESLOGFIT_TESTONEAPIBODY_H
