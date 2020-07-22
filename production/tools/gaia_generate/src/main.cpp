/////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////
#include "gaia_catalog.hpp"
#include <iostream>
#include <memory>
#include <vector>
#include <set>

using namespace gaia::catalog;
using namespace std;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: gaia_generate database_name\n");
        exit(1);
    }

    begin_session();

    auto code_lines = gaia_generate(argv[1]);
    cout << code_lines << endl;

    end_session();
}
