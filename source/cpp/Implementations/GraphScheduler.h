//
// Created by juanp on 6/4/18.
//

#ifndef BARNESLOGFIT_GRAPHLOGFIT_H
#define BARNESLOGFIT_GRAPHLOGFIT_H

#include "../DataStructures/ProvidedDataStructures.h"
#include "tbb/tick_count.h"
#include "../Interfaces/Engines/IEngine.h"

using namespace tbb;

template<typename TExectionBody, typename TSchedulerEngine,
        typename ...TArgs>
class GraphScheduler {
private:
    Params p;
    TExectionBody *body;
    TSchedulerEngine engine;

    tick_count startCpu, stopCpu, startGpu, stopGpu;
public:

    GraphScheduler(Params p, TExectionBody *body, TSchedulerEngine engine) : p(p), body(body), engine(engine) {}

    void StartParallelExecution() {
        flow::graph graph;
        int begin = 0, end = body->GetVsize(), cpuCounter = 0;
        flow::function_node<t_index, Type> cpuNode(graph, flow::unlimited,
                                                   [this, &cpuCounter](t_index indexes) -> Type {

                                                       startCpu = tick_count::now();
                                                       body->OperatorCPU(indexes.begin, indexes.end);
                                                       stopCpu = tick_count::now();


                                                       engine.recordCPUTh(indexes.end - indexes.begin, (stopCpu -
                                                                                                        startCpu).seconds());
#ifdef NDEBUG
                                                       std::cout << "CPU differential time: " << (stopCpu -
                                                                                                  startCpu).seconds()
                                                                 << std::endl;

                                                       std::cout << "Counter: " << ++cpuCounter << std::endl;
#endif
                                                       return CPU;
                                                   });

        dataStructures::GpuDeviceSelector gpuSelector;
        flow::opencl_node<tbb::flow::tuple<TArgs ...>> gpuNode(graph,
                                            flow::opencl_program<>(flow::opencl_program_type::SOURCE,
                                                                   p.openclFile).get_kernel(p.kernelName)
        ,gpuSelector);

        flow::function_node<t_index, Type> gpuCallbackReceiver(graph, flow::unlimited, [this](t_index indexes) -> Type {
            stopGpu = tick_count::now();

            engine.recordGPUTh(indexes.end - indexes.begin, (stopGpu - startGpu).seconds());

#ifdef NDEBUG
            std::cout << "GPU differential time: " << (stopGpu - startGpu).seconds()
                      << std::endl;
#endif
            return GPU;
        });

        flow::function_node<Type> processorSelectorNode(graph, flow::serial,
                                             [this, &cpuNode, &gpuNode, &begin, &end](Type token) {
                                                 if (begin < end) {
                                                     if (token == GPU) {
                                                         int ckgpu = engine.getGPUChunk(begin, end);
                                                         if (ckgpu > 0) {    // si el trozo es > 0 se genera un token de GPU
                                                             int auxEnd = begin + ckgpu;
                                                             t_index indexes = {begin, auxEnd};
                                                             begin = auxEnd;
#ifdef NDEBUG
                                                             std::cout << "\033[0;33m" << "GPU computing from: "
                                                                       << indexes.begin << " to: " << indexes.end
                                                                       << "\033[0m" << std::endl;
#endif
                                                             auto args = body->GetGPUArgs(indexes);
                                                             startGpu = tick_count::now();
                                                             dataStructures::try_put<0, TArgs...>(&gpuNode, args);
                                                         }
                                                     } else {
                                                         int ckcpu = engine.getGPUChunk(begin, end);
                                                         if (ckcpu > 0) {    // si el trozo es > 0 se genera un token de GPU
                                                             int auxEnd = begin + ckcpu;
                                                             auxEnd = (auxEnd > end) ? end : auxEnd;
                                                             t_index indexes = {begin, auxEnd};
                                                             begin = auxEnd;
#ifdef NDEBUG
                                                             std::cout << "\033[0;33m" << "CPU computing from: "
                                                                       << indexes.begin << " to: " << indexes.end
                                                                       << "\033[0m" << std::endl;
#endif
                                                             cpuNode.try_put(indexes);
                                                         }
                                                     }
                                                 }
                                             });

        std::array<unsigned int, 1> range{1};
        gpuNode.set_range(body->GetNDRange());

        flow::make_edge(cpuNode, processorSelectorNode);
        flow::make_edge(gpuCallbackReceiver, processorSelectorNode);
        flow::make_edge(flow::output_port<0>(gpuNode), gpuCallbackReceiver);

        for (int i = 0; i < p.numcpus; ++i) {
            processorSelectorNode.try_put(CPU);
        }
        for (int j = 0; j < p.numgpus; ++j) {
            processorSelectorNode.try_put(GPU);
        }

        graph.wait_for_all();
    }
};

#endif //BARNESLOGFIT_GRAPHLOGFIT_H
