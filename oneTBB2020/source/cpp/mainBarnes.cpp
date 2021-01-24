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
#include <tbb/global_control.h>
#include <scheduler/SchedulerFactory.h>
#include <utils/Utils.h>

#include "Implementations/Bodies/BarnesBody.h"
#include "DataStructures/BarnesDataStructures.h"

using namespace std;
using namespace tbb;
using MySchedulerType = GraphScheduler<LogFitEngine, BarnesBody, t_index,
        buffer_OctTreeLeafNode, buffer_OctTreeInternalNode, int, float, float>;
/*****************************************************************************
 * Main Function
 * **************************************************************************/

int main(int argc, char **argv) {

    Params p = ConsoleUtils::parseArgs(argc, argv);
    cout << CONSOLE_YELLOW << "Test Simulation: " << p.inputData << ", Number of CPU's cores: " << p.numcpus <<
         ", Number of GPUs: " << p.numgpus << CONSOLE_WHITE << endl;

    size_t threadNum{p.numcpus + p.numgpus};

    auto mp = global_control::max_allowed_parallelism;
    global_control gc{mp, threadNum};

    ReadInput(p.inputData);
    initialize_tree();

    auto logFitScheduler{HelperFactories::SchedulerFactory::getInstance <
            MySchedulerType ,
            LogFitEngine,
            BarnesBody, t_index,
            buffer_OctTreeLeafNode, buffer_OctTreeInternalNode, int, float, float>
                                      (p, new BarnesBody(BarnesHutDataStructures::nbodies))};

    logFitScheduler->startTimeAndEnergy();
    for (step = 0; step < timesteps; step++) {

//        cout << "Step " << step << endl;

        float diameter, centerx, centery, centerz;
        ComputeCenterAndDiameter(nbodies, diameter, centerx, centery, centerz);

        // create the tree's root
        int root = NewNode(centerx, centery, centerz);
        OctTreeInternalNode &p_r = tree[root];

        float radius = diameter * 0.5;
        for (int i = 0; i < nbodies; i++) {
            Insert(p_r, i, radius); // grow the tree by inserting each body
        }
        gdiameter = diameter;
        groot = root;
        int curr = 0;
        curr = ComputeCenterOfMass(p_r, curr);
        copy_to_bodies();

        ComputeOpeningCriteriaForEachCell(tree[0], gdiameter * gdiameter * itolsq);

        OctTreeNode node;
        node.index = -1;
        node.type = CELL;
        node.used = USED;
        ComputeNextMorePointers(tree[root], node);

        logFitScheduler->StartParallelExecution();

        RecycleTree(); // recycle the tree

        for (int i = 0; i < nbodies; i++) { // the iterations are independent: they can be executed in any order and in parallel
            Advance(bodies[i]); // advance the position and velocity of each body
        }

    } // end of time step

    logFitScheduler->endTimeAndEnergy();
    logFitScheduler->saveResultsForBench();

    logFitScheduler->getTypedEngine()->printGPUChunks();

    HelperFactories::SchedulerFactory::deleteInstance
            <MySchedulerType, LogFitEngine, BarnesBody>(logFitScheduler);

    return EXIT_SUCCESS;
}