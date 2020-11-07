/*****************************************************************************
 * Data Structure
 * **************************************************************************/

typedef struct {
    int begin, end;
} t_index;

/*****************************************************************************
* Kernels
* **************************************************************************/

__kernel void sample(t_index indexes, __global float *Adevice, __global float *Bdevice, __global float *Cdevice
) {
    int idx = get_global_id(0) + indexes.begin;
    if (idx >= indexes.end) return;
//    printf("\nidx: %d\n", idx);
    Cdevice[idx] = Adevice[idx] + Bdevice[idx];
}
