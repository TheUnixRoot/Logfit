//
// Created by juanp on 6/4/18.
//

#ifndef BARNESLOGFIT_GRAPHLOGFIT_H
#define BARNESLOGFIT_GRAPHLOGFIT_H

#include "../DataStructures/ProvidedDataStructures.h"
#include "tbb/tick_count.h"

using namespace tbb;

template<typename TBody, typename Tgpu>
class GraphLogFit {
private:
    flow::graph graph;
    tick_count startCpu, stopCpu, startGpu, stopGpu;
public:

    GraphLogFit(Params p, TBody *body) {
        flow::function_node<t_index, Type> cpuNode(graph, flow::unlimited, [&body, this](t_index indexes) -> Type {

            startCpu = tick_count::now();
            body->OperatorCPU(indexes.begin, indexes.end);
            stopCpu = tick_count::now();

            return CPU;
        });

        flow::opencl_node<Tgpu> gpuNode(graph,
                flow::opencl_program<>(flow::opencl_program_type::SOURCE, p.openclFile).get_kernel(p.kernelName));

        flow::function_node<t_index, Type> gpuJoiner(graph, flow::unlimited, [&body, this](t_index) -> Type {
            stopGpu = tick_count::now();
            return GPU;
        });

        flow::function_node<Type> dispatcher(graph, flow::serial, [&cpuNode, &gpuNode, &body, this](Type token) {
            // TODO: size partition

            if (token == CPU) {
                t_index indexes = {0, 5};
                cpuNode.try_put(indexes);
            } else {
                t_index indexes = {5, 10};
                startGpu = tick_count::now();


                flow::input_port<0>(gpuNode).try_put(indexes);
                auto args = std::make_tuple(1, 2, 3);
                dataStructures::try_put<Tgpu>(args, &gpuNode);

                std::cout << "\033[0;33m" << "GPU computing from: " << indexes.begin << " to: " << indexes.end
                          << "\033[0m" << std::endl;
                // TODO: Waiting stuff - Token should work

            }
        });

        std::array<unsigned int, 1> range{1};
        gpuNode.set_range(range);

//        flow::make_edge(cpuNode, dispatcher);
//        flow::make_edge(gpuJoiner, dispatcher);
        flow::make_edge(flow::output_port<0>(gpuNode), gpuJoiner);
        dispatcher.try_put(CPU);
        dispatcher.try_put(GPU);

        graph.wait_for_all();
        std::cout << "CPU differential time: " << (stopCpu - startCpu).seconds() << std::endl;
        std::cout << "GPU differential time: " << (stopGpu - startGpu).seconds() << std::endl;
        body->ShowCallback();
        body->~Body();
    }
};

#endif //BARNESLOGFIT_GRAPHLOGFIT_H
