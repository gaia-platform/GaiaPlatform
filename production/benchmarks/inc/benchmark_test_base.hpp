/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <gtest/gtest.h>

#include "db_server_cmd.hpp"
#include "gaiac_cmd.hpp"

namespace gaia
{
namespace benchmark
{

// Flags to enable/disable persistence.
constexpr bool c_persistence_disabled = false;
constexpr bool c_persistence_enabled = true;

/**
 * Base class to run benchmarks. It takes care of running the db server and loading the DDL into it.
 * The server is killed between each run to ensure a clean start.
 */
class benchmark_test_base : public ::testing::Test
{
public:
    benchmark_test_base(std::string ddl_path, bool persistence_enabled);
    explicit benchmark_test_base(std::string ddl_path);

protected:
    static void SetUpTestSuite()
    {
        // TODO: Workaround until this PR is merged: https://github.com/gaia-platform/GaiaPlatform/pull/1501
        //  Kills the gaia_db_server run by ctest which interferes with the gaia_db_server run by the benchmark.
        system("pkill -f -KILL gaia_db_server ");
    }

    static void TearDownTestSuite()
    {
        // TODO: Workaround until this PR is merged: https://github.com/gaia-platform/GaiaPlatform/pull/1501
        //  Ctest expects the gaia_db_server to be running in at the end of this test.
        //  If it is not running it the following tests will fail.
        system("systemd-run --no-ask-password ../db/core/gaia_db_server --persistence disabled &");
    }

    void SetUp() override;

    void TearDown() override;

private:
    gaia::cmd::server_instance_t m_server;
    gaia::cmd::gaiac_t m_gaiac;
    std::string m_ddl_path;
    bool m_persistence_enabled;
};

} // namespace benchmark
} // namespace gaia
