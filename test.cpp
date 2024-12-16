//
// Created by theo on 12/15/24.
//

#include "numstore/testing.hpp"

int
main() {
    for (int i = 0; i < test_count; ++i) {
        tests[i]();
    }
    return 0;
}
