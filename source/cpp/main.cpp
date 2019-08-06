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
#include "Implementations/ExecutionBody.h"
#include "Implementations/GraphScheduler.h"
#include "Implementations/Engines/LogFit.h"
#include "Implementations/Engines/Dynamic.h"
#include "Utils/ConsoleUtils.h"
#include "DataStructures/ProvidedDataStructures.h"

using namespace std;
using namespace tbb;

/*****************************************************************************
 * Main Function
 * **************************************************************************/

int main(int argc, char** argv){

	Params p = ConsoleUtils::parseArgs(argc, argv);
    cout << "\033[0;33m" << "BarnesHut Simulation: "<< argv[1] << ", Number of CPU's cores: " << p.numcpus <<
                                                           ", Number of GPUs: " << p.numgpus << "\033[0m" << endl;
    {
        ExecutionBody body;
        LogFitEngine logFitEngine{p.numcpus, p.numgpus, 1, 1};
        GraphScheduler<ExecutionBody, LogFitEngine, t_index, buffer_f, buffer_f, buffer_f> logFit(p, &body,
                                                                                                  logFitEngine);
        logFit.StartParallelExecution();
        body.ShowCallback();
    }
    {
        ExecutionBody body;
        DynamicEngine dynamicEngine{p.numcpus, p.numgpus, 3};
        GraphScheduler<ExecutionBody, DynamicEngine, t_index, buffer_f, buffer_f, buffer_f> dynamic(p, &body,
                                                                                                    dynamicEngine);
        dynamic.StartParallelExecution();
        body.ShowCallback();
    }
	return EXIT_SUCCESS;
}
