//
// Created by juanp on 26/10/20.
//

#ifndef HETEROGENEOUS_PARALLEL_FOR_SCHEDULER_H
#define HETEROGENEOUS_PARALLEL_FOR_SCHEDULER_H

#include "../../lib/Interfaces/Schedulers/IScheduler.cpp"
#include "../body/IOneApiBody.h"
#include "OneApiScheduler.h"
#include "OnePipelineScheduler.h"

namespace HelperFactories {
    enum SchedulerType : int;

    struct SchedulerFactory {
        template<typename TScheduler,
                typename TEngine,
                typename TBody,
                typename ...Args>
        static inline typename std::enable_if<std::is_same<TScheduler,
            OneApiScheduler<TEngine, TBody>>::value &&
                is_base_of<IOneApiBody, TBody>::value, TScheduler *>::type
        getInstance(Params p, TBody *body) {
            auto engine{HelperFactories::EngineFactory::getInstance<TEngine>(p.numcpus, p.numgpus, 1, 1)};
            return new OneApiScheduler<TEngine, TBody>(p, *body, *engine);
        }


        template<typename TScheduler,
                typename TEngine,
                typename TBody,
                typename ...Args>
        static inline typename std::enable_if<std::is_same<TScheduler,
                OnePipelineScheduler<TEngine, TBody>>::value &&
                    is_base_of<IOneApiBody, TBody>::value, TScheduler *>::type
        getInstance(Params p, TBody *body) {
            auto engine{HelperFactories::EngineFactory::getInstance<TEngine>(p.numcpus, p.numgpus, 1, 1)};
            return new OnePipelineScheduler<TEngine, TBody>(p, *body, *engine);
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
