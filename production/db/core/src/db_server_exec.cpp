/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <string>

#include "db_server.hpp"

static void usage()
{
    std::cerr
        << std::endl
        << "Copyright (c) Gaia Platform LLC"
        << std::endl
        << std::endl
        << "Usage: gaia_db_server ["
        << gaia::db::server::c_disable_persistence_flag
        << " | "
        << gaia::db::server::c_disable_persistence_after_recovery_flag
        << " | "
        << gaia::db::server::c_reinitialize_persistent_store_flag
        << "] "
        << "["
        << gaia::db::persistent_store_manager::c_data_dir_command_flag
        << " <data dir>]"
        << std::endl;
    std::exit(1);
}

int main(int argc, char* argv[])
{
    server::persistence_mode_t persistence_mode{server::persistence_mode_t::e_default};

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], gaia::db::server::c_disable_persistence_flag) == 0)
        {
            persistence_mode = server::persistence_mode_t::e_disabled;
        }
        else if (strcmp(argv[i], gaia::db::server::c_disable_persistence_after_recovery_flag) == 0)
        {
            persistence_mode = server::persistence_mode_t::e_disabled_after_recovery;
        }
        else if (strcmp(argv[i], gaia::db::server::c_reinitialize_persistent_store_flag) == 0)
        {
            persistence_mode = server::persistence_mode_t::e_reinitialized_on_startup;
        }
        else if (strcmp(argv[i], gaia::db::persistent_store_manager::c_data_dir_command_flag) == 0)
        {
            ++i;
            gaia::db::persistent_store_manager::s_data_dir_path = argv[i];
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

    gaia::db::server::run(persistence_mode);
}
