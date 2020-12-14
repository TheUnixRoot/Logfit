//
// Created by juanp on 8/7/19.
//

#ifndef BARNESLOGFIT_BARNESDATASTRUCTURES_H
#define BARNESLOGFIT_BARNESDATASTRUCTURES_H

#if defined(__APPLE__)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif
#include "../Interfaces/Bodies/BarnesBody.h"

using dim_range = std::vector<size_t>;
using t_index = struct _t_index {
    int begin, end;
};
using body_buffer = tbb::flow::opencl_buffer<oct_tree_body>;
using tree_buffer = tbb::flow::opencl_buffer<oct_tree_cell>;
using type_gpu_node = tbb::flow::tuple<t_index, body_buffer, tree_buffer, int, float, float>;


#endif //BARNESLOGFIT_BARNESDATASTRUCTURES_H
