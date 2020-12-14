//
// Created by juanp on 8/7/19.
//

#ifndef _BODY_TASK_
#define _BODY_TASK_
#include <vector>
#include <algorithm>
#include <CL/sycl.hpp>
#include <body/IOneApiBody.h>
#include "../../Interfaces/Bodies/BarnesBody.h"

/*****************************************************************************
 * class Body
 * **************************************************************************/
class BarnesOneApiBody
        : public IOneApiBody,
          public BarnesBody<oct_tree_body *, oct_tree_cell*>{

private:
    cl::sycl::queue gpu_queue;
public:

    BarnesOneApiBody(Params p, unsigned long nbodies) :
        BarnesBody{true, p, nbodies}, gpu_queue{cl::sycl::queue{cl::sycl::gpu_selector{}}} {
        tree = (oct_tree_cell *) cl::sycl::malloc_shared(
                sizeof(oct_tree_cell) * nbodies, gpu_queue);
        ReadInput(p.inputData);
        RecycleTree(nbodies);
    }

    void OperatorCPU(int begin, int end) {
        for (int i = begin; i < end; i++) {
            BarnesBody::ComputeForce(bodies[i], bodies, tree, step, epssq, dthf);
        }
    }

    int GetVsize() {
        return nbodies;
    }

    void OperatorGPU(int begin, int end) {
        using namespace cl::sycl;
        size_t range_size = end - begin;
        auto &device_bodies{bodies};
        auto &device_tree{tree};
        auto &device_step{step};
        auto &device_epssq{epssq};
        auto &device_dthf{dthf};
        gpu_queue.submit([&](handler& handler){
            handler.parallel_for(range<1>{range_size}, id<1>(begin), [=](id<1> idx){
                if (idx >= end) return;
                BarnesOneApiBody::ComputeForce(device_bodies[idx], device_bodies, device_tree, device_step, device_epssq, device_dthf);
            });
        });
        gpu_queue.wait();
    }

    void ShowCallback() {
        for (int i = 0; i < nbodies; i++) { // print result
            Printfloat(bodies[i].posx);
            printf(" ");
            Printfloat(bodies[i].posy);
            printf(" ");
            Printfloat(bodies[i].posz);
            printf("\n");
        }
        fflush(stdout);
    }

protected:
    inline void ReadInput(char *filename) {
        float vx, vy, vz;
        FILE *f;
        int i;
        oct_tree_body *b;

        f = fopen(filename, "r+t");
        if (f == NULL) {
            fprintf(stderr, "file not found: %s\n", filename);
            exit(-1);
        }

        fscanf(f, "%d", &nbodies);
        fscanf(f, "%d", &timesteps);
        fscanf(f, "%f", &dtime);
        fscanf(f, "%f", &eps);
        fscanf(f, "%f", &tol);

        dthf = 0.5 * dtime;
        epssq = eps * eps;
        itolsq = 1.0 / (tol * tol);

        if (bodies == NULL) {
            std::cout << CONSOLE_YELLOW << "configuration: " << nbodies << " bodies, " << timesteps << " time steps, " << nthreads << " threads" << CONSOLE_WHITE << endl;
            bodies = (oct_tree_body *) cl::sycl::malloc_shared(sizeof(oct_tree_body) * nbodies, gpu_queue);
            bodies2 = (oct_tree_body *) cl::sycl::malloc_shared(sizeof(oct_tree_body) * nbodies, gpu_queue);

        }

        for (i = 0; i < nbodies; i++) {
            b = &bodies[i];
            fscanf(f, "%fE", &(b->mass));
            fscanf(f, "%fE", &(b->posx));
            fscanf(f, "%fE", &(b->posy));
            fscanf(f, "%fE", &(b->posz));
            fscanf(f, "%fE", &(b->velx));
            fscanf(f, "%fE", &(b->vely));
            fscanf(f, "%fE", &(b->velz));
            b->accx = 0.0;
            b->accy = 0.0;
            b->accz = 0.0;
        }
        fclose(f);
    }
};

#endif