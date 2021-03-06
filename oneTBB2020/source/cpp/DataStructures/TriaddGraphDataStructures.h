//
// Created by juanp on 4/22/18.
//

#ifndef BARNESLOGFIT_DATASTRUCTURES_H
#define BARNESLOGFIT_DATASTRUCTURES_H

#include <utils/Utils.h>
#include <vector>
#include <tbb/flow_graph_opencl_node.h>

using t_index = struct _t_index {
    int begin, end;
};
using buffer_f = tbb::flow::opencl_buffer<cl_float>;
using dim_range = std::vector<size_t>;

using type_gpu_node = tbb::flow::tuple<t_index, buffer_f, buffer_f, buffer_f>;

#endif //BARNESLOGFIT_DATASTRUCTURES_H
