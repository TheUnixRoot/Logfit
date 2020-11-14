//
// Created by juanp on 4/11/20.
//


#ifndef BARNESLOGFIT_TRIADDONEAPIBODYTEST_H
#define BARNESLOGFIT_TRIADDONEAPIBODYTEST_H
#include "../Bodies/TriaddOneApiBody.h"

class TriaddOneApiBodyTest {
public:
    TriaddOneApiBodyTest() {}

    bool runTest(TriaddOneApiBody &body) {
        auto A = body.A;
        auto B = body.B;
        auto C = body.C;
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

#endif //BARNESLOGFIT_TRIADDONEAPIBODYTEST_H