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
    float *A, *B, *Cdevice, *Chost;
    cl::sycl::queue gpu_queue;
public:

    TriaddOneApiBody(size_t vsize = 10000) : vsize{vsize}, gpu_queue(cl::sycl::gpu_selector{}) {
        using namespace cl::sycl;
//        std::cout << gpu_queue.get_device().get_info<info::device::name>() << std::endl;
        auto context = gpu_queue.get_context();
        auto device = gpu_queue.get_device();
        A = (float*) malloc_shared(vsize * sizeof(float), gpu_queue);
        B = (float*) malloc_shared(vsize * sizeof(float), gpu_queue);
        Cdevice = (float*) malloc_shared(vsize * sizeof(float), gpu_queue);
        Chost = (float*) malloc_shared(vsize * sizeof(float), gpu_queue);

        std::iota(A, A + vsize, 1);
        std::iota(B, B + vsize, 1);
        std::generate(Cdevice, Cdevice + vsize, [](){return 0;});
        std::generate(Chost, Chost + vsize, [](){return 0;});
    }

    void OperatorCPU(int begin, int end) {
        for (int i = begin; i < end; i++) {
            Chost[i] = A[i] + B[i];
        }
    }

    void OperatorGPU(int begin, int end) {
        using namespace cl::sycl;
        size_t range_size = end - begin;
        auto C = this->Cdevice, A = this->A, B = this->B;
        gpu_queue.submit([&](handler& handler){
            handler.parallel_for(range<1>{range_size}, id<1>(begin), [=](id<1> idx){
                if (idx >= end) return;
                C[idx] = A[idx] + B[idx];
            });
        });
        gpu_queue.wait();
    }

    void ShowCallback() {
        for (int i = 0; i < vsize; i++) {
            if (Chost[i] > 0 && Cdevice[i] == 0) ;
            else if (Cdevice[i] > 0 && Chost[i] == 0) ;
            else std::cout << "cdev" <<Cdevice[i] << " chos" << Chost[i] << std::endl;

            Chost[i] += Cdevice[i];
        }
    }

    int GetVsize(){
        return vsize;
    }

    ~TriaddOneApiBody() {
        sycl::free(A, gpu_queue);
        sycl::free(B, gpu_queue);
        sycl::free(Cdevice, gpu_queue);
        sycl::free(Chost, gpu_queue);
    }

    friend class TriaddOneApiBodyTest;
};
#endif //BARNESLOGFIT_TRIADDONEAPIBODY_H
