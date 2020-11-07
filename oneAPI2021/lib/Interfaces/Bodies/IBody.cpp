#ifndef BARNESLOGFIT_IBODY_H
#define BARNESLOGFIT_IBODY_H

#include <tuple>

/*****************************************************************************
 * Interface IBody
 *
 * **************************************************************************/
class IBody {
public:
    virtual void OperatorCPU(int begin, int end) = 0;
};

#endif //BARNESLOGFIT_IBODY_H
