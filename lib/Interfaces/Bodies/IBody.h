#ifndef BARNESLOGFIT_IBODY_H
#define BARNESLOGFIT_IBODY_H

#include <tuple>

/*****************************************************************************
 * Interface IBody
 *
 * **************************************************************************/
template <typename NDRange, typename Tindex, typename ...Args>
class IBody {
public:
    virtual int GetVsize() = 0;

    virtual void OperatorCPU(int begin, int end) = 0;

    virtual std::tuple<Tindex, Args ...> GetGPUArgs(Tindex indexes) = 0;

    virtual NDRange GetNDRange() = 0;
};

#endif //BARNESLOGFIT_IBODY_H
