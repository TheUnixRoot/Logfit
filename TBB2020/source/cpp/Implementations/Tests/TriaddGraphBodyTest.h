//
// Created by juanp on 4/11/20.
//

#ifndef BARNESLOGFIT_TRIADDGRAPHBODYTEST_H
#define BARNESLOGFIT_TRIADDGRAPHBODYTEST_H
#include "../Bodies/TriaddGraphBody.h"
#include <utils/Utils.h>
class TriaddGraphBodyTest {
public:
    TriaddGraphBodyTest() {}

    bool runTest(TriaddGraphBody &body) {
        auto A = body.Ahost;
        auto B = body.Bhost;
        auto C = body.Chost;
        for (int i = 0; i < body.vsize; ++i) {
            if (C[i] != A[i] + B[i]) {
                cout << CONSOLE_RED << i << " :C=" << C[i]
                    << " A+B=" << A[i] + B[i] << CONSOLE_WHITE << endl;
                return false;
            }
        }
        return true;
    }

};
#endif //BARNESLOGFIT_TRIADDGRAPHBODYTEST_H
