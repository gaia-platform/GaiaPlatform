/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <cstdlib>

#include <initializer_list>
#include <iostream>
#include <string>

#include "gaia_internal/common/config.hpp"
#include "gaia_internal/common/gaia_version.hpp"

#include "cpptoml.h"
#include "db_server.hpp"

using namespace gaia::common;
using namespace gaia::db;

static constexpr char c_help_param[] = "--help";
static constexpr char c_version_param[] = "--version";
static constexpr char c_data_dir_command_param[] = "--data-dir";
static constexpr char c_instance_name_command_param[] = "--instance-name";
static constexpr char c_config_file_param[] = "--config-file";
static constexpr char c_persistence_param[] = "--persistence";
static constexpr char c_reset_data_store_param[] = "--reset-data-store";
static constexpr char c_skip_catalog_integrity_param[] = "--skip-catalog-integrity-checks";

static constexpr char c_persistence_enabled_mode[] = "enabled";
static constexpr char c_persistence_disabled_mode[] = "disabled";
static constexpr char c_persistence_disabled_after_recovery_mode[] = "disabled-after-recovery";

static void usage()
{
    std::cerr
        << "OVERVIEW: Gaia Database Server. Used by Gaia applications to store data.\n"
           "USAGE: gaia_db_server [options]\n"
           "\n"
           "OPTIONS:\n"
           "  --persistence <mode>        Specifies the database persistence mode.\n"
           "                              If not specified, the default mode is enabled.\n"
           "                              The data location is specified with --data-dir.\n"
           "                              - <enabled>: Persist data [default].\n"
           "                              - <disabled>: Do not persist any data.\n"
           "                              - <disabled-after-recovery>: Load data from the datastore and\n"
           "                                disable persistence.\n"
           "  --data-dir <data_dir>       Specifies the directory in which to create the data store.\n"
           "  --reset-data-store          Deletes the data in the data store.\n"
#ifdef DEBUG
           "  --instance-name <db_instance_name>   Specify the database instance name.\n"
           "                                       If not specified, will use "
        << c_default_instance_name
        << ".\n"
           "  --skip-catalog-integrity-checks      ????"
#endif
           "  --config-file <file>        Specifies the location of the Gaia configuration file to use.\n"
           "  --help                      Print help information.\n"
           "  --version                   Print version information.\n";
}

static void version()
{
    std::cerr << "Gaia DB Server " << gaia_full_version() << "\nCopyright (c) Gaia Platform LLC\n";
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
        std::exit(1);
    }

    path = home + path.substr(1);
}

template <typename T_type>
T_type lexical_cast(const std::string& str)
{
    T_type var;
    std::istringstream iss;
    iss.str(str);
    iss >> var;
    return var;
}

/**
 * Allow getting a config value from multiple sources: command line, environment variable, and config file.
 *
 * The value is selected from the first available source, in order of importance:
 * 1. Command line arg.
 * 2. Environment variable.
 * 3. Configuration file.
 */
class gaia_config_fallback_t
{
public:
    explicit gaia_config_fallback_t(std::string config_path)
        : m_config_path(std::move(config_path)){};

    gaia_config_fallback_t() = default;

    template <class T_value>
    std::optional<T_value> get_config_file_value(const char* key)
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

    template <class T_value>
    std::optional<T_value> get_env_value(const char* env_name)
    {
        char* value = std::getenv(env_name);

        if (value)
        {
            return {lexical_cast<T_value>(value)};
        }

        return std::nullopt;
    }

    template <class T_value>
    std::optional<T_value> get_value(std::string param_value, const char* env_name, const char* config_file_key)
    {
        if (!param_value.empty())
        {
            return param_value;
        }

        auto env_value = get_env_value<T_value>(env_name);

        if (env_value)
        {
            return env_value;
        }

        return get_config_file_value<T_value>(config_file_key);
    }

