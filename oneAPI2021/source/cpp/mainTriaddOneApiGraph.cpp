//
// Created by juanp on 3/11/20.
//

//#ifndef USE_NEW_PIPELINE
//#define USE_NEW_PIPELINE true
//#endif


#include <cstdlib>
#include <iostream>
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


    auto logFitOneApiScheduler{HelperFactories::SchedulerFactory::getInstance <
            MySchedulerType ,
            LogFitEngine,
            TriaddOneApiBody,
            vector<double>> (p, new TriaddOneApiBody())};

    logFitOneApiScheduler->startTimeAndEnergy();

    logFitOneApiScheduler->StartParallelExecution();

    logFitOneApiScheduler->endTimeAndEnergy();

    logFitOneApiScheduler->saveResultsForBench();

    TriaddOneApiBodyTest bodyTest;
    if (!bodyTest.runTest(*(TriaddOneApiBody*)logFitOneApiScheduler->getBody()))
        cout << CONSOLE_RED << "Verification failed" ;

    HelperFactories::SchedulerFactory::deleteInstance
            <MySchedulerType, LogFitEngine, TriaddOneApiBody>(logFitOneApiScheduler);

    return EXIT_SUCCESS;
}