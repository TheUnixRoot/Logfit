//
// Created by juanp on 6/4/18.
//

#ifndef BARNESLOGFIT_GRAPHLOGFIT_H
#define BARNESLOGFIT_GRAPHLOGFIT_H

#include "../../Interfaces/Engines/IEngine.h"
#include "../../Interfaces/Scheduler/IScheduler.h"

using namespace tbb;

template<typename TExectionBody, typename TSchedulerEngine,
        typename ...TArgs>
class GraphScheduler : public IScheduler {
private:
    TExectionBody *body;
    TSchedulerEngine engine;

    flow::graph graph;
    int begin, end, cpuCounter;
    flow::function_node<t_index, Type> cpuNode;
    dataStructures::GpuDeviceSelector gpuSelector;
    flow::opencl_node<tbb::flow::tuple<TArgs ...>> gpuNode;
    flow::function_node<t_index, Type> gpuCallbackReceiver;
    flow::function_node<Type> processorSelectorNode;

public:

    GraphScheduler(Params p, TExectionBody *body, TSchedulerEngine engine) :
        IScheduler(p), body{body}, engine{engine},
        begin{0}, end{body->GetVsize()}, cpuCounter{0},
        cpuNode{graph, flow::unlimited,
                [this, body, &engine](t_index indexes) -> Type {

                    startCpu = tick_count::now();
                    body->OperatorCPU(indexes.begin, indexes.end);
                    stopCpu = tick_count::now();


                    engine.recordCPUTh(indexes.end - indexes.begin, (stopCpu -
                                                                     startCpu).seconds());
#ifndef NDEBUG
std::cout << "CPU differential time: " << (stopCpu -
startCpu).seconds()
<< std::endl;

std::cout << "Counter: " << ++cpuCounter << std::endl;
#endif
                                                return CPU;
                                                    }},
        gpuNode{graph,
                flow::opencl_program<>(flow::opencl_program_type::SOURCE,
                parameters.openclFile).get_kernel(parameters.kernelName)
                ,gpuSelector},
        gpuCallbackReceiver{graph, flow::unlimited,
                [this, &engine](t_index indexes) -> Type {
                    stopGpu = tick_count::now();
                    engine.recordGPUTh(indexes.end - indexes.begin, (stopGpu - startGpu).seconds());

#ifndef NDEBUG
        std::cout << "GPU differential time: " << (stopGpu - startGpu).seconds()
                      << std::endl;
#endif
                return GPU;
        }},
        processorSelectorNode{graph, flow::serial,
                [this, body, &engine](Type token) {
                    if (begin < end) {
                        if (token == GPU) {
                            int gpuChunk = engine.getGPUChunk(begin, end);
                            if (gpuChunk > 0) {    // si el trozo es > 0 se genera un token de GPU
                                int auxEnd = begin + gpuChunk;
                                t_index indexes = {begin, auxEnd};
                                begin = auxEnd;
#ifndef NDEBUG
                                std::cout << "\033[0;33m" << "GPU computing from: "
                                          << indexes.begin << " to: " << indexes.end
                                          << "\033[0m" << std::endl;
#endif
                                gpuNode.set_range(body->GetNDRange());
                                auto args = body->GetGPUArgs(indexes);
                                startGpu = tick_count::now();
                                dataStructures::try_put<0, TArgs...>(&gpuNode, args);
                            }
                        } else {
                            int cpuChunk = engine.getCPUChunk(begin, end);
                            if (cpuChunk > 0) {    // si el trozo es > 0 se genera un token de GPU
                                int auxEnd = begin + cpuChunk;
                                auxEnd = (auxEnd > end) ? end : auxEnd;
                                t_index indexes = {begin, auxEnd};
                                begin = auxEnd;
#ifndef NDEBUG
                                std::cout << "\033[0;33m" << "CPU computing from: "
                                                                       << indexes.begin << " to: " << indexes.end
                                                                       << "\033[0m" << std::endl;
#endif
                                cpuNode.try_put(indexes);
                            }
                        }
                    }
                }}
    {
        flow::make_edge(cpuNode, processorSelectorNode);
        flow::make_edge(gpuCallbackReceiver, processorSelectorNode);
        flow::make_edge(flow::output_port<0>(gpuNode), gpuCallbackReceiver);
    }

    void StartParallelExecution() {
        begin = 0;
        end = body->GetVsize();
        cpuCounter = 0;
        engine.reStart();
        for (int i = 0; i < parameters.numcpus; ++i) {
            processorSelectorNode.try_put(CPU);
        }
        for (int j = 0; j < parameters.numgpus; ++j) {
            processorSelectorNode.try_put(GPU);
        }

        graph.wait_for_all();
    }
};

#endif //BARNESLOGFIT_GRAPHLOGFIT_H
