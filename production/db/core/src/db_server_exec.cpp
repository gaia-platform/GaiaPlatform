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
        << "\nCopyright (c) Gaia Platform LLC\n\n"
        << "Usage: gaia_db_server ["
        << gaia::db::server::c_disable_persistence_flag
        << " | "
        << gaia::db::server::c_disable_persistence_after_recovery_flag
        << " | "
        << gaia::db::server::c_reinitialize_persistent_store_flag
        << "] \n"
        << "                      ["
        << gaia::db::persistent_store_manager::c_data_dir_command_flag
        << " <data dir>]\n"
        << "                      ["
        << gaia::common::c_conf_file_flag
        << " <config file path>]"
        << std::endl;
    std::exit(1);
}

static void expand_user_path(std::string& path)
{
    char* home;

    if (path.compare(0, 2, "~/") != 0)
    {
        return;
    }

    home = getenv("HOME");
    if (!home)
    {
        std::cerr
            << "Unable to expand directory path '"
            << path
            << "'. No $HOME environment variable found."
            << std::endl;
        usage();
    }

    path = home + path.substr(1);
}

static server::persistence_mode_t process_command_line(int argc, char* argv[])
{
    std::set<std::string> used_flags;
    bool found_data_dir = false;
    const char* conf_file_path = nullptr;

    server::persistence_mode_t persistence_mode{server::persistence_mode_t::e_default};

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
            conf_file_path = argv[++i];
        }
        else
        {
            std::cerr
                << "\nUnrecognized argument, '"
                << argv[i]
                << "'."
                << std::endl;
            usage();
        }
    }

    std::string gaia_configuration_file;
    if (!found_data_dir && (persistence_mode != server::persistence_mode_t::e_disabled))
    {
        // Since there is no --data-dir parameter, locate it from the configuration.
        gaia_configuration_file = gaia::common::get_conf_file_path(conf_file_path, c_default_conf_file_name);
        if (!gaia_configuration_file.empty())
        {
            std::shared_ptr<cpptoml::table> root_config = cpptoml::parse_file(gaia_configuration_file);
            auto data_dir_string = root_config->get_qualified_as<std::string>(c_data_dir_string_key);
            if (data_dir_string)
            {
                // The 'data_dir' key exists. Make sure it is non-empty.
                if (!data_dir_string->empty())
                {
                    gaia::db::persistent_store_manager::s_data_dir_path = *data_dir_string;
                }
            }
        }
        else
        {
            std::cerr
                << "Configuration file could not be found." << std::endl;
        }
    }

    // In case anyone can see this...
    if (persistence_mode == server::persistence_mode_t::e_disabled)
    {
        std::cerr
            << "Persistence is disabled." << std::endl;
    }
    if (persistence_mode == server::persistence_mode_t::e_disabled_after_recovery)
    {
        std::cerr
            << "Persistence is disabled after recovery." << std::endl;
    }
    if (persistence_mode == server::persistence_mode_t::e_reinitialized_on_startup)
    {
        std::cerr
            << "Persistence is reinitialized on startup." << std::endl;
    }
    if (persistence_mode != server::persistence_mode_t::e_disabled)
    {
        if (!found_data_dir)
        {
            if (!gaia_configuration_file.empty())
            {
                std::cerr
                    << "Configuration file found at '"
                    << gaia_configuration_file
                    << "'." << std::endl;
            }
            else
            {
                std::cerr
                    << "No configuration file found." << std::endl;
            }
        }
        if (gaia::db::persistent_store_manager::s_data_dir_path.empty())
        {
            std::cerr
                << "Unable to locate a database directory.\n"
                << "Use the '--data-dir' flag, or the configuration file "
                << "to identify the location of the database."
                << std::endl;
            usage();
        }

        expand_user_path(gaia::db::persistent_store_manager::s_data_dir_path);
        std::cerr
            << "Database directory is '"
            << gaia::db::persistent_store_manager::s_data_dir_path
            << "'." << std::endl;
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
                << "\n'"
                << flag_pair.first
                << "' and '"
                << flag_pair.second
                << "' flags are mutually exclusive."
                << std::endl;
            usage();
        }
    }

    return persistence_mode;
}

int main(int argc, char* argv[])
{
    auto persistence_mode = process_command_line(argc, argv);

    gaia::db::server::run(persistence_mode);
}
