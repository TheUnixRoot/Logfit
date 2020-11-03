//
// Created by juanp on 2/11/20.
//

#ifndef HETEROGENEOUS_PARALLEL_FOR_IPIPELINEBODY_H
#define HETEROGENEOUS_PARALLEL_FOR_IPIPELINEBODY_H
#include <type_traits>
#include "../../lib/Interfaces/Bodies/IBody.cpp"
/*****************************************************************************
 * Interface IPipelineBody
 * Body base class for Pipeline Schedulers implementations
 * **************************************************************************/
template<typename NDRange, typename Tindex, typename ...Args>
class IPipelineBody : IBody {
public:
    virtual int GetVsize() = 0;

    virtual void OperatorCPU(int begin, int end) = 0;

    virtual std::tuple<Tindex, Args ...> GetGPUArgs(Tindex indexes) = 0;

    virtual NDRange GetNDRange() = 0;

    virtual void sendObjectToGPU(int begin, int end, void *null_ptr) = 0;

    virtual void OperatorGPU(int begin, int end, void *null_ptr) = 0;

    virtual void getBackObjectFromGPU(int begin, int end, void *null_ptr) = 0;

};

std::false_type is_pipeline_implementation(...);
template <typename ...Args>
std::true_type is_pipeline_implementation(IPipelineBody<Args...> const volatile&);
template <typename TBody>
using is_pipeline_body = decltype(is_pipeline_implementation(std::declval<TBody&>()));


#endif //HETEROGENEOUS_PARALLEL_FOR_IPIPELINEBODY_H
