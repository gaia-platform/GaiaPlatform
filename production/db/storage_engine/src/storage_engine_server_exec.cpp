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
        if (argc != 2)
        {
            throw new std::invalid_argument("Unexpected number of arguments!");
        }

        // The sole argument must be the `--disable-persistence` flag.
        if (strcmp(argv[1], gaia::db::server::c_disable_persistence_flag))
        {
            throw new std::invalid_argument(
                "A different argument than the expected `--disable-persistence` was found!");
        }

        disable_persistence = true;
    }

    gaia::db::server::run(disable_persistence);
}
