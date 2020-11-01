//
// Created by juanp on 26/10/20.
//

#ifndef HETEROGENEOUS_PARALLEL_FOR_UTILS_H
#define HETEROGENEOUS_PARALLEL_FOR_UTILS_H

#define CONSOLE_YELLOW "\033[0;33m"
#define CONSOLE_WHITE "\033[0m"
#define CONSOLE_RED ""


using Params = struct _params {
    unsigned int numcpus;
    unsigned int numgpus;
    char benchName[256];
    char kernelName[50];
    char openclFile[256];
    char inputData[256];
    float ratioG;
};


enum ProcessorUnit : int;
namespace dataStructures {

    struct GpuDeviceSelector;

    template<std::size_t I = 0, typename ...Tg>
    inline typename std::enable_if<I == sizeof...(Tg), void>::type
    try_put(tbb::flow::opencl_node<tbb::flow::tuple<Tg...>> *node, tbb::flow::tuple<Tg...> &args);

    template<std::size_t I = 0, typename ...Tg>
    inline typename std::enable_if<I < sizeof...(Tg), void>::type
    try_put(tbb::flow::opencl_node<tbb::flow::tuple<Tg...>> *node, tbb::flow::tuple<Tg...> &args);

}

namespace ConsoleUtils {
    static constexpr unsigned int str2int(const char *str, int h = 0) {
        return !str[h] ? 5381 : (str2int(str, h + 1) + 33) ^ str[h];
    }

    static std::string validateArgs(Params p);

    static Params parseArgs(int argc, char *argv[]);

    static void saveResultsForBench(Params p, double runtime);

}
#endif //HETEROGENEOUS_PARALLEL_FOR_UTILS_H
