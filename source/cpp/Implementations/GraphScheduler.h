//
// Created by juanp on 6/4/18.
//

#ifndef BARNESLOGFIT_GRAPHLOGFIT_H
#define BARNESLOGFIT_GRAPHLOGFIT_H

#include "../DataStructures/ProvidedDataStructures.h"
#include "tbb/tick_count.h"
#include "../Interfaces/IEngine.h"

using namespace tbb;

template<typename TBody, typename Tgpu, typename TSchedulerEngine, typename std::enable_if<std::is_base_of<IEngine, TSchedulerEngine>::value>::type* = nullptr>
class GraphScheduler {
private:
    std::mutex mtx;
    flow::graph graph;
    int begin, end, cpuCounter;
public:

    GraphScheduler(Params p, TBody *body, TSchedulerEngine engine) : mtx(), begin(0), end(body->GetVsize()), cpuCounter(0) {
        flow::function_node<t_index, Type> cpuNode(graph, flow::unlimited, [&engine, &body, this](t_index indexes) -> Type {

            engine.SetStartCpu(tick_count::now());
            body->OperatorCPU(indexes.begin, indexes.end);
            engine.SetStopCpu(tick_count::now());


            std::cout << "CPU differential time: " << (engine.GetStopCpu() - engine.GetStartCpu()).seconds()<< std::endl
#ifdef NDEBUG
                    << "Counter: " << cpuCounter << std::endl
#endif
                    ;
            return CPU;
        });

        flow::opencl_node<Tgpu> gpuNode(graph,
                                        flow::opencl_program<>(flow::opencl_program_type::SOURCE,
                                                               p.openclFile).get_kernel(p.kernelName));

        flow::function_node<t_index, Type> gpuJoiner(graph, flow::unlimited, [&engine, &body, this](t_index) -> Type {
            engine.SetStopGpu(tick_count::now());
            std::cout << "GPU differential time: " << (engine.GetStopGpu() - engine.GetStartGpu()).seconds() << std::endl;
            return GPU;
        });

        flow::function_node<Type> dispatcher(graph, flow::serial, [&engine, &cpuNode, &gpuNode, &body, this](Type token) {
            // TODO: size partition
            mtx.lock();
            if (begin <= end) {
                //Checking whether the GPU is idle or not.

                if (token == GPU) {
                    int ckgpu = engine.GetGPUChunk(begin, end);
                    if (ckgpu > 0) {    // si el trozo es > 0 se genera un token de GPU
                        int auxEnd = begin + ckgpu;
                        auxEnd = (auxEnd > end) ? end : auxEnd;
                        t_index indexes = {begin, auxEnd};
                        begin = auxEnd;

                        mtx.unlock();
#ifdef NDEBUG
                        std::cout << "\033[0;33m" << "GPU computing from: " << indexes.begin << " to: " << indexes.end
                                  << "\033[0m" << std::endl;
#endif
                        flow::input_port<0>(gpuNode).try_put(indexes);
                        auto args = std::make_tuple(1, 2, 3);
                        engine.SetStartGpu(tick_count::now());
                        dataStructures::try_put<Tgpu>(args, &gpuNode);
                    } else {
                        mtx.unlock();
                    }
                } else {
                    int ckcpu = engine.GetGPUChunk(begin, end);
                    if (ckcpu > 0) {    // si el trozo es > 0 se genera un token de GPU
                        int auxEnd = begin + ckcpu;
                        auxEnd = (auxEnd > end) ? end : auxEnd;
                        t_index indexes = {begin, auxEnd};
                        begin = auxEnd;
                        cpuCounter++;

                        mtx.unlock();
#ifdef NDEBUG
                        std::cout << "\033[0;33m" << "CPU computing from: " << indexes.begin << " to: " << indexes.end
                                  << "\033[0m" << std::endl;
#endif
                        cpuNode.try_put(indexes);
                    } else {

                        mtx.unlock();
                    }
                }
            }
        });

        std::array<unsigned int, 1> range{1};
        gpuNode.set_range(range);

        flow::make_edge(cpuNode, dispatcher);
        flow::make_edge(gpuJoiner, dispatcher);
        flow::make_edge(flow::output_port<0>(gpuNode), gpuJoiner);
        dispatcher.try_put(CPU);
        dispatcher.try_put(GPU);

        graph.wait_for_all();
        body->ShowCallback();
        body->~Body();
    }
};

#endif //BARNESLOGFIT_GRAPHLOGFIT_H
