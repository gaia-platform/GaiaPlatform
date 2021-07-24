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

TEST_F(ddl_execution_test, create_relationship_using_fields)
{
    const string ddl = R"(
DROP TABLE IF EXISTS t1;
CREATE TABLE IF NOT EXISTS t1(c1 INT32 UNIQUE);

DROP TABLE IF EXISTS t2;
CREATE TABLE IF NOT EXISTS t2(c2 INT32 UNIQUE);

CREATE RELATIONSHIP r1 (
  t1.link1 -> t2,
  t2.link2 -> t1,
  USING t1(c1), t2(c2)
);
)";

    ddl::parser_t parser;
    ASSERT_NO_THROW(parser.parse_line(ddl));
    ASSERT_NO_THROW(execute(parser.statements));

    gaia::direct_access::auto_transaction_t txn(false);
    ASSERT_EQ(
        gaia_relationship_t::list().where(gaia_relationship_expr::to_child_link_name == "link1").begin()->child_fields()[0],
        gaia_field_t::list().where(gaia_field_expr::name == "c2").begin()->gaia_id());
    ASSERT_EQ(
        gaia_relationship_t::list().where(gaia_relationship_expr::to_child_link_name == "link1").begin()->parent_fields()[0],
        gaia_field_t::list().where(gaia_field_expr::name == "c1").begin()->gaia_id());
}

TEST_F(ddl_execution_test, drop_relationship)
{
    const string create_relationship_ddl = R"(
DROP TABLE IF EXISTS t1;
CREATE TABLE IF NOT EXISTS t1(c1 INT32);

DROP TABLE IF EXISTS t2;
CREATE TABLE IF NOT EXISTS t2(c2 INT32);

DROP RELATIONSHIP IF EXISTS r1;
CREATE RELATIONSHIP IF NOT EXISTS r1 (
  t1.link1 -> t2,
  t2.link2 -> t1
);

DROP RELATIONSHIP IF EXISTS r2;
CREATE RELATIONSHIP IF NOT EXISTS r2 (
  t2.link3 -> t1,
  t1.link4 -> t2
);
)";

    const string drop_relationship_ddl = R"(
DROP RELATIONSHIP IF EXISTS r1;
DROP RELATIONSHIP IF EXISTS r2;
DROP TABLE IF EXISTS t1;
DROP TABLE IF EXISTS t2;
)";

    ddl::parser_t parser;
    ASSERT_NO_THROW(parser.parse_line(create_relationship_ddl));
    ASSERT_NO_THROW(execute(parser.statements));

    {
        gaia::direct_access::auto_transaction_t txn(false);

        ASSERT_EQ(gaia_table_t::list().where(gaia_table_expr::name == "t1").size(), 1);
        ASSERT_EQ(gaia_table_t::list().where(gaia_table_expr::name == "t2").size(), 1);

        ASSERT_EQ(gaia_relationship_t::list().where(gaia_relationship_expr::name == "r1").size(), 1);
        ASSERT_EQ(gaia_relationship_t::list().where(gaia_relationship_expr::name == "r2").size(), 1);
    }

    ASSERT_NO_THROW(parser.parse_line(drop_relationship_ddl));
    ASSERT_NO_THROW(execute(parser.statements));

    {
        gaia::direct_access::auto_transaction_t txn(false);

        ASSERT_EQ(gaia_relationship_t::list().where(gaia_relationship_expr::name == "r1").size(), 0);
        ASSERT_EQ(gaia_relationship_t::list().where(gaia_relationship_expr::name == "r2").size(), 0);

        ASSERT_EQ(gaia_table_t::list().where(gaia_table_expr::name == "t1").size(), 0);
        ASSERT_EQ(gaia_table_t::list().where(gaia_table_expr::name == "t2").size(), 0);
    }

    ASSERT_NO_THROW(parser.parse_line("DROP RELATIONSHIP r3;"));
    ASSERT_THROW(execute(parser.statements), relationship_not_exists);
}
