
#ifndef LIBRARY_SCHEDULERFACTORY_H
#define LIBRARY_SCHEDULERFACTORY_H

#include "../Implementations/Scheduler/GraphScheduler.h"
#include "../Implementations/Scheduler/PipelineScheduler.h"
#include "EngineFactory.h"

namespace HelperFactories {
    enum SchedulerType : int {
        Graph = 0, Pipeline = 1
    };

    struct SchedulerFactory {
        template<typename TScheduler,
                typename TEngine,
                typename TBody,
                typename ...Args>
        static inline typename std::enable_if<std::is_same<TScheduler,
            GraphScheduler<TEngine, TBody, Args ...>>::value , TScheduler*>::type
        getInstance(Params p, TBody *body) {
            auto engine{HelperFactories::EngineFactory::getInstance<TEngine>(p.numcpus, p.numgpus, 1, 1)};
            return new GraphScheduler<TEngine, TBody, Args ...>(p, *body, *engine);
        }
        template<typename TScheduler,
                typename TEngine,
                typename TBody,
                typename ...Args>
        static inline typename std::enable_if<std::is_same<TScheduler,
                PipelineScheduler<TEngine, TBody, Args ...>>::value , TScheduler*>::type
        getInstance(Params p, TBody *body) {
            auto engine{HelperFactories::EngineFactory::getInstance<TEngine>(p.numcpus, p.numgpus, 1, 1)};
            return new PipelineScheduler<TEngine, TBody, Args ...>(p, *body, *engine);
        }

        template<typename TScheduler>
        static inline void deleteInstance(TScheduler* instance) {
            delete(instance->getBody());
            HelperFactories::EngineFactory::deleteInstance(instance->getEngine());
            delete(instance);
        }
    };

}

#endif //LIBRARY_SCHEDULERFACTORY_H