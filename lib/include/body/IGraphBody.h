//
// Created by juanp on 2/11/20.
//

#ifndef HETEROGENEOUS_PARALLEL_FOR_IGRAPHBODY_H
#define HETEROGENEOUS_PARALLEL_FOR_IGRAPHBODY_H
#include <type_traits>
#include "../../lib/Interfaces/Bodies/IBody.cpp"
/*****************************************************************************
 * Interface IBody
 * Body base class for Graph Schedulers implementations
 * **************************************************************************/
template<typename NDRange, typename Tindex, typename ...Args>
class IGraphBody : IBody {
public:
    virtual int GetVsize() = 0;

    virtual void OperatorCPU(int begin, int end) = 0;

    virtual std::tuple<Tindex, Args ...> GetGPUArgs(Tindex indexes) = 0;

    virtual NDRange GetNDRange() = 0;
};

std::false_type is_graph_implementation(...);
template <typename ...Args>
std::true_type is_graph_implementation(IGraphBody<Args...> const volatile&);
template <typename TBody>
using is_graph_body = decltype(is_graph_implementation(std::declval<TBody&>()));


#endif //HETEROGENEOUS_PARALLEL_FOR_IGRAPHBODY_H
