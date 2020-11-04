/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <string>

#include "storage_engine_server.hpp"

int main(int argc, char* argv[])
{
    bool disable_persistence = false;
    if (argc > 1)
    {
        // We currently accept only one argument.
        retail_assert(argc == 2);
        // The sole argument must be the `--disable-persistence` flag.
        retail_assert(argv[1] == std::string(gaia::db::server::c_disable_persistence_flag));
        disable_persistence = true;
    }
    gaia::db::server::run(disable_persistence);
}
