/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <string>

#include "se_server.hpp"

static void usage()
{
    std::cerr
        << std::endl
        << "Copyright (c) Gaia Platform LLC"
        << std::endl
        << std::endl
        << "Usage: gaia_se_server ["
        << gaia::db::server::c_disable_persistence_flag
        << " | "
        << gaia::db::server::c_reinitialize_persistent_store_flag
        << "]"
        << std::endl
        << std::endl;
    std::exit(1);
}

int main(int argc, char* argv[])
{
    bool disable_persistence = false;
    bool reinitialize_persistent_store = false;

    // We currently accept only one argument.
    if (argc > 2)
    {
        std::cerr
            << std::endl
            << "Too many arguments (maximum 1)."
            << std::endl;
        usage();
    }

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], gaia::db::server::c_disable_persistence_flag) == 0)
        {
            disable_persistence = true;
        }
        else if (strcmp(argv[i], gaia::db::server::c_reinitialize_persistent_store_flag) == 0)
        {
            reinitialize_persistent_store = true;
        }
        else
        {
            std::cerr
                << std::endl
                << "Unrecognized argument."
                << std::endl;
            usage();
        }
    }

    gaia::db::server::run(disable_persistence, reinitialize_persistent_store);
}
