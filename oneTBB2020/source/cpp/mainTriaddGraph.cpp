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
    std::for_each(tbb::flow::interface11::opencl_info::available_devices().cbegin(), tbb::flow::interface11::opencl_info::available_devices().cend(),
                 [](const tbb::flow::opencl_device &d) {
                     std::cout << "Running in: " << d.name() << std::endl;
                     return d.type() == CL_DEVICE_TYPE_GPU;
                 }
    );

    auto logFitGraphScheduler{HelperFactories::SchedulerFactory::getInstance <
                              MySchedulerType ,
                              LogFitEngine,
                              TriaddGraphBody, t_index,
                              buffer_f, buffer_f, buffer_f >
                                                  (p, new TriaddGraphBody())};

    logFitGraphScheduler->startTimeAndEnergy();

    logFitGraphScheduler->StartParallelExecution();

    logFitGraphScheduler->endTimeAndEnergy();

    logFitGraphScheduler->saveResultsForBench();

    TriaddGraphBodyTest bodyTest;
    if (!bodyTest.runTest(*(TriaddGraphBody*)logFitGraphScheduler->getBody()))
        cout << CONSOLE_RED << "Verification failed" << CONSOLE_WHITE << endl;

    HelperFactories::SchedulerFactory::deleteInstance
            <MySchedulerType, LogFitEngine, TriaddGraphBody>(logFitGraphScheduler);

    return EXIT_SUCCESS;
}