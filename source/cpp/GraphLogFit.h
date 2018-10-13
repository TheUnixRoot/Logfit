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

        flow::function_node<flow::tuple<int, int>, Token*> cpuNode(graph, flow::unlimited, [&body](flow::tuple<int, int> data) -> Token *{

            tick_count start = tick_count::now();
            body->OperatorCPU(flow::get<0>(data), flow::get<1>(data));
            tick_count stop = tick_count::now();
            return new Token(CPU);
        });

        flow::function_node<t_index, Token*> gpuJoiner(graph, flow::unlimited, [&body](t_index indexes) -> Token *{
            return new Token(GPU);
        });

        flow::opencl_node<type_gpu> gpuNode(
                graph, flow::opencl_program<>(flow::opencl_program_type::SOURCE, p.openclFile).get_kernel(p.kernelName));

        flow::queue_node<Token*> bufferNode(graph);

        flow::function_node<Token*> dispatcher(graph, flow::unlimited, [&cpuNode, &gpuNode, &body](Token* data){
                    // TODO: size partition

                    if (data->type == CPU) {
                        cpuNode.try_put(make_tuple(0, 5));
                    } else {
                        t_index indexes = {5, 10};
                        tick_count start = tick_count::now();
                        body->OperatorGPU(&gpuNode, indexes);
                        // TODO: Waiting stuff

                        tick_count stop = tick_count::now();
                    }
                    delete data;
                });

        std::array<unsigned int, 1> range{1};
        gpuNode.set_range(range);

        flow::make_edge(bufferNode, dispatcher);
//        flow::make_edge(cpuNode, bufferNode);
        flow::make_edge(flow::output_port<0>(gpuNode), gpuJoiner);
//        flow::make_edge(gpuJoiner, bufferNode);

        bufferNode.try_put(new Token(CPU));
        bufferNode.try_put(new Token(GPU));



        graph.wait_for_all();

        for (int i = 0; i < 10; i++) {
            cout << i << ": " << body->Ahost[i] << " + " << body->Bhost[i] << " = " << body->Chost[i] << endl;
        }

    }
};

#endif //BARNESLOGFIT_GRAPHLOGFIT_H
