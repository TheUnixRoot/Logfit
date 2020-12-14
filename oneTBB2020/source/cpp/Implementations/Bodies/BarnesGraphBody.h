//
// Created by juanp on 11/12/20.
//

#ifndef LOGFIT_BARNESGRAPHBODY_H
#define LOGFIT_BARNESGRAPHBODY_H

#if defined(__APPLE__)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#include <body/IGraphBody.h>
#include "../../DataStructures/BarnesGraphDataStructures.h"

/*****************************************************************************
 * class Body
 * **************************************************************************/
class BarnesGraphBody
        : public IGraphBody<dim_range, t_index, body_buffer, tree_buffer, int, float, float>,
          public BarnesBody<body_buffer, tree_buffer> {
private:
    dim_range globalWorkSize;
public:
    BarnesGraphBody(Params p, unsigned long nbodies) :
            BarnesBody{true, p, nbodies} , globalWorkSize{nbodies} {
        bodies = body_buffer{nbodies};
        bodies2 = body_buffer {nbodies};
        tree = tree_buffer{nbodies};
        ReadInput(p.inputData);
        RecycleTree(nbodies);
    }

    void OperatorCPU(int begin, int end) {
        auto size{end - begin};
        auto subbodies{bodies.subbuffer(begin, size)};
        auto subtree{tree.subbuffer(begin, size)};
        for (int i = 0; i < size; i++) {
            BarnesBody<oct_tree_body *, oct_tree_cell *>::ComputeForce(subbodies[i], subbodies.data(), subtree.data(), step, epssq, dthf);
        }
    }

    int GetVsize() {
        return nbodies;
    }

    dim_range GetNDRange() {
        return globalWorkSize;
    }

    type_gpu_node GetGPUArgs(t_index indexes) {
        globalWorkSize[0] = (indexes.end - indexes.begin);
        return std::make_tuple(indexes, bodies, tree, step, epssq, dthf);
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
        FILE *f;
        int i;
        if ((f = fopen(filename, "r+t")) == NULL) {
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

        std::cout << CONSOLE_YELLOW << "configuration: " << nbodies << " bodies, " << timesteps << " time steps, " << nthreads << " threads" << CONSOLE_WHITE << endl;


        for (i = 0; i < nbodies; i++) {
            oct_tree_body &b = bodies[i];
            fscanf(f, "%fE", &(b.mass));
            fscanf(f, "%fE", &(b.posx));
            fscanf(f, "%fE", &(b.posy));
            fscanf(f, "%fE", &(b.posz));
            fscanf(f, "%fE", &(b.velx));
            fscanf(f, "%fE", &(b.vely));
            fscanf(f, "%fE", &(b.velz));
            b.accx = 0.0;
            b.accy = 0.0;
            b.accz = 0.0;
        }
        fclose(f);
    }
};
#endif //LOGFIT_BARNESGRAPHBODY_H
