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
#include "Implementations/Bodies/BarnesBody.h"
//#include "Implementations/Bodies/TestExecutionBody.h"
#include "Implementations/GraphScheduler.h"
#include "Implementations/Engines/LogFitEngine.h"
#include "Utils/ConsoleUtils.h"

using namespace std;
using namespace tbb;

/*****************************************************************************
 * Main Function
 * **************************************************************************/

int main(int argc, char** argv){

	Params p = ConsoleUtils::parseArgs(argc, argv);
    cout << "\033[0;33m" << "BarnesHut Simulation: "<< p.inputData << ", Number of CPU's cores: " << p.numcpus <<
                                                           ", Number of GPUs: " << p.numgpus << "\033[0m" << endl;
    {
        LogFitEngine logFitEngine{p.numcpus, p.numgpus, 1, 1};

        ReadInput(p.inputData);
        initialize_tree();
        BarnesBody body(BarnesHutDataStructures::nbodies);

        GraphScheduler<BarnesBody, LogFitEngine, t_index, buffer_OctTreeLeafNode,
                buffer_OctTreeInternalNode, int, float, float> logFit(p, &body, logFitEngine);

        for (step = 0; step < timesteps; step++) {
            cout << "Step " << step << endl;
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

            logFit.StartParallelExecution();

            RecycleTree(); // recycle the tree

            for (int i = 0; i < nbodies; i++) { // the iterations are independent: they can be executed in any order and in parallel
                Advance(bodies[i]); // advance the position and velocity of each body
            }

        } // end of time step

        for (int i = 0; i < nbodies; i++) { // print result
            Printfloat(bodies[i].posx);
            printf(" ");
            Printfloat(bodies[i].posy);
            printf(" ");
            Printfloat(bodies[i].posz);
            printf("\n");
        }
    }
//    {
//        BarnesBody body;
//        LogFitEngine logFitEngine{p.numcpus, p.numgpus, 1, 1};
//        GraphScheduler<BarnesBody, LogFitEngine, t_index, OctTreeLeafNode, OctTreeInternalNode, int, float, float> logFit(p, &body,
//                                                                                                  logFitEngine);
//        logFit.StartParallelExecution();
//        body.ShowCallback();
//    }
//    {
//        TestExecutionBody body;
//        LogFitEngine logFitEngine{p.numcpus, p.numgpus, 1, 1};
//        GraphScheduler<TestExecutionBody, LogFitEngine, t_index, buffer_f, buffer_f, buffer_f> logfit(p, &body,
//                                                                                                       logFitEngine);
//        logfit.StartParallelExecution();
//        body.ShowCallback();
//
//    }
	return EXIT_SUCCESS;
}
