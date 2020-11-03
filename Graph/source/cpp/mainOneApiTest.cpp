//
// Created by juanp on 3/11/20.
//


#include <cstdlib>
#include <iostream>
#include <tbb/task_scheduler_init.h>
#include <scheduler/SchedulerFactory.h>
#include <utils/Utils.h>

#include "Implementations/Bodies/TestOneApiBody.h"

using namespace std;
using namespace tbb;
using MySchedulerType = OneApiScheduler < LogFitEngine, TestOneApiBody >;
/*****************************************************************************
 * Main Function
 * **************************************************************************/

int main(int argc, char **argv) {

    Params p = ConsoleUtils::parseArgs(argc, argv);
    cout << CONSOLE_YELLOW << "Test Simulation: " << p.inputData << ", Number of CPU's cores: " << p.numcpus <<
         ", Number of GPUs: " << p.numgpus << CONSOLE_WHITE << endl;

    size_t threadNum{p.numcpus + p.numgpus};

    task_scheduler_init taskSchedulerInit{static_cast<int>(threadNum)};

    auto logFitOneApiScheduler{HelperFactories::SchedulerFactory::getInstance <
            MySchedulerType ,
            LogFitEngine,
            TestOneApiBody,
            vector<double>> (p, new TestOneApiBody())};

    logFitOneApiScheduler->startTimeAndEnergy();

    logFitOneApiScheduler->StartParallelExecution();

    logFitOneApiScheduler->endTimeAndEnergy();

    logFitOneApiScheduler->saveResultsForBench();

    HelperFactories::SchedulerFactory::deleteInstance
            <MySchedulerType, LogFitEngine, TestOneApiBody>(logFitOneApiScheduler);

    return EXIT_SUCCESS;
}