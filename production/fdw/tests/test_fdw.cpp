/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/common/file.hpp"
#include "gaia_internal/db/db_catalog_test_base.hpp"

using namespace std;

using namespace gaia::db;
using namespace gaia::catalog;
using namespace gaia::catalog::ddl;

class fdw__test : public db_catalog_test_base_t
{
protected:
    fdw__test()
        : db_catalog_test_base_t()
    {
    }

    static void SetUpTestSuite()
    {
        gaia_log::initialize({});

        // For these tests, we need to use the default database instance,
        // because the FDW is unaware of any other instances.
        server_instance_config_t conf = server_instance_config_t::get_default_config();
        config::set_default_session_options(config::get_default_session_options());

        s_server_instance = server_instance_t(conf);
        s_server_instance.start();
        s_server_instance.wait_for_init();
    }
};

void verify_command_output(string command_filename)
{
    // Load the command string generated by the build.
    gaia::common::file_loader_t command_loader;
    command_loader.load_file_data(command_filename, true);

    char* command = reinterpret_cast<char*>(command_loader.get_data());

    cerr
        << "Command executed by test is:" << endl
        << command << endl;

    // Execute the command and validate its return value.
    int return_value = system(command);
    ASSERT_EQ(0, return_value);
}

TEST_F(fdw__test, array)
{
    load_schema("array_schema.ddl");

    // End the DDL session needed for the schema loading
    // and start a regular session for the remainder of the test.
    gaia::db::end_session();
    gaia::db::begin_session();

    verify_command_output("fdw_test_array_command.txt");
}

TEST_F(fdw__test, airport)
{
    load_schema("airport_schema.ddl");

    // End the DDL session needed for the schema loading
    // and start a regular session for the remainder of the test.
    gaia::db::end_session();
    gaia::db::begin_session();

    verify_command_output("fdw_test_airport_command.txt");
}
