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

using namespace gaia::common;
using namespace gaia::db;

static constexpr char c_data_dir_command_flag[] = "--data-dir";
static constexpr char c_disable_persistence_flag[] = "--disable-persistence";
static constexpr char c_disable_persistence_after_recovery_flag[] = "--disable-persistence-after-recovery";
static constexpr char c_reinitialize_persistent_store_flag[] = "--reinitialize-persistent-store";

static void usage()
{
    std::cerr
        << "\nCopyright (c) Gaia Platform LLC\n\n"
        << "Usage: gaia_db_server ["
        << c_disable_persistence_flag
        << " | "
        << c_disable_persistence_after_recovery_flag
        << " | "
        << c_reinitialize_persistent_store_flag
        << "] \n"
        << "                      ["
        << c_data_dir_command_flag
        << " <data dir>]\n"
        << "                      ["
        << gaia::common::c_conf_file_flag
        << " <config file path>]"
        << std::endl;
    std::exit(1);
}

// Replaces the tilde '~' with the full user home path.
static void expand_home_path(std::string& path)
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

static server_conf_t process_command_line(int argc, char* argv[])
{
    std::set<std::string> used_flags;
    const char* conf_file_path = nullptr;

    server_conf_t::persistence_mode_t persistence_mode{server_conf_t::persistence_mode_t::e_default};
    std::string data_dir;

    for (int i = 1; i < argc; ++i)
    {
        used_flags.insert(argv[i]);
        if (strcmp(argv[i], c_disable_persistence_flag) == 0)
        {
            persistence_mode = server_conf_t::persistence_mode_t::e_disabled;
        }
        else if (strcmp(argv[i], c_disable_persistence_after_recovery_flag) == 0)
        {
            persistence_mode = server_conf_t::persistence_mode_t::e_disabled_after_recovery;
        }
        else if (strcmp(argv[i], c_reinitialize_persistent_store_flag) == 0)
        {
            persistence_mode = server_conf_t::persistence_mode_t::e_reinitialized_on_startup;
        }
        else if ((strcmp(argv[i], c_data_dir_command_flag) == 0) && (i + 1 < argc))
        {
            //            gaia::db::persistent_store_manager::s_data_dir_path = argv[++i];
            data_dir = argv[++i];
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
    if (data_dir.empty() && (persistence_mode != server_conf_t::persistence_mode_t::e_disabled))
    {
        // Since there is no --data-dir parameter, locate it from the configuration.
        gaia_configuration_file = gaia::common::get_conf_file_path(conf_file_path, c_default_conf_file_name);
        if (!gaia_configuration_file.empty())
        {
            std::shared_ptr<cpptoml::table> root_config = cpptoml::parse_file(gaia_configuration_file);
            auto data_dir_string = root_config->get_qualified_as<std::string>(c_data_dir_string_key);
            // There are two cases of s_data_dir "missing" from the configuration file. First, if the
            // s_data_dir key doesn't exist. Second, if the s_data_dir key exists, but the value doesn't
            // exist. Either way, we don't care, and the effect is that we obtain no data_dir_string
            // from the configuration file.
            if (data_dir_string)
            {
                // The 's_data_dir' key exists. Make sure it is non-empty.
                if (!data_dir_string->empty())
                {
                    data_dir = *data_dir_string;
                }
            }
        }
    }

    // In case anyone can see this...
    if (persistence_mode == server_conf_t::persistence_mode_t::e_disabled)
    {
        std::cerr
            << "Persistence is disabled." << std::endl;
    }

    if (persistence_mode == server_conf_t::persistence_mode_t::e_disabled_after_recovery)
    {
        std::cerr
            << "Persistence is disabled after recovery." << std::endl;
    }

    if (persistence_mode == server_conf_t::persistence_mode_t::e_reinitialized_on_startup)
    {
        std::cerr
            << "Persistence is reinitialized on startup." << std::endl;
    }

    if (persistence_mode != server_conf_t::persistence_mode_t::e_disabled)
    {
        if (data_dir.empty())
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

        if (data_dir.empty())
        {
            std::cerr
                << "Unable to locate a database directory.\n"
                << "Use the '--data-dir' flag, or the configuration file "
                << "to identify the location of the database."
                << std::endl;
            usage();
        }

        expand_home_path(data_dir);
        // The RocksDB Open() can create one more subdirectory (e.g. /var/lib/gaia/db can be created if /var/lib/gaia
        // already exists). To enable the use of longer paths, we will do a mkdir of the entire path.
        // If there is a failure to open the database after this, there is nothing more we can do about it
        // here. Let the normal error processing catch and handle the problem.
        // Note that the mkdir command may fail because the directory already exists, or because it doesn't
        // have permission.
        std::string mkdir_command = "mkdir -p " + data_dir;
        ::system(mkdir_command.c_str());

        std::cerr
            << "Database directory is '" << data_dir << "'." << std::endl;
    }

    for (const auto& flag_pair : std::initializer_list<std::pair<std::string, std::string>>{
             // Disable persistence flag is mutually exclusive with specifying data directory for persistence.
             {c_disable_persistence_flag, c_data_dir_command_flag},
             // The three persistence flags that are mutually exclusive with each other.
             {c_disable_persistence_flag, c_disable_persistence_after_recovery_flag},
             {c_disable_persistence_flag, c_reinitialize_persistent_store_flag},
             {c_disable_persistence_after_recovery_flag, c_reinitialize_persistent_store_flag}})
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

    return server_conf_t{persistence_mode, data_dir};
}

int main(int argc, char* argv[])
{
    auto server_conf = process_command_line(argc, argv);

    gaia::db::server_t::run(server_conf);
}
