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
#include <Implementations/Engines/LogFitEngine.h>
#include <Implementations/Scheduler/GraphScheduler.h>

#include "Implementations/Bodies/TestExecutionBody.h"

using namespace std;
using namespace tbb;

/*****************************************************************************
 * Main Function
 * **************************************************************************/

int main(int argc, char** argv){

	Params p = ConsoleUtils::parseArgs(argc, argv);
    cout << CONSOLE_YELLOW << "Test Simulation: "<< p.inputData << ", Number of CPU's cores: " << p.numcpus <<
               ", Number of GPUs: " << p.numgpus << CONSOLE_WHITE << endl;

    LogFitEngine logFitEngine{p.numcpus, p.numgpus, 1, 1};

    size_t threadNum{p.numcpus + p.numgpus};

    task_scheduler_init taskSchedulerInit{static_cast<int>(threadNum)};
    TestExecutionBody body;

    GraphScheduler<TestExecutionBody, LogFitEngine, t_index, buffer_f, buffer_f, buffer_f> logFitGraphScheduler(p, &body, logFitEngine);

    logFitGraphScheduler.startTimeAndEnergy();
    logFitGraphScheduler.StartParallelExecution();

    logFitGraphScheduler.endTimeAndEnergy();
    logFitGraphScheduler.saveResultsForBench();

    body.ShowCallback();

	return EXIT_SUCCESS;
}