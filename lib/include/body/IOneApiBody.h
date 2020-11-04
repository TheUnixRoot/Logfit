//
// Created by juanp on 2/11/20.
//

#ifndef HETEROGENEOUS_PARALLEL_FOR_IONEAPIBODY_H
#define HETEROGENEOUS_PARALLEL_FOR_IONEAPIBODY_H
#include <type_traits>
#include <CL/cl.h>
#include "../../lib/Interfaces/Bodies/IBody.cpp"
/*****************************************************************************
 * Interface IOneAPiBody
 * Body base class for OneApi Schedulers implementations
 * **************************************************************************/
class IOneApiBody : IBody {
public:
    virtual void OperatorCPU(int begin, int end) = 0;

    virtual void OperatorGPU(int begin, int end)  = 0;

    virtual int GetVsize() = 0;
};


#endif //HETEROGENEOUS_PARALLEL_FOR_IONEAPIBODY_H
