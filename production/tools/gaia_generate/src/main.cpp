/////////////////////////////////////////////
//// Copyright (c) Gaia Platform LLC
//// All rights reserved.
///////////////////////////////////////////////

#include <iostream>
#include <memory>
#include <set>
#include <vector>

#include "gaia/db/catalog.hpp"
#include "gaia/db/gaia_db.hpp"

using namespace gaia::catalog;
using namespace std;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: gaia_generate database_name\n");
        exit(1);
    }

    gaia::db::begin_session();

    try
    {
        auto code_lines = gaia_generate(argv[1]);
        cout << code_lines << endl;
    }
    catch (gaia::common::gaia_exception& e)
    {
        cerr << "gaia_generate failed: " << e.what() << endl;
    }

    gaia::db::end_session();
}
