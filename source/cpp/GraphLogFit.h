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

    GraphLogFit(Params p, flow::graph *g, T *body) :
            bufferNode(*g),
            cpuNode(*g, flow::unlimited, [body](flow::tuple<int, int> data){
                tick_count start = tick_count::now();
                //body->OperatorCPU(flow::get<0>(data), flow::get<1>(data));
                tick_count stop = tick_count::now();

                std::cout << start.resolution() << " .. .. " << stop.resolution() << std::endl;
            }),
            gpuNode(*g, flow::opencl_program<>(flow::opencl_program_type::SOURCE, p.openclFile).get_kernel(p.kernelName)),
            dispatcher(*g, flow::unlimited, [this](Bundle data){
                if (data.type == CPU) {
                    cpuNode.try_put(std::make_tuple(data.begin, data.end));
                } else {
                    flow::interface10::input_port<0>(gpuNode).try_put(4);//.try_put(std::make_tuple(data.begin, data.end));
                }
            })
    {
        graph = g;
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


        graph->wait_for_all();
    }

    flow::function_node<flow::tuple<int, int>> cpuNode;
    flow::opencl_node<flow::tuple<int>> gpuNode;

private:

    void sendChunk(Bundle *pBundle) {
        bufferNode.try_put(*pBundle);
        // TODO: Some returning stuff and synchronisation
    }

    flow::graph *graph;
    flow::queue_node<Bundle> bufferNode;
    flow::function_node<Bundle> dispatcher;

};

#endif //BARNESLOGFIT_GRAPHLOGFIT_H
