/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "benchmark_test_base.hpp"

#include <utility>

#include "gaia/system.hpp"

namespace gaia
{
namespace bench
{

benchmark_test_base::benchmark_test_base(std::string ddl_path, bool persistence_enabled)
    : m_ddl_path(std::move(ddl_path)), m_persistence_enabled(persistence_enabled)
{
    auto server_config = cmd::server_instance_config_t::get_default_config();

    if (m_persistence_enabled)
    {
        server_config.disable_persistence = false;
        server_config.data_dir = cmd::server_instance_config_t::generate_data_dir(server_config.instance_name);
    }

    m_server = cmd::server_instance_t(server_config);
}

benchmark_test_base::benchmark_test_base(std::string ddl_path)
    : benchmark_test_base(std::move(ddl_path), false)
{
}

void benchmark_test_base::SetUp()
{
    m_server.start();
    gaia::system::initialize("gaia.conf", "gaia_log.conf");

    m_gaiac.load_ddl(m_ddl_path);
}

void benchmark_test_base::TearDown()
{
    gaia::system::shutdown();
    // The server is killed in between runs to ensure to start with a clean state.
    m_server.stop();
}

} // namespace bench
} // namespace gaia
