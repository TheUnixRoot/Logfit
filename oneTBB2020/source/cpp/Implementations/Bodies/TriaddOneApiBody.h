//
// Created by juanp on 2/11/20.
//

#ifndef BARNESLOGFIT_TRIADDONEAPIBODY_H
#define BARNESLOGFIT_TRIADDONEAPIBODY_H
#include <vector>
#include <algorithm>
#include <CL/sycl.hpp>
#include <body/IOneApiBody.h>

const int NUM_RAND = 256;

inline int RandomNumber() { return (std::rand() % NUM_RAND); }
class TriaddOneApiBody : public IOneApiBody {
private:
    size_t vsize;
    float *A, *B, *C;
    cl::sycl::queue gpu_queue;
public:

    TriaddOneApiBody(size_t vsize = 10000000) : vsize{vsize} {
        using namespace cl::sycl;
        gpu_queue = sycl::queue(cl::sycl::host_selector{});
        std::cout << gpu_queue.get_device().get_info<info::device::name>() << std::endl;
        auto context = gpu_queue.get_context();
        auto device = gpu_queue.get_device();
        A = (float*) malloc_host(vsize * sizeof(float), gpu_queue);
        B = (float*) malloc_host(vsize * sizeof(float), gpu_queue);
        C = (float*) malloc_host(vsize * sizeof(float), gpu_queue);
        std::generate(A, A + vsize, RandomNumber);
        std::generate(B, B + vsize, RandomNumber);
        std::generate(C, C + vsize, RandomNumber);
    }

    void OperatorCPU(int begin, int end) {
        for (int i = begin; i < end; i++) {
            C[i] = A[i] + B[i];
        }
    }

    void OperatorGPU(int begin, int end) {
        using namespace cl::sycl;
        size_t range_size = end - begin;
        gpu_queue.submit([&](handler& handler){
            handler.parallel_for(range<1>{range_size}, [=](id<1> id){
                auto idx = id[0] + begin;
                if (idx >= end) return;
                C[idx] = A[idx] + B[idx];
            });
        });
        gpu_queue.wait();
    }

    void ShowCallback() {
        for (int i = 0; i < vsize; i++) {
            std::cout << i << ": " << A[i] << " + " << B[i] << " = " << C[i] << std::endl;
        }
    }

    int GetVsize(){
        return vsize;
    }

    ~TriaddOneApiBody() {
        sycl::free(A, gpu_queue);
        sycl::free(B, gpu_queue);
        sycl::free(C, gpu_queue);
    }

    friend class TriaddOneApiBodyTest;
};
#endif //BARNESLOGFIT_TRIADDONEAPIBODY_H
