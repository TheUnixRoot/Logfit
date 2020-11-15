//
// Created by juanp on 7/11/20.
//


#include <cstdlib>
#include <iostream>
#include <tbb/task_scheduler_init.h>
#include <scheduler/SchedulerFactory_pipeline.h>
#include <utils/Utils.h>

#include "Implementations/Bodies/TriaddPipelineBody.h"
#include "Implementations/Tests/TriaddPipelineBodyTest.h"

using namespace std;
using namespace tbb;
using MySchedulerType = PipelineScheduler < LogFitEngine, TriaddPipelineBody, t_index,
        buffer_f, buffer_f, buffer_f >;
/*****************************************************************************
 * Main Function
 * **************************************************************************/

int main(int argc, char **argv) {

    Params p = ConsoleUtils::parseArgs(argc, argv);
    cout << CONSOLE_YELLOW << "Test Simulation: " << p.inputData << ", Number of CPU's cores: " << p.numcpus <<
         ", Number of GPUs: " << p.numgpus << CONSOLE_WHITE << endl;

    size_t threadNum{p.numcpus + p.numgpus};

    task_scheduler_init taskSchedulerInit{static_cast<int>(threadNum)};

    auto logFitScheduler{HelperFactories::SchedulerFactory::getInstance <
            MySchedulerType ,
            LogFitEngine,
            TriaddPipelineBody, t_index,
            buffer_f, buffer_f, buffer_f >
                                      (p, new TriaddPipelineBody())};

    logFitScheduler->startTimeAndEnergy();

    logFitScheduler->StartParallelExecution();

    logFitScheduler->endTimeAndEnergy();

    logFitScheduler->saveResultsForBench();

    TriaddPipelineBodyTest bodyTest;
    if (!bodyTest.runTest(*(TriaddPipelineBody*)logFitScheduler->getBody()))
        cout << CONSOLE_RED << "Verification failed" << CONSOLE_WHITE << endl;

    HelperFactories::SchedulerFactory::deleteInstance
            <MySchedulerType, LogFitEngine, TriaddPipelineBody>(logFitScheduler);

    return EXIT_SUCCESS;
}