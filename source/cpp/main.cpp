//============================================================================
// Name			: main.cpp
// Author		: Antonio Vilches
// Version		: 1.0
// Date			: 02 / 01 / 2015
// Copyright	: Department. Computer's Architecture (c)
// Description	: Main file of BarnesHut simulation
//============================================================================

#include <cstdlib> 
#include <iostream>
#include <fstream>
#include "Implementations/Body.h"
#include "Implementations/GraphScheduler.h"
#include "Implementations/LogFitEngine.h"
#include "Utils/ConsoleUtils.h"

using namespace std;
using namespace tbb;

/*****************************************************************************
 * Main Function
 * **************************************************************************/

int main(int argc, char** argv){
	
	//variables
	Body *body = new Body();
	Params p = ConsoleUtils::parseArgs(argc, argv);
	//end variables
    cout << "\033[0;33m" << "BarnesHut Simulation: "<< argv[1] << ", Number of CPU's cores: " << p.numcpus << ", Number of GPUs: " << p.numgpus << "\033[0m" << endl;

//    Functions::initialize<nullptr_t >(new dataStorage(), p, nullptr);

    LogFitEngine logFitEngine;
	GraphScheduler<Body, type_gpu, LogFitEngine> logFit(p, body, logFitEngine);


///*Initializing scheduler*/
//	LogFit * hs = LogFit::getInstance(&p);
//	ReadInput(p.inputData);
//	initialize_tree();
//
//	//Allocating object in GPU
//	body.AllocateMemoryObjects();
//
//	// time-step the system
//	//timesteps = 1;
//	hs->startTimeAndEnergy();
//	for (step = 0; step < timesteps; step++) {
//		cout << "Step " << step << endl;
//		register float diameter, centerx, centery, centerz;
//		ComputeCenterAndDiameter(nbodies, diameter, centerx, centery, centerz);
//
//		// create the tree's root
//		int root = NewNode(centerx, centery, centerz);
//		OctTreeInternalNode * p_r = &tree[root];
//
//		register float radius = diameter * 0.5;
//		for (int i = 0; i < nbodies; i++) {
//			Insert(p_r, i, radius); // grow the tree by inserting each body
//		}
//		gdiameter = diameter;
//		groot = root;
//		register int curr = 0;
//		curr = ComputeCenterOfMass(p_r, curr);
//		copy_aux_array();
//
//		ComputeOpeningCriteriaForEachCell(&tree[0], gdiameter * gdiameter * itolsq);
//
//		OctTreeNode node;
//		node.index = -1;
//		node.type = CELL;
//		node.used = 1;
//		ComputeNextMorePointers(&tree[root], node);
//
//		// serial CPU version
//		/*for (int i = 0; i < nbodies; i++) {
//			ComputeForce(root, &bodies[i], diameter);
//		}*/
//
//		hs->heterogeneous_parallel_for(0, nbodies, &body);
//
//		RecycleTree(); // recycle the tree
//
//		for (int i = 0; i < nbodies; i++) { // the iterations are independent: they can be executed in any order and in parallel
//			Advance(&bodies[i]); // advance the position and velocity of each body
//		}
//
//	} // end of time step
//	hs->endTimeAndEnergy();
//	hs->saveResultsForBench();
//
//	/*for (int i = 0; i < nbodies; i++) { // print result
//		Printfloat(bodies[i].posx);
//		printf(" ");
//		Printfloat(bodies[i].posy);
//		printf(" ");
//		Printfloat(bodies[i].posz);
//		printf("\n");
//	}*/

	return EXIT_SUCCESS;
}
