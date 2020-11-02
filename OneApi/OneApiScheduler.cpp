//
// Created by juanp on 1/11/20.
//

#include "OneApiScheduler.h"
template <typename TSchedulerEngine, typename TExecutionBody,
typename ...TArgs>
OneApiScheduler<TSchedulerEngine, TExecutionBody,
        TArgs...>::OneApiScheduler(Params p, TExecutionBody &body, TSchedulerEngine &engine) :
            IScheduler(p), body{body}, engine{engine}
        {

        }