/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <initializer_list>
#include <iostream>
#include <string>

#include "gaia/system.hpp"

#include "gaia_internal/common/config.hpp"

#include "cpptoml.h"
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
        << std::endl
        << "                      ["
        << gaia::db::persistent_store_manager::c_data_dir_command_flag
        << " <data dir>]"
        << std::endl
        << "                      ["
        << gaia::common::c_conf_file_flag
        << " <config file path>]"
        << std::endl;
    std::exit(1);
}

int main(int argc, char* argv[])
{
    server::persistence_mode_t persistence_mode{server::persistence_mode_t::e_default};

    {
        std::set<std::string> used_flags;
        bool found_data_dir = false;
        const char* conf_file = nullptr;

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
                found_data_dir = true;
            }
            else if ((strcmp(argv[i], gaia::common::c_conf_file_flag) == 0) && (i + 1 < argc))
            {
                conf_file = argv[++i];
            }
            else
            {
                std::cerr
                    << std::endl
                    << "Unrecognized argument, \""
                    << argv[i]
                    << "\"."
                    << std::endl;
                usage();
            }
        }
        if (!found_data_dir)
        {
            // Since there is no --data-dir parameter, locate it from the configuration.
            static const char* c_data_dir_string_key = "Database.data_dir";

            std::string c_gaia_conf = gaia::common::get_conf_file(conf_file, c_default_conf_file_name);
            std::shared_ptr<cpptoml::table> root_config = cpptoml::parse_file(c_gaia_conf);
            auto data_dir_string = root_config->get_qualified_as<std::string>(c_data_dir_string_key);
            // If there is no data_dir string in the configuration file, it falls to "/var/lib/gaia/db".
            if (!data_dir_string->empty())
            {
                gaia::db::persistent_store_manager::s_data_dir_path = *data_dir_string;
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
                    << "\""
                    << flag_pair.first
                    << "\" and \""
                    << flag_pair.second
                    << "\" flags are mutually exclusive."
                    << std::endl;
                usage();
            }
        }
    }

    gaia::db::server::run(persistence_mode);
}
