//
// Created by juanp on 6/4/18.
//

#ifndef BARNESLOGFIT_GRAPHLOGFIT_H
#define BARNESLOGFIT_GRAPHLOGFIT_H

#include "DataStructures.h"
#include "tbb/tick_count.h"

using namespace tbb;

template <typename T>
class GraphLogFit {
public:

    GraphLogFit(Params p, T *body)
    {
        flow::graph graph;

        flow::function_node<flow::tuple<int, int>> cpuNode(graph, flow::unlimited, [&body](flow::tuple<int, int> data){

            tick_count start = tick_count::now();
            body->OperatorCPU(flow::get<0>(data), flow::get<1>(data));
            tick_count stop = tick_count::now();
        });
        flow::opencl_node<type_gpu> gpuNode(
                graph, flow::opencl_program<>(flow::opencl_program_type::SOURCE, p.openclFile).get_kernel(p.kernelName));
        flow::queue_node<Bundle> bufferNode(graph);
        flow::function_node<Bundle> dispatcher(graph, flow::unlimited, [&cpuNode, &gpuNode, &body](Bundle data){
                    // TODO: size partition

                    if (data.type == CPU) {
                    cpuNode.try_put(make_tuple(data.begin, data.end / 2));
                    } else {
                        t_index indexes = {data.end / 2, data.end};
                        tick_count start = tick_count::now();
                        body->OperatorGPU(&gpuNode, indexes);
                        // TODO: Waiting stuff

                        tick_count stop = tick_count::now();
                    }
                });

        std::array<unsigned int, 1> range{1};
        gpuNode.set_range(range);

        flow::make_edge(bufferNode, dispatcher);



        Bundle *bundle = new Bundle(0, 10, CPU);
        bufferNode.try_put(*bundle);

        bundle->type = GPU;
        bufferNode.try_put(*bundle);

        graph.wait_for_all();

        for (int i = 0; i < 10; i++) {
            cout << i << ": " << body->Ahost[i] << " + " << body->Bhost[i] << " = " << body->Chost[i] << endl;
        }

    }
};

#endif //BARNESLOGFIT_GRAPHLOGFIT_H
