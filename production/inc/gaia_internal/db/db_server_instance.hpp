/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <vector>

namespace gaia
{
namespace db
{

struct server_instance_conf_t
{
    std::string server_exec_path;
    std::string instance_name;
    // TODO these should be mutually exclusive
    bool reinitialize_persistent_store;
    bool disable_persistence;
    std::string data_dir;

    static server_instance_conf_t get_default();

    static std::string find_server_exec_path();

    static std::string generate_instance_name();

    static std::string generate_data_dir(const std::string& instance_name);
};

class server_instance_t
{
public:
    server_instance_t(const server_instance_conf_t server_conf)
        : m_conf(server_conf)
    {
    }

    server_instance_t()
        : server_instance_t(server_instance_conf_t::get_default())
    {
    }

    void start(bool wait_for_init = true);

    void stop();

    void restart(bool wait_for_init = true);

    void reset_server();

    void wait_for_init();

    void delete_data_dir();

    std::string instance_name();

    bool is_initialized();

private:
    server_instance_conf_t m_conf;
    ::pid_t m_server_pid = -1;
    bool m_is_initialized{false};

    std::vector<const char*> get_server_command();
};

} // namespace db
} // namespace gaia
