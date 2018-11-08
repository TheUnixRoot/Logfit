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
private:
    flow::graph graph;
public:

    GraphLogFit(Params p, T *body)
    {
        flow::function_node<flow::tuple<int, int>, Token*> cpuNode(graph, flow::unlimited, [&body](flow::tuple<int, int> data) -> Token *{

            tick_count start = tick_count::now();
            body->OperatorCPU(flow::get<0>(data), flow::get<1>(data));
            tick_count stop = tick_count::now();

            return new Token(CPU);
        });

        flow::opencl_node<type_gpu> gpuNode(
                graph, flow::opencl_program<>(flow::opencl_program_type::SOURCE, p.openclFile).get_kernel(p.kernelName));

        flow::function_node<t_index, Token*> gpuJoiner(graph, flow::unlimited, [&body](t_index indexes) -> Token *{
            tick_count stop = tick_count::now();
            cout << "#DONE" << endl;
            return new Token(GPU);
        });

        flow::function_node<Type> dispatcher(graph, flow::serial, [&cpuNode, &gpuNode, &body](Type token){
                    // TODO: size partition

                    if (token == CPU) {
                        cpuNode.try_put(make_tuple(0, 5));
                    } else {
                        t_index indexes = {5, 10};
                        tick_count start = tick_count::now();



                        flow::input_port<0>(gpuNode).try_put(indexes);
                        auto args = std::make_tuple(1, 2, 3);
                        try_put(args, &gpuNode);

                        std::cout << "\033[0;33m" << "GPU computing from: " << indexes.begin << " to: " << indexes.end << "\033[0m" << std::endl;
                        // TODO: Waiting stuff - Token should work

                    }
                });

        std::array<unsigned int, 1> range{1};
        gpuNode.set_range(range);

//        flow::make_edge(bufferNode, dispatcher);
//        flow::make_edge(cpuNode, bufferNode);
        int k = 0;
//        flow::join_node< tbb::flow::tuple<int,float> > j(graph);
        flow::make_edge(flow::output_port<0>(gpuNode), gpuJoiner);
//        flow::make_edge(gpuJoiner, bufferNode);
        dispatcher.try_put(CPU);
        dispatcher.try_put(GPU);

        graph.wait_for_all();
        body->ShowCallback();
    }
};

#endif //BARNESLOGFIT_GRAPHLOGFIT_H
