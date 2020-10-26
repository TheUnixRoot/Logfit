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

    virtual void sendObjectToGPU(int begin, int end, void * null_ptr) {}

    virtual void OperatorGPU(int begin, int end, void * null_ptr) {}

    virtual void getBackObjectFromGPU(int begin, int end, void * null_ptr){}

};

#endif //BARNESLOGFIT_IBODY_H
