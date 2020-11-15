//
// Created by juanp on 3/11/20.
//


#include <cstdlib>
#include <iostream>
#include <tbb/task_scheduler_init.h>
#include <scheduler/SchedulerFactory.h>
#include <utils/Utils.h>

#include "Implementations/Bodies/TriaddOneApiBody.h"
#include "Implementations/Tests/TriaddOneApiBodyTest.h"

using namespace std;
using namespace tbb;
using MySchedulerType = OneApiScheduler < LogFitEngine, TriaddOneApiBody >;
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
            TriaddOneApiBody,
            vector<double>> (p, new TriaddOneApiBody())};

    logFitScheduler->startTimeAndEnergy();

    logFitScheduler->StartParallelExecution();

    logFitScheduler->endTimeAndEnergy();

    logFitScheduler->saveResultsForBench();

    TriaddOneApiBodyTest bodyTest;
    if (!bodyTest.runTest(*(TriaddOneApiBody*)logFitScheduler->getBody()))
        cout << CONSOLE_RED << "Verification failed" ;

    HelperFactories::SchedulerFactory::deleteInstance
            <MySchedulerType, LogFitEngine, TriaddOneApiBody>(logFitScheduler);

    return EXIT_SUCCESS;
}