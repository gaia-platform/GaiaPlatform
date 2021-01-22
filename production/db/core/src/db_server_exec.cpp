/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <initializer_list>
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

    {
        std::set<std::string> used_flags;
        for (int i = 1; i < argc; ++i)
        {
            used_flags.insert(argv[i]);
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
            else if ((strcmp(argv[i], gaia::db::persistent_store_manager::c_data_dir_command_flag) == 0) && (i + 1 < argc))
            {
                gaia::db::persistent_store_manager::s_data_dir_path = argv[++i];
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
        for (const auto& flag_pair : std::initializer_list<std::pair<std::string, std::string>>{
                 // Disable persistence flag is mutually exclusive with specifying data directory for persistence.
                 {gaia::db::server::c_disable_persistence_flag, gaia::db::persistent_store_manager::c_data_dir_command_flag},
                 // The three persistence flags that are mutually exclusive with each other.
                 {gaia::db::server::c_disable_persistence_flag, gaia::db::server::c_disable_persistence_after_recovery_flag},
                 {gaia::db::server::c_disable_persistence_flag, gaia::db::server::c_reinitialize_persistent_store_flag},
                 {gaia::db::server::c_disable_persistence_after_recovery_flag, gaia::db::server::c_reinitialize_persistent_store_flag}})
        {
            if ((used_flags.find(flag_pair.first) != used_flags.end()) && (used_flags.find(flag_pair.second) != used_flags.end()))
            {
                std::cerr
                    << std::endl
                    << flag_pair.first
                    << " and "
                    << flag_pair.second
                    << " flags are mutually exclusive."
                    << std::endl;
                usage();
            }
        }
    }

    gaia::db::server::run(persistence_mode);
}