    template <class T_value>
    T_value get_value(std::string param_value, const char* env_name, const char* config_file_key, T_value default_value)
    {
        auto value = get_value<T_value>(param_value, env_name, config_file_key);

        if (value)
        {
            return *value;
        }

        return default_value;
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

static server_config_t::persistence_mode_t parse_persistence_mode(std::string persistence_mode)
{
    if (persistence_mode == c_persistence_enabled_mode)
    {
        return server_config_t::persistence_mode_t::e_enabled;
    }
    else if (persistence_mode == c_persistence_disabled_mode)
    {
        return server_config_t::persistence_mode_t::e_disabled;
    }
    else if (persistence_mode == c_persistence_disabled_after_recovery_mode)
    {
        return server_config_t::persistence_mode_t::e_disabled_after_recovery;
    }
    else
    {
        std::cerr
            << "\nUnrecognized persistence mode: '"
            << persistence_mode
            << "'."
            << std::endl;
        usage();
        std::exit(1);
    }
}

static server_config_t process_command_line(int argc, char* argv[])
{
    std::set<std::string> used_params;

    server_config_t::persistence_mode_t persistence_mode{server_config_t::c_default_persistence_mode};
    std::string instance_name;
    std::string data_dir;
    std::string conf_file_path;
    bool testing = false;

    // TODO argument parsing needs refactoring. ATM it is unclear:
    //  - what arguments are optional/mandatory
    //  - which ones can be read from a configuration file
    //  - whether the config file is mandatory in the first place.
    //  These things are eventually clear if you read the entire code, it should not be like that.
    //  https://www.boost.org/doc/libs/1_36_0/doc/html/program_options/tutorial.html#id3451469

    for (int i = 1; i < argc; ++i)
    {
        used_params.insert(argv[i]);
        if (strcmp(argv[i], c_help_param) == 0)
        {
            usage();
            std::exit(0);
        }
        if (strcmp(argv[i], c_version_param) == 0)
        {
            version();
            std::exit(0);
        }
        else if (strcmp(argv[i], c_persistence_param) == 0)
        {
            persistence_mode = parse_persistence_mode(argv[++i]);
        }
        else if (strcmp(argv[i], c_reset_data_store_param) == 0)
        {
            persistence_mode = server_config_t::persistence_mode_t::e_reinitialized_on_startup;
        }
        else if ((strcmp(argv[i], c_data_dir_command_param) == 0) && (i + 1 < argc))
        {
            data_dir = argv[++i];
        }
        else if ((strcmp(argv[i], c_config_file_param) == 0) && (i + 1 < argc))
        {
            conf_file_path = argv[++i];
        }
        else if ((strcmp(argv[i], c_instance_name_command_param) == 0) && (i + 1 < argc))
        {
            instance_name = argv[++i];
        }
        else if ((strcmp(argv[i], c_skip_catalog_integrity_param) == 0))
        {
            testing = true;
        }
        else
        {
            std::cerr
                << "\nUnrecognized argument, '"
                << argv[i]
                << "'."
                << std::endl;
            usage();
            std::exit(1);
        }
    }

    std::cerr << "Starting " << c_db_server_name << "..." << std::endl;

    gaia_config_fallback_t gaia_conf{conf_file_path};

    data_dir = gaia_conf.get_value<std::string>(
        data_dir, c_data_dir_string_env, c_data_dir_string_key, "");

    instance_name = gaia_conf.get_value<std::string>(
        instance_name, c_instance_name_string_env,
        c_instance_name_string_key, c_default_instance_name);

    // In case anyone can see this...
    if (persistence_mode == server_config_t::persistence_mode_t::e_disabled)
    {
        std::cerr
            << "Persistence is disabled." << std::endl;
    }

    if (persistence_mode == server_config_t::persistence_mode_t::e_disabled_after_recovery)
    {
        std::cerr
            << "Persistence is disabled after recovery." << std::endl;
    }

    if (persistence_mode == server_config_t::persistence_mode_t::e_reinitialized_on_startup)
    {
        std::cerr
            << "Persistence is reinitialized on startup." << std::endl;
    }

    std::cerr
        << "Database instance name is '" << instance_name << "'." << std::endl;

    if (persistence_mode != server_config_t::persistence_mode_t::e_disabled)
    {
        if (data_dir.empty())
        {
            std::cerr
                << "Unable to locate a database directory.\n"
                << "Use the '--data-dir' flag, or the configuration file "
                << "to identify the location of the database."
                << std::endl;
            usage();
            std::exit(1);
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

    return server_config_t{persistence_mode, instance_name, data_dir, testing};
}

int main(int argc, char* argv[])
{
    auto server_conf = process_command_line(argc, argv);

    std::cerr << c_db_server_name << " started!" << std::endl;

    gaia::db::server_t::run(server_conf);
}
