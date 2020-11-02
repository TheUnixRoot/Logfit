//
// Created by juanp on 1/11/20.
//

#ifndef HETEROGENEOUS_PARALLEL_FOR_IONEAPIBODY_H
#define HETEROGENEOUS_PARALLEL_FOR_IONEAPIBODY_H
#include "lib/IBody.cpp"
/*****************************************************************************
 * Interface IOneAPiBody
 * Body base class for OneApi Schedulers implementations
 * **************************************************************************/
template<typename NDRange, typename Tindex, typename ...Args>
class IOneApiBody : IBody<NDRange, Tindex, Args...> {
public:
    virtual int GetVsize() = 0;

    virtual void OperatorCPU(int begin, int end) = 0;

    virtual std::tuple<Tindex, Args ...> GetGPUArgs(Tindex indexes) = 0;

    virtual NDRange GetNDRange() = 0;

    virtual void OperatorGPU(int begin, int end, void *null_ptr)  = 0;

};

std::false_type is_one_api_implementation(...);
template <typename ...T>
std::true_type is_one_api_implementation(IOneApiBody<T...> const volatile&);
template <typename T>
using is_one_api_body = decltype(is_one_api_implementation(std::declval<T&>()));

#endif //HETEROGENEOUS_PARALLEL_FOR_IONEAPIBODY_H
