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

#include "Implementations/Bodies/BarnesBody.h"

using namespace std;
using namespace tbb;

/*****************************************************************************
 * Main Function
 * **************************************************************************/

int main(int argc, char** argv){

	Params p = ConsoleUtils::parseArgs(argc, argv);
    cout << CONSOLE_YELLOW << "BarnesHut Simulation: "<< p.inputData << ", Number of CPU's cores: " << p.numcpus <<
               ", Number of GPUs: " << p.numgpus << CONSOLE_WHITE << endl;

    LogFitEngine logFitEngine{p.numcpus, p.numgpus, 1, 1};

    size_t threadNum{p.numcpus + p.numgpus};

    task_scheduler_init taskSchedulerInit{static_cast<int>(threadNum)};
    ReadInput(p.inputData);
    initialize_tree();
    BarnesBody body(BarnesHutDataStructures::nbodies);

    GraphScheduler<BarnesBody, LogFitEngine, t_index, buffer_OctTreeLeafNode,
            buffer_OctTreeInternalNode, int, float, float> logFitGraphScheduler(p, &body, logFitEngine);

    logFitGraphScheduler.startTimeAndEnergy();
    for (step = 0; step < timesteps; step++) {
//        cout << "Step " << step << endl;
        register float diameter, centerx, centery, centerz;
        ComputeCenterAndDiameter(nbodies, diameter, centerx, centery, centerz);

        // create the tree's root
        int root = NewNode(centerx, centery, centerz);
        OctTreeInternalNode &p_r = tree[root];

        register float radius = diameter * 0.5;
        for (int i = 0; i < nbodies; i++) {
            Insert(p_r, i, radius); // grow the tree by inserting each body
        }
        gdiameter = diameter;
        groot = root;
        register int curr = 0;
        curr = ComputeCenterOfMass(p_r, curr);
        copy_to_bodies();

        ComputeOpeningCriteriaForEachCell(tree[0], gdiameter * gdiameter * itolsq);

        OctTreeNode node;
        node.index = -1;
        node.type = CELL;
        node.used = USED;
        ComputeNextMorePointers(tree[root], node);

        logFitGraphScheduler.StartParallelExecution();

        RecycleTree(); // recycle the tree

        for (int i = 0; i < nbodies; i++) { // the iterations are independent: they can be executed in any order and in parallel
            Advance(bodies[i]); // advance the position and velocity of each body
        }

    } // end of time step

    logFitGraphScheduler.endTimeAndEnergy();
    logFitGraphScheduler.saveResultsForBench();

	return EXIT_SUCCESS;
}