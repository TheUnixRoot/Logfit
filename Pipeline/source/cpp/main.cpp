//============================================================================
// Name			: main.cpp
// Author		: Antonio Vilches
// Version		: 1.0
// Date			: 02 / 01 / 2015
// Copyright	: Department. Computer's Architecture (c)
// Description	: Main file of BarnesHut simulation
//============================================================================


//#define DEBUG
//#define DEEP_CPU_REPORT
//#define DEEP_GPU_REPORT
//#define DYNAMIC
//#define ORACLE
//#define ORACLEMOD
//#define FIXEDCHUNK
//#define CONCORD
//#define HDSSS
#define LOGFIT 1
//#define LOGFITMODEL
//#define PJTRACER
//#define OVERHEAD_STUDY


#include <cstdlib> 
#include <iostream>
#include <fstream>
#include "BarnesHut.h" 

#ifdef ORACLE
#include "Oracle.h"
#endif
#ifdef ORACLEMOD
#include "OracleMod.h"
#endif
#ifdef DYNAMIC
#include "Dynamic.h"
#endif
#ifdef FIXEDCHUNK
#include "FixedChunkSize.h"
#endif
#ifdef CONCORD
#include "Concord.h"
#endif
#ifdef HDSSS
#include "HDSS.h"
#endif
#ifdef LOGFIT
#include "LogFit.h"
#endif
#ifdef LOGFITMODEL
#include "LogFitModel.h"
#endif
#ifdef FASTFIT
#include "FastFit.h"
#endif
#include "Body.h" 

using namespace std;
using namespace tbb;

/*****************************************************************************
 * Main Function
 * **************************************************************************/

int main(int argc, char** argv){
	
	//variables
	Body body;
	Params p;
	//end variables

	/*Checking command line parameters*/
	if (argc < 4) {
		cerr << "argumentos: fichero_entrada numcpus numgpus [ratio | gpuCHUNK] [cpuChunk]" << endl;
		exit(-1);
	}
	p.numcpus = atoi(argv[2]);
	p.numgpus = atoi(argv[3]);
#if defined(DYNAMIC)
	p.gpuChunk = atoi(argv[4]);
#endif
#if defined(ORACLE) || defined(ORACLEMOD)
	p.ratioG = atof(argv[4]);
#endif
#ifdef FIXEDCHUNK
	p.gpuChunk = atoi(argv[4]);
	p.cpuChunk = atoi(argv[5]);
#endif
#ifdef LOGFITMODEL
	/*
	p.doeprof = 1;
	p.niterprof = 1;
	if (argc >= 5)
		p.doeprof = atoi(argv[4]);
	if (argc >= 6)
		p.niterprof = atoi(argv[5]);
	if (argc >= 7) 
		profChunk = atoi(argv[6]);
		*/
#endif
#ifdef FASTFIT
    if (argc >= 5)
        p.gpuChunk = atoi(argv[4]);
    else
        p.gpuChunk = 0;
#endif
	sprintf(p.benchName, "BarnesHut");
	sprintf(p.kernelName, "IterativeForce");
	cerr << "BarnesHut Simulation: "<< argv[1] << ", Number of CPU's cores: " << p.numcpus << ", Number of GPUs: " << p.numgpus << endl;

	
/*Initializing scheduler*/
#if defined(ORACLE) || defined(ORACLEMOD)
	Oracle * hs = Oracle::getInstance(&p);
#endif
#ifdef DYNAMIC
	Dynamic * hs = Dynamic::getInstance(&p);
#endif
#ifdef FIXEDCHUNK
	FixedChunkSize * hs = FixedChunkSize::getInstance(&p);
#endif
#ifdef CONCORD
	Concord * hs = Concord::getInstance(&p);
#endif
#ifdef HDSSS
	HDSS * hs = HDSS::getInstance(&p);
#endif
#ifdef LOGFIT
	LogFit * hs = LogFit::getInstance(&p);
#endif
#ifdef LOGFITMODEL
	LogFit * hs = LogFit::getInstance(&p);
#endif
#ifdef FASTFIT
        FastFit * hs = FastFit::getInstance(&p);
#endif
	ReadInput(argv[1]);
	initialize_tree();

	//Allocating object in GPU
	body.AllocateMemoryObjects();

	// time-step the system
	//timesteps = 1;
	hs->startTimeAndEnergy();
	for (step = 0; step < timesteps; step++) {
		cout << "Step " << step << endl;
		register float diameter, centerx, centery, centerz;
		ComputeCenterAndDiameter(nbodies, diameter, centerx, centery, centerz);

		// create the tree's root
		int root = NewNode(centerx, centery, centerz);

		register float radius = diameter * 0.5;
		for (int i = 0; i < nbodies; i++) {
			Insert(tree[root], i, radius); // grow the tree by inserting each body
		}
		gdiameter = diameter;
		groot = root;
		register int curr = 0;
		curr = ComputeCenterOfMass(tree[root], curr);
        copy_to_bodies();

		ComputeOpeningCriteriaForEachCell(tree[0], gdiameter * gdiameter * itolsq);

		OctTreeNode node;
		node.index = -1;
		node.type = CELL;
		node.used = 1;
		ComputeNextMorePointers(tree[root], node);

		// serial CPU version
		/*for (int i = 0; i < nbodies; i++) {
			ComputeForce(root, &bodies[i], diameter);
		}*/

		hs->heterogeneous_parallel_for(0, nbodies, &body);
		
		RecycleTree(); // recycle the tree

		for (int i = 0; i < nbodies; i++) { // the iterations are independent: they can be executed in any order and in parallel
			Advance(bodies[i]); // advance the position and velocity of each body
		}

	} // end of time step
	hs->endTimeAndEnergy();
	hs->saveResultsForBench();

	for (int i = 0; i < nbodies; i++) { // print result
		Printfloat(bodies[i].posx);
		printf(" ");
		Printfloat(bodies[i].posy);
		printf(" ");
		Printfloat(bodies[i].posz);
		printf("\n");
	}

	return EXIT_SUCCESS;
}