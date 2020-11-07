//
// Created by juanp on 7/11/20.
//

//#ifndef USE_NEW_PIPELINE
//#define USE_NEW_PIPELINE true
//#endif


#include <cstdlib>
#include <iostream>
#include <tbb/task_scheduler_init.h>

#include <scheduler/SchedulerFactory.h>
#include <utils/Utils.h>

#include "Implementations/Bodies/TriaddOneApiBody.h"
#include "Implementations/Tests/TriaddOneApiBodyTest.h"

using namespace std;
using namespace tbb;
using MySchedulerType = OnePipelineScheduler < LogFitEngine, TriaddOneApiBody >;
/*****************************************************************************
 * Main Function
 * **************************************************************************/

int main(int argc, char **argv) {

    Params p = ConsoleUtils::parseArgs(argc, argv);
    cout << CONSOLE_YELLOW << "Test Simulation: " << p.inputData << ", Number of CPU's cores: " << p.numcpus <<
         ", Number of GPUs: " << p.numgpus << CONSOLE_WHITE << endl;

    size_t threadNum{p.numcpus + p.numgpus};

    task_scheduler_init taskSchedulerInit{static_cast<int>(threadNum)};
    auto body{new TriaddOneApiBody()};
    auto logFitOneApiScheduler{HelperFactories::SchedulerFactory::getInstance <
            MySchedulerType ,
            LogFitEngine,
            TriaddOneApiBody,
            vector<double>> (p, body)};

    logFitOneApiScheduler->startTimeAndEnergy();

    logFitOneApiScheduler->StartParallelExecution();

    logFitOneApiScheduler->endTimeAndEnergy();

    logFitOneApiScheduler->saveResultsForBench();

    if (!(std::make_unique <TriaddOneApiBodyTest> ())->runTest(*(TriaddOneApiBody*)logFitOneApiScheduler->getBody()))
        cout << CONSOLE_RED << "Verification failed" ;

    HelperFactories::SchedulerFactory::deleteInstance
            <MySchedulerType, LogFitEngine, TriaddOneApiBody>(logFitOneApiScheduler);

    return EXIT_SUCCESS;
}