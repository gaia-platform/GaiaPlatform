/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"

#include "catalog.hpp"
#include "gaia_parser.hpp"
#include "yy_parser.hpp"

using namespace gaia::catalog::ddl;

TEST(catalog_ddl_parser_test, create_table)
{
    parser_t parser;
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("CREATE TABLE t (c INT32);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create);

    auto create_stmt = dynamic_cast<create_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_table);
    EXPECT_EQ(create_stmt->name, "t");
    EXPECT_FALSE(create_stmt->if_not_exists);
}

TEST(catalog_ddl_parser_test, create_table_if_not_exists)
{
    parser_t parser;
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("CREATE TABLE IF NOT EXISTS t (c INT32);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create);

    auto create_stmt = dynamic_cast<create_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_table);
    EXPECT_EQ(create_stmt->name, "t");
    EXPECT_TRUE(create_stmt->if_not_exists);
}

TEST(catalog_ddl_parser_test, create_table_multiple_fields)
{
    parser_t parser;
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("CREATE TABLE t (c1 INT32[], c2 DOUBLE[2]);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create);

    auto create_stmt = dynamic_cast<create_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_table);
    EXPECT_EQ(create_stmt->name, "t");
    EXPECT_EQ(create_stmt->fields.size(), 2);

    EXPECT_EQ(create_stmt->fields.at(0)->name, "c1");
    EXPECT_EQ(create_stmt->fields.at(0)->type, data_type_t::e_int32);
    EXPECT_EQ(create_stmt->fields.at(0)->length, 0);
    EXPECT_EQ(create_stmt->fields.at(0)->active, false);

    EXPECT_EQ(create_stmt->fields.at(1)->name, "c2");
    EXPECT_EQ(create_stmt->fields.at(1)->type, data_type_t::e_double);
    EXPECT_EQ(create_stmt->fields.at(1)->length, 2);
    EXPECT_EQ(create_stmt->fields.at(0)->active, false);
}

TEST(catalog_ddl_parser_test, create_table_references)
{
    parser_t parser;
    string ddl = string(
        "CREATE TABLE t "
        "(c1 REFERENCES t1,"
        " REFERENCES d.t2);");
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line(ddl));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create);

    auto create_stmt = dynamic_cast<create_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_table);
    EXPECT_EQ(create_stmt->name, "t");
    EXPECT_EQ(create_stmt->fields.size(), 2);

    EXPECT_EQ(create_stmt->fields.at(0)->name, "c1");
    EXPECT_EQ(create_stmt->fields.at(0)->type, data_type_t::e_references);
    EXPECT_EQ(create_stmt->fields.at(0)->table_type_name, "t1");
    EXPECT_EQ(create_stmt->fields.at(0)->table_type_database, "");
    EXPECT_EQ(create_stmt->fields.at(0)->length, 1);
    EXPECT_EQ(create_stmt->fields.at(0)->active, false);

    // Anonymous references have the same binding for named references except the name string is empty.
    EXPECT_EQ(create_stmt->fields.at(1)->name, "");
    EXPECT_EQ(create_stmt->fields.at(1)->type, data_type_t::e_references);
    EXPECT_EQ(create_stmt->fields.at(1)->table_type_name, "t2");
    EXPECT_EQ(create_stmt->fields.at(1)->table_type_database, "d");
    EXPECT_EQ(create_stmt->fields.at(1)->length, 1);
    EXPECT_EQ(create_stmt->fields.at(1)->active, false);
}

TEST(catalog_ddl_parser_test, drop_table)
{
    parser_t parser;
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("DROP TABLE t;"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::drop);

    auto drop_stmt = dynamic_cast<drop_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(drop_stmt->type, drop_type_t::drop_table);
    EXPECT_EQ(drop_stmt->name, "t");
}

TEST(catalog_ddl_parser_test, case_sensitivity)
{
    parser_t parser;
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("CREATE TABLE t (c INT32);"));
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("create table t (c int32);"));
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("cReAte taBle T (c int32);"));
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("CREATE TABLE T (c int32, C int32);"));

    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("DROP TABLE t;"));
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("drop table t;"));
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("DrOp TaBle T;"));
}

TEST(catalog_ddl_parser_test, create_active_field)
{
    parser_t parser;
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("CREATE TABLE t (id INT32[] ACTIVE, name STRING ACTIVE);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create);

    auto create_stmt = dynamic_cast<create_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_table);
    EXPECT_EQ(create_stmt->name, "t");
    EXPECT_EQ(create_stmt->fields.size(), 2);

    EXPECT_EQ(create_stmt->fields.at(0)->name, "id");
    EXPECT_EQ(create_stmt->fields.at(0)->type, data_type_t::e_int32);
    EXPECT_EQ(create_stmt->fields.at(0)->length, 0);
    EXPECT_EQ(create_stmt->fields.at(0)->active, true);

    EXPECT_EQ(create_stmt->fields.at(1)->name, "name");
    EXPECT_EQ(create_stmt->fields.at(1)->type, data_type_t::e_string);
    EXPECT_EQ(create_stmt->fields.at(1)->length, 1);
    EXPECT_EQ(create_stmt->fields.at(1)->active, true);
}

TEST(catalog_ddl_parser_test, create_database)
{
    parser_t parser;
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("CREATE DATABASE db;"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create);

    auto create_stmt = dynamic_cast<create_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_database);
    EXPECT_EQ(create_stmt->name, "db");
    EXPECT_FALSE(create_stmt->if_not_exists);
}

TEST(catalog_ddl_parser_test, create_database_if_not_exists)
{
    parser_t parser;
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("CREATE DATABASE IF NOT EXISTS db;"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create);

    auto create_stmt = dynamic_cast<create_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_database);
    EXPECT_EQ(create_stmt->name, "db");
    EXPECT_TRUE(create_stmt->if_not_exists);
}

TEST(catalog_ddl_parser_test, create_table_in_database)
{
    parser_t parser;
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("CREATE TABLE d.t (id INT32);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create);

    auto create_stmt = dynamic_cast<create_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_table);
    EXPECT_EQ(create_stmt->name, "t");
    EXPECT_EQ(create_stmt->database, "d");
}

TEST(catalog_ddl_parser_test, drop_database)
{
    parser_t parser;
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("DROP DATABASE d;"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::drop);

    auto drop_stmt = dynamic_cast<drop_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(drop_stmt->type, drop_type_t::drop_database);
    EXPECT_EQ(drop_stmt->name, "d");
}
