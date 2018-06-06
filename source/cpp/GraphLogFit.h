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

    GraphLogFit(Params p, T *body) :
            graph(),
            bufferNode(graph),
            cpuNode(graph, flow::unlimited, [body](flow::tuple<int, int> data){
                tick_count start = tick_count::now();
                //body->OperatorCPU(flow::get<0>(data), flow::get<1>(data));
                tick_count stop = tick_count::now();

                std::cout << start.resolution() << " .. .. " << stop.resolution() << std::endl;
            }),
            gpuNode(graph,
                    flow::opencl_program<>(flow::opencl_program_type::SOURCE, p.openclFile).get_kernel(p.kernelName)),
            dispatcher(graph, flow::unlimited, [this](Bundle data){
                // TODO: size partition


                if (data.type == CPU) {
                    cpuNode.try_put(std::make_tuple(data.begin, data.end));
                } else {
                    // Array 0--N con el size del buffer correspondiente
                    switch (0) {
//                        case 8:
//                            flow::interface10::input_port<8>(gpuNode).try_put(4);
//                        case 7:
//                            flow::interface10::input_port<7>(gpuNode).try_put(4);
//                        case 6:
//                            flow::interface10::input_port<6>(gpuNode).try_put(4);
//                        case 5:
//                            flow::interface10::input_port<5>(gpuNode).try_put(4);
//                        case 4:
//                            flow::interface10::input_port<4>(gpuNode).try_put(4);
//                        case 3:
//                            flow::interface10::input_port<3>(gpuNode).try_put(4);
//                        case 2:
//                            flow::interface10::input_port<2>(gpuNode).try_put(4);
//                        case 1:
//                            flow::interface10::input_port<1>(gpuNode).try_put(4);
                        case 0:
                            flow::interface10::input_port<0>(gpuNode).try_put(4);
                            break;
                        default:
                            std::cerr << "Number of arguments is greater than 10";
                            std::exit(1);
                    }
                    flow::interface10::input_port<0>(gpuNode).try_put(4);//.try_put(std::make_tuple(data.begin, data.end));

                }
            })
    {
        std::array<unsigned int, 1> range{1};
        gpuNode.set_range(range);

        flow::make_edge(bufferNode, dispatcher);

    }

    void heterogeneous_parallel_for() {
        // TODO: Implementation of bundle creation logic


        Bundle *bundle = new Bundle();
        bundle->type = CPU;
        sendChunk(bundle);

        bundle->type = GPU;
        sendChunk(bundle);

        graph.wait_for_all();
    }

    flow::function_node<flow::tuple<int, int>> cpuNode;
    flow::opencl_node<flow::tuple<type_0>> gpuNode;

private:

    void sendChunk(Bundle *pBundle) {
        bufferNode.try_put(*pBundle);
        // TODO: Some returning stuff and synchronisation
    }

    flow::graph graph;
    flow::queue_node<Bundle> bufferNode;
    flow::function_node<Bundle> dispatcher;

};

#endif //BARNESLOGFIT_GRAPHLOGFIT_H
