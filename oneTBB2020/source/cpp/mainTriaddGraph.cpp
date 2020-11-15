//============================================================================
// Name			: main.cpp
// Author		: Juan Pedro Dominguez
// Version		: 1.0
// Date			: 08 / 01 / 2019
// Copyright	: Department. Computer's Architecture (c)
// Description	: Main file of BarnesHut simulation
//============================================================================

#include <cstdlib>
#include <iostream>
#include <tbb/task_scheduler_init.h>
#include <scheduler/SchedulerFactory.h>
#include <utils/Utils.h>

#include "Implementations/Bodies/TriaddGraphBody.h"
#include "Implementations/Tests/TriaddGraphBodyTest.h"

using namespace std;
using namespace tbb;
using MySchedulerType = GraphScheduler < LogFitEngine, TriaddGraphBody, t_index,
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
                              TriaddGraphBody, t_index,
                              buffer_f, buffer_f, buffer_f >
                                                  (p, new TriaddGraphBody())};

    logFitScheduler->startTimeAndEnergy();

    logFitScheduler->StartParallelExecution();

    logFitScheduler->endTimeAndEnergy();

    logFitScheduler->saveResultsForBench();

    TriaddGraphBodyTest bodyTest;
    if (!bodyTest.runTest(*(TriaddGraphBody*)logFitScheduler->getBody()))
        cout << CONSOLE_RED << "Verification failed" << CONSOLE_WHITE << endl;

    HelperFactories::SchedulerFactory::deleteInstance
            <MySchedulerType, LogFitEngine, TriaddGraphBody>(logFitScheduler);

    return EXIT_SUCCESS;
}