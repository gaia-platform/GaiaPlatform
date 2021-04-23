/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <initializer_list>
#include <iostream>
#include <string>

#include "spdlog/fmt/fmt.h"

#include "gaia/logger.hpp"

#include "gaia_internal/common/config.hpp"
#include "gaia_internal/common/timer.hpp"

#include "cpptoml.h"
#include "db_server.hpp"

using namespace gaia::common;
using namespace gaia::db;

using gaia_timer_t = gaia::common::timer_t;

static constexpr char c_data_dir_command_flag[] = "--data-dir";
static constexpr char c_session_command_flag[] = "--session";
static constexpr char c_disable_persistence_flag[] = "--disable-persistence";
static constexpr char c_disable_persistence_after_recovery_flag[] = "--disable-persistence-after-recovery";
static constexpr char c_reinitialize_persistent_store_flag[] = "--reinitialize-persistent-store";
static constexpr char c_conf_file_flag[] = "--configuration-file-path";

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
        << c_conf_file_flag
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

class lazy_gaia_conf
{
public:
    explicit lazy_gaia_conf(std::string config_path)
        : m_config_path(std::move(config_path))
    {
    }

    template <class T_value>
    std::optional<T_value> get_value(const char* key)
    {
        auto root_config = get_root_config();

        if (!root_config)
        {
            return std::nullopt;
        }

        auto val = root_config->template get_qualified_as<T_value>(key);

        if (val)
        {
            return {*val};
        }

        return std::nullopt;
    }

private:
    std::string m_config_path;
    std::shared_ptr<cpptoml::table> m_root_config = nullptr;
    bool m_is_initialized = false;

private:
    std::shared_ptr<cpptoml::table> get_root_config()
    {
        if (m_is_initialized)
        {
            return m_root_config;
        }

        std::string gaia_configuration_file = gaia::common::get_conf_file_path(
            m_config_path.c_str(), c_default_conf_file_name);

        m_is_initialized = true;

        if (gaia_configuration_file.empty())
        {
            std::cerr
                << "No configuration file found." << std::endl;

            return nullptr;
        }
        else
        {
            std::cerr
                << "Configuration file found at '"
                << gaia_configuration_file
                << "'." << std::endl;
        }

        m_root_config = cpptoml::parse_file(gaia_configuration_file);
        return m_root_config;
    }
};

static server_conf_t process_command_line(int argc, char* argv[])
{
    std::set<std::string> used_flags;

    server_conf_t::persistence_mode_t persistence_mode{server_conf_t::c_default_persistence_mode};
    std::string instance_name;
    std::string data_dir;
    std::string conf_file_path;

    // TODO argument parsing needs refactoring. ATM it is unclear:
    //  - what arguments are optional/mandatory
    //  - which ones can be read from a configuration file
    //  - whether the config file is mandatory in the first place.
    //  These things are eventually clear if you read the entire code, it should not be like that.
    //  https://www.boost.org/doc/libs/1_36_0/doc/html/program_options/tutorial.html#id3451469

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
            data_dir = argv[++i];
        }
        else if ((strcmp(argv[i], c_conf_file_flag) == 0) && (i + 1 < argc))
        {
            conf_file_path = argv[++i];
        }
        else if ((strcmp(argv[i], c_session_command_flag) == 0) && (i + 1 < argc))
        {
            instance_name = argv[++i];
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

    lazy_gaia_conf gaia_conf{conf_file_path};

    if (data_dir.empty() && (persistence_mode != server_conf_t::persistence_mode_t::e_disabled))
    {
        auto data_dir_string = gaia_conf.get_value<std::string>(c_data_dir_string_key);
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

    if (instance_name.empty())
    {
        auto instance_name_string = gaia_conf.get_value<std::string>(c_data_dir_string_key);

        if (instance_name_string)
        {
            instance_name = *instance_name_string;
        }

        if (instance_name.empty())
        {
            instance_name = gaia::db::c_default_instance_name;
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

    std::cerr
        << "Database session name is '" << instance_name << "'." << std::endl;

    if (persistence_mode != server_conf_t::persistence_mode_t::e_disabled)
    {
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

    return server_conf_t{persistence_mode, instance_name, data_dir};
}

#include <strstream>

int main(int argc, char* argv[])
{
    //    auto server_conf = process_command_line(argc, argv);

    //    gaia::db::server_t::run(server_conf);

    std::string str1 = "Hello World";
    const char* str2 = "Ciao Mondo";

    int num_iter = 10000;

    int sizes[num_iter];

    std::chrono::steady_clock::time_point start;

    std::cout << "Num Iter: " << num_iter << std::endl;

    start = gaia_timer_t::get_time_point();
    for (int i = 0; i < num_iter; i++)
    {
        std::string final = fmt::format("{}_{}_{}", str1, str2, i);
        sizes[i] = final.size();
    }
    gaia_timer_t::log_duration(start, "fmt");

    start = gaia_timer_t::get_time_point();
    for (int i = 0; i < num_iter; i++)
    {
        std::ostringstream ss;
        ss << str1 << "_" << str2 << "_" << i;
        std::string final = ss.str();
        sizes[i] = final.size();
    }
    gaia_timer_t::log_duration(start, "ostringstream");

    start = gaia_timer_t::get_time_point();
    for (int i = 0; i < num_iter; i++)
    {
        std::stringstream ss;
        ss << str1 << "_" << str2 << "_" << i;
        std::string final = ss.str();
        sizes[i] = final.size();
    }
    gaia_timer_t::log_duration(start, "stringstream");

    start = gaia_timer_t::get_time_point();
    for (int i = 0; i < num_iter; i++)
    {
        std::string final = str1 + "_" + str2 + "_" + std::to_string(i);
        sizes[i] = final.size();
    }
    gaia_timer_t::log_duration(start, "string concat");
}
