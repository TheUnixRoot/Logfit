#ifndef BARNESLOGFIT_IGRAPHBODY_H
#define BARNESLOGFIT_IGRAPHBODY_H
#include "../../lib/Interfaces/Bodies/IBody.cpp"
/*****************************************************************************
 * Interface IBody
 * Body base class for Graph Schedulers implementations
 * **************************************************************************/
template<typename NDRange, typename Tindex, typename ...Args>
class IGraphBody : IBody<NDRange, Tindex, Args...> {
public:
    virtual int GetVsize() = 0;

    virtual void OperatorCPU(int begin, int end) = 0;

    virtual std::tuple<Tindex, Args ...> GetGPUArgs(Tindex indexes) = 0;

    virtual NDRange GetNDRange() = 0;
};

std::false_type is_graph_implementation(...);
template <typename ...T>
std::true_type is_graph_implementation(IGraphBody<T...> const volatile&);
template <typename T>
using is_graph_body = decltype(is_graph_implementation(std::declval<T&>()));


#endif //BARNESLOGFIT_IGRAPHBODY_H
