//============================================================================
// Name			: BarnesHut.h
// Author		: Antonio Vilches
// Version		: 1.0
// Date			: 30 / 12 / 2014
// Copyright	: Department. Computer's Architecture (c)
// Description	: Implementation of barriers
//============================================================================

#ifndef __BARNESHUT__
#define __BARNESHUT__

#include <CL/sycl.hpp>
#include <math.h>


/*****************************************************************************
 * Data Structure
 * **************************************************************************/
typedef enum {
    CELL, BODY
} space_type;
typedef enum {
    UNUSED, USED
} used_type;

typedef struct {
    space_type type; // CELL is 0,  BODY is 1
    int index;
    used_type used; // O  = unused 1 = used
} OctTreeNode;

typedef struct {
    float mass;
    float posx;
    float posy;
    float posz;
    OctTreeNode more;
    OctTreeNode child[8]; //index of OctTreeNode array.
    float dsq;
    OctTreeNode next;
} OctTreeInternalNode;

typedef struct {
    float mass;
    float posx;
    float posy;
    float posz;
    float velx;
    float vely;
    float velz;
    float accx;
    float accy;
    float accz;
    float dsq;
    OctTreeNode next;
} OctTreeLeafNode;

#endif