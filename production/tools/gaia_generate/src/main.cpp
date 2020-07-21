/////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////
#include "catalog_gaia_generated.h"
#include "catalog_manager.hpp"
#include "code_writer.hpp"
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

    auto code_lines = gaia_generate(argv[1]);
    cout << code_lines << endl;
}
