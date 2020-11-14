//
// Created by juanp on 7/11/20.
//

#ifndef BARNESLOGFIT_TRIADDPIPELINEBODYTEST_H
#define BARNESLOGFIT_TRIADDPIPELINEBODYTEST_H
#include "../Bodies/TriaddPipelineBody.h"
#include <utils/Utils.h>
class TriaddPipelineBodyTest {
public:
    TriaddPipelineBodyTest() {}

    bool runTest(TriaddPipelineBody &body) {
        auto A = body.Ahost;
        auto B = body.Bhost;
        auto C = body.Chost;
        bool result{true};
        for (int i = 0; i < body.vsize; ++i) {
            if (C[i] != A[i] + B[i]) {
                cout << CONSOLE_RED << i << " : C = " << C[i]
                     << " /=/ A (" << A[i] << ") + B (" << B[i] << ") =" << A[i] + B[i] << CONSOLE_WHITE << endl;
                result = false;
            }
        }
        return result;
    }

};
#endif //BARNESLOGFIT_TRIADDPIPELINEBODYTEST_H
