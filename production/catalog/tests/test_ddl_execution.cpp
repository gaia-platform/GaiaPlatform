/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"

#include "gaia/direct_access/auto_transaction.hpp"

#include "gaia_internal/catalog/ddl_execution.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "catalog_tests_helper.hpp"
#include "gaia_parser.hpp"

using namespace std;

using namespace gaia::catalog;
using namespace gaia::common;
using namespace gaia::db;

class ddl_execution_test : public db_catalog_test_base_t
{
protected:
    ddl_execution_test()
        : db_catalog_test_base_t(){};

    void check_table_name(gaia_id_t id, const string& name)
    {
        gaia::db::begin_transaction();
        gaia_table_t t = gaia_table_t::get(id);
        EXPECT_EQ(name, t.name());
        gaia::db::commit_transaction();
    }
};

TEST_F(ddl_execution_test, create_table_with_unique_constraints)
{
    ddl::parser_t parser;
    ASSERT_NO_THROW(parser.parse_line("DROP TABLE IF EXISTS t; CREATE TABLE IF NOT EXISTS t(c INT32 UNIQUE);"));
    ASSERT_EQ(parser.statements.size(), 2);
    ASSERT_NO_THROW(execute(parser.statements));

    gaia::direct_access::auto_transaction_t txn(false);
    ASSERT_TRUE(gaia_table_t::list()
                    .where(gaia_table_expr::name == "t")
                    .begin()
                    ->gaia_fields()
                    .where(gaia_field_expr::name == "c")
                    .begin()
                    ->unique());
    ASSERT_EQ(gaia_index_t::list().where(gaia_index_expr::name == "t_c").size(), 1);
}
