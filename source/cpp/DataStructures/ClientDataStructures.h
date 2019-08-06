//
// Created by juanp on 4/22/18.
//

#ifndef BARNESLOGFIT_DATASTRUCTURES_H
#define BARNESLOGFIT_DATASTRUCTURES_H

#include "ProvidedDataStructures.h"

using buffer_f = tbb::flow::opencl_buffer<cl_float>;
using dim_range = std::array<unsigned int, 1>;

using t_index = struct _t_index{
    int begin, end;
};
using type_gpu_node = tbb::flow::tuple<t_index, buffer_f, buffer_f, buffer_f>;



#endif //BARNESLOGFIT_DATASTRUCTURES_H
