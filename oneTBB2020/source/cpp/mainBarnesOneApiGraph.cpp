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

#include "Implementations/Bodies/BarnesOneApiBody.h"

using namespace std;
using MySchedulerType = OneApiScheduler<LogFitEngine, BarnesOneApiBody>;
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

    auto body{new BarnesOneApiBody(p)};
    auto logFitScheduler{HelperFactories::SchedulerFactory::getInstance <
            MySchedulerType ,
            LogFitEngine,
            BarnesOneApiBody>
            (p, body)};

    logFitScheduler->startTimeAndEnergy();
    for (body->step = 0; body->step < body->timesteps; body->step++) {

//        cout << "Step " << step << endl;

        float diameter, centerx, centery, centerz;
        body->ComputeCenterAndDiameter(diameter, centerx, centery, centerz);

        // create the tree's root
        int root = body->NewNode(centerx, centery, centerz);
        OctTreeInternalNode &p_r = body->tree[root];

        float radius = diameter * 0.5;
        for (int i = 0; i < body->nbodies; i++) {
            body->Insert(p_r, i, radius); // grow the tree by inserting each body
        }
        body->gdiameter = diameter;
        body->groot = root;
        int curr = 0;
        curr = body->ComputeCenterOfMass(p_r, curr);
        body->copy_to_bodies();

        body->ComputeOpeningCriteriaForEachCell(body->tree[0], body->gdiameter * body->gdiameter * body->itolsq);

        OctTreeNode node;
        node.index = -1;
        node.type = CELL;
        node.used = USED;
        body->ComputeNextMorePointers(body->tree[root], node);

        logFitScheduler->StartParallelExecution();

        body->RecycleTree(body->num_cells); // recycle the tree

        for (int i = 0;
             i < body->nbodies; i++) { // the iterations are independent: they can be executed in any order and in parallel
            body->Advance(body->bodies[i]); // advance the position and velocity of each body
        }

    } // end of time step

    logFitScheduler->endTimeAndEnergy();
    logFitScheduler->saveResultsForBench();

    logFitScheduler->getTypedEngine()->printGPUChunks();

    HelperFactories::SchedulerFactory::deleteInstance
            <MySchedulerType, LogFitEngine, BarnesOneApiBody>(logFitScheduler);

    return EXIT_SUCCESS;
}