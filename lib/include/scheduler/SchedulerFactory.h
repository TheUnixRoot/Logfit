//
// Created by juanp on 26/10/20.
//

#ifndef HETEROGENEOUS_PARALLEL_FOR_SCHEDULER_H
#define HETEROGENEOUS_PARALLEL_FOR_SCHEDULER_H

#include "../../lib/Interfaces/Scheduler/IScheduler.cpp"

#include "GraphScheduler.h"
#include "PipelineScheduler.h"

namespace HelperFactories {
    enum SchedulerType : int;

    struct SchedulerFactory {
        template<typename TScheduler,
                typename TEngine,
                typename TBody,
                typename ...Args>
        static inline typename std::enable_if<std::is_same<TScheduler,
                GraphScheduler<TEngine, TBody, Args ...>>::value, TScheduler *>::type
        getInstance(Params p, TBody *body) {
            auto engine{HelperFactories::EngineFactory::getInstance<TEngine>(p.numcpus, p.numgpus, 1, 1)};
            return new GraphScheduler<TEngine, TBody, Args ...>(p, *body, *engine);
        }

        template<typename TScheduler,
                typename TEngine,
                typename TBody,
                typename ...Args>
        static inline typename std::enable_if<std::is_same<TScheduler,
                PipelineScheduler<TEngine, TBody, Args ...>>::value, TScheduler *>::type
        getInstance(Params p, TBody *body) {
            auto engine{HelperFactories::EngineFactory::getInstance<TEngine>(p.numcpus, p.numgpus, 1, 1)};
            return new PipelineScheduler<TEngine, TBody, Args ...>(p, *body, *engine);
        }

        template<typename TScheduler,
                 typename TEngine,
                 typename TBody>
        static inline void deleteInstance(TScheduler *instance) {
            delete ((TBody*)instance->getBody());
            HelperFactories::EngineFactory::deleteInstance<TEngine>((TEngine*)instance->getEngine());
            delete (instance);
        }
    };
}
#endif //HETEROGENEOUS_PARALLEL_FOR_SCHEDULER_H
