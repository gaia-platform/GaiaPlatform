////////////////////////////////////////////////////
// Copyright (c) Gaia Platform Authors
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace gaia
{
namespace db
{

/**
 * Configuration to start an gaia_db_server instance.
 */
struct server_instance_config_t
{
    std::filesystem::path server_exec_path;
    std::string instance_name;
    bool disable_persistence;
    bool skip_catalog_integrity_check;
    std::string data_dir;

    /**
     * Builds a configuration for connecting to the default database instance.
     */
    static server_instance_config_t get_default_config();

    /**
     * Builds a configuration for connecting to a new server instance.
     * This method will also generate the name of the new instance.
     */
    static server_instance_config_t get_new_instance_config();

    /**
     * Finds the server executable path. Throws an exception if
     * the path is not found.
     */
    static std::filesystem::path find_server_path();

    /**
     * Generates a random instance name unless the GAIA_DB_INSTANCE_NAME
     * env is set.
     */
    static std::string generate_instance_name();

    static std::string generate_data_dir(const std::string& instance_name);
};

/**
 * Controls a gaia_db_server instance.
 */
class server_instance_t
{
public:
    explicit server_instance_t(const server_instance_config_t server_conf)
        : m_conf(server_conf)
    {
    }

    server_instance_t()
        : server_instance_t(server_instance_config_t::get_new_instance_config())
    {
    }

    /**
     * Starts a gaia_db_server instance into a separated process.
     * The gaia_db_server will be killed when the parent process dies.
     *
     * @param wait_for_init if true, wait for the server to be initialized.
     */
    void start(bool wait_for_init = true);

    /**
     * Kill the gaia_db_server instance associated with this class.
     */
    void stop();

    /**
     * Stop() and start().
     *
     * @param wait_for_init if true, wait for the server to be initialized.
     */
    void restart(bool wait_for_init = true);

    /**
     * Reset the client shared memory and the server data.
     *
     * @param wait_for_init if true, wait for the server to be initialized.
     */
    void reset_server(bool wait_for_init = true);

    /**
     * Waits for the server to be initialized.
     */
    void wait_for_init();

    /**
     * Delete the data directory.
     */
    void delete_data_dir();

    /**
     * Returns the instance name.
     */
    std::string instance_name();

    /**
     * Returns true if we are skipping catalog integrity checks (for testing).
     */
    bool skip_catalog_integrity_check();

    /**
     * Returns true is start() was called.
     */
    bool is_initialized();

private:
    server_instance_config_t m_conf;
    ::pid_t m_server_pid = -1;
    bool m_is_initialized{false};

    std::vector<const char*> get_server_command_and_argument();
};

} // namespace db
} // namespace gaia
