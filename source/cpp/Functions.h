//
// Created by juanp on 6/25/18.
//

#ifndef BARNESLOGFIT_FUNCTIONS_H
#define BARNESLOGFIT_FUNCTIONS_H

#include "DataStructures.h"
#include "tbb/tick_count.h"

using namespace tbb;

namespace Functions {

    template <typename T>
    void initialize(dataStorage * myData, Params p, T *body) {
        std::cout << "Done.";

        flow::graph g;
        tbb::flow::function_node<tbb::flow::tuple<int, int>> nodo(g, flow::unlimited, [](flow::tuple<int, int> data){ std::cout << "hola";});
//
//        myData -> cpuNode = new tbb::flow::function_node<tbb::flow::tuple<int, int>>(*(myData -> graph), flow::unlimited, [body](flow::tuple<int, int> data){
//            tick_count start = tick_count::now();
//            //body->OperatorCPU(flow::get<0>(data), flow::get<1>(data));
//            tick_count stop = tick_count::now();
//
//            std::cout << start.resolution() << " .. .. " << stop.resolution() << std::endl;
//        });
        std::cout << "Done.";
    }
}

#endif //BARNESLOGFIT_FUNCTIONS_H
