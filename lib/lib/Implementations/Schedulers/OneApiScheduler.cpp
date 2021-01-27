//
// Created by juanp on 2/11/20.
//

#include "../../../include/scheduler/OneApiScheduler.h"
template<typename TSchedulerEngine, typename TExecutionBody>
OneApiScheduler<TSchedulerEngine, TExecutionBody>
        ::OneApiScheduler(Params p, TExecutionBody &body, TSchedulerEngine &engine) :
        IScheduler(p), body{body}, engine{engine},
        begin{0}, end{body.GetVsize()},
        cpuNode{graph, flow::unlimited,
            [&](execution_range indexes) -> ProcessorUnit {

                startCpu = tick_count::now();
                body.OperatorCPU(indexes.begin, indexes.end);
                stopCpu = tick_count::now();

                engine.recordCPUTh(indexes.end - indexes.begin, (stopCpu -
                                                                 startCpu).seconds());
#ifndef NDEBUG
                std::cout << "CPU differential time: " << (stopCpu -
                            startCpu).seconds()
                            << std::endl;
#endif
                return CPU;
        }},
        gpuNode{graph, flow::unlimited,
            [&](execution_range indexes) -> ProcessorUnit {

                startGpu = tick_count::now();
                body.OperatorGPU(indexes.begin, indexes.end);
                stopGpu = tick_count::now();

                engine.recordGPUTh(indexes.end - indexes.begin, (stopGpu -
                                                                 startGpu).seconds());
#ifndef NDEBUG
                std::cout << "GPU differential time: " << (stopGpu -
                            startGpu).seconds()
                            << std::endl;
#endif
                return GPU;
        }},
        processorSelectorNode{graph, flow::serial,
            [&](ProcessorUnit token) {
              if (begin < end) {
                  if (token == GPU) {
                      int gpuChunk = engine.getGPUChunk(begin, end);
                      if (gpuChunk > 0) {    // si el trozo es > 0 se genera un token de GPU
                          int auxEnd = begin + gpuChunk;
                          execution_range indexes = {begin, auxEnd};
                          begin = auxEnd;
#ifndef NDEBUG
                          std::cout << "\033[0;33m" << "GPU computing from: "
                                        << indexes.begin << " to: " << indexes.end
                                        << "\033[0m" << std::endl;
#endif
                          gpuNode.try_put(indexes);
                      }
                  } else {
                      int cpuChunk = engine.getCPUChunk(begin, end);
                      if (cpuChunk > 0) {    // si el trozo es > 0 se genera un token de GPU
                          int auxEnd = begin + cpuChunk;
                          auxEnd = (auxEnd > end) ? end : auxEnd;
                          execution_range indexes = {begin, auxEnd};
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
    flow::make_edge(gpuNode, processorSelectorNode);
}
template <typename TSchedulerEngine, typename TExecutionBody>
void OneApiScheduler<TSchedulerEngine, TExecutionBody>
        ::StartParallelExecution(){
    begin = 0;
    end = body.GetVsize();
    for (int j = 0; j < parameters.numgpus; ++j) {
        processorSelectorNode.try_put(GPU);
    }
    engine.reStart();
    for (int i = 0; i < parameters.numcpus; ++i) {
        processorSelectorNode.try_put(CPU);
    }
    graph.wait_for_all();
}
template <typename TSchedulerEngine, typename TExecutionBody>
void* OneApiScheduler<TSchedulerEngine, TExecutionBody>
        ::getEngine() {
    return &engine;
}

template <typename TSchedulerEngine, typename TExecutionBody>
TSchedulerEngine* OneApiScheduler<TSchedulerEngine, TExecutionBody>
        ::getTypedEngine() {
    return &engine;
}

template <typename TSchedulerEngine, typename TExecutionBody>
void* OneApiScheduler<TSchedulerEngine, TExecutionBody>
        ::getBody() {
    return &body;
}