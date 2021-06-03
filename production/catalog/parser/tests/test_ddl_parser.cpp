/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"

#include "gaia_internal/catalog/catalog.hpp"

#include "gaia_parser.hpp"

using namespace std;
using namespace gaia::catalog::ddl;

TEST(catalog_ddl_parser_test, create_table)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_line("CREATE TABLE t (c INT32);"));

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
    ASSERT_NO_THROW(parser.parse_line("CREATE TABLE IF NOT EXISTS t (c INT32);"));

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
    ASSERT_NO_THROW(parser.parse_line("CREATE TABLE t (c1 INT32[], c2 DOUBLE[]);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create);

    auto create_stmt = dynamic_cast<create_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_table);
    EXPECT_EQ(create_stmt->name, "t");
    EXPECT_EQ(create_stmt->fields.size(), 2);

    const data_field_def_t* field;

    EXPECT_EQ(create_stmt->fields.at(0)->field_type, field_type_t::data);
    field = dynamic_cast<data_field_def_t*>(create_stmt->fields.at(0).get());
    EXPECT_EQ(field->name, "c1");
    EXPECT_EQ(field->data_type, data_type_t::e_int32);
    EXPECT_EQ(field->length, 0);
    EXPECT_EQ(field->active, false);

    EXPECT_EQ(create_stmt->fields.at(1)->field_type, field_type_t::data);
    field = dynamic_cast<data_field_def_t*>(create_stmt->fields.at(1).get());
    EXPECT_EQ(field->name, "c2");
    EXPECT_EQ(field->data_type, data_type_t::e_double);
    EXPECT_EQ(field->length, 0);
    EXPECT_EQ(field->active, false);
}

TEST(catalog_ddl_parser_test, drop_table)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_line("DROP TABLE t;"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::drop);

    auto drop_stmt = dynamic_cast<drop_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(drop_stmt->type, drop_type_t::drop_table);
    EXPECT_EQ(drop_stmt->name, "t");
    EXPECT_FALSE(drop_stmt->if_exists);

    ASSERT_NO_THROW(parser.parse_line("DROP TABLE IF EXISTS t;"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::drop);

    drop_stmt = dynamic_cast<drop_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(drop_stmt->type, drop_type_t::drop_table);
    EXPECT_EQ(drop_stmt->name, "t");
    EXPECT_TRUE(drop_stmt->if_exists);
}

TEST(catalog_ddl_parser_test, case_sensitivity)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_line("CREATE TABLE t (c INT32);"));
    ASSERT_NO_THROW(parser.parse_line("create table t (c int32);"));
    ASSERT_NO_THROW(parser.parse_line("cReAte taBle T (c int32);"));
    ASSERT_NO_THROW(parser.parse_line("CREATE TABLE T (c int32, C int32);"));

    ASSERT_NO_THROW(parser.parse_line("DROP TABLE t;"));
    ASSERT_NO_THROW(parser.parse_line("drop table t;"));
    ASSERT_NO_THROW(parser.parse_line("DrOp TaBle T;"));
}

TEST(catalog_ddl_parser_test, create_active_field)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_line("CREATE TABLE t (id INT32[] ACTIVE, name STRING ACTIVE);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create);

    auto create_stmt = dynamic_cast<create_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_table);
    EXPECT_EQ(create_stmt->name, "t");
    EXPECT_EQ(create_stmt->fields.size(), 2);

    const data_field_def_t* field;

    EXPECT_EQ(create_stmt->fields.at(0)->field_type, field_type_t::data);
    field = dynamic_cast<data_field_def_t*>(create_stmt->fields.at(0).get());
    EXPECT_EQ(field->name, "id");
    EXPECT_EQ(field->data_type, data_type_t::e_int32);
    EXPECT_EQ(field->length, 0);
    EXPECT_EQ(field->active, true);

    EXPECT_EQ(create_stmt->fields.at(1)->field_type, field_type_t::data);
    field = dynamic_cast<data_field_def_t*>(create_stmt->fields.at(1).get());
    EXPECT_EQ(field->name, "name");
    EXPECT_EQ(field->data_type, data_type_t::e_string);
    EXPECT_EQ(field->length, 1);
    EXPECT_EQ(field->active, true);
}

TEST(catalog_ddl_parser_test, create_database)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_line("CREATE DATABASE db;"));

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
    ASSERT_NO_THROW(parser.parse_line("CREATE DATABASE IF NOT EXISTS db;"));

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
    ASSERT_NO_THROW(parser.parse_line("CREATE TABLE d.t (id INT32);"));

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
    ASSERT_NO_THROW(parser.parse_line("DROP DATABASE d;"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::drop);

    auto drop_stmt = dynamic_cast<drop_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(drop_stmt->type, drop_type_t::drop_database);
    EXPECT_EQ(drop_stmt->name, "d");
    EXPECT_FALSE(drop_stmt->if_exists);

    ASSERT_NO_THROW(parser.parse_line("DROP DATABASE IF EXISTS d;"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::drop);

    drop_stmt = dynamic_cast<drop_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(drop_stmt->type, drop_type_t::drop_database);
    EXPECT_EQ(drop_stmt->name, "d");
    EXPECT_TRUE(drop_stmt->if_exists);
}

TEST(catalog_ddl_parser_test, illegal_characters)
{
    parser_t parser;
    EXPECT_THROW(parser.parse_line("CREATE TABLE t(id : int8);"), parsing_error);
    EXPECT_THROW(parser.parse_line("CREATE : TABLE t(id int8);"), parsing_error);
    EXPECT_THROW(parser.parse_line("CREATE TABLE t(id - int8);"), parsing_error);
}

TEST(catalog_ddl_parser_test, fixed_size_array)
{
    parser_t parser;
    ASSERT_THROW(parser.parse_line("CREATE TABLE t (c INT32[2]);"), parsing_error);
    ASSERT_THROW(parser.parse_line("CREATE TABLE t (c INT32[1]);"), parsing_error);
    ASSERT_THROW(parser.parse_line("CREATE TABLE t (c INT32[0]);"), parsing_error);
}

TEST(catalog_ddl_parser_test, vector_of_strings)
{
    parser_t parser;
    ASSERT_THROW(parser.parse_line("CREATE TABLE t (c STRING[]);"), parsing_error);
}

TEST(catalog_ddl_parser_test, code_comments)
{
    const string correct_ddl_text = R"(
-------------------------------
/*
 * block comment
 */
-------------------------------
CREATE TABLE t -- create table t
( /**
   **/ id int, -- column id
  /* column c */ c STRING -- column c
); -- end create table t
-------------------------------
)";

    const string incorrect_ddl_text = R"(/*)";

    parser_t parser;

    ASSERT_NO_THROW(parser.parse_line(correct_ddl_text));
    EXPECT_EQ(1, parser.statements.size());

    ASSERT_THROW(parser.parse_line(incorrect_ddl_text), parsing_error);
}

TEST(catalog_ddl_parser_test, create_empty_table)
{
    parser_t parser;

    ASSERT_NO_THROW(parser.parse_line("CREATE TABLE t ();"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create);

    auto create_stmt = dynamic_cast<create_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_table);
    EXPECT_EQ(create_stmt->name, "t");
    EXPECT_FALSE(create_stmt->if_not_exists);
    EXPECT_EQ(create_stmt->fields.size(), 0);
}

TEST(catalog_ddl_parser_test, create_relationship)
{
    parser_t parser;

    const string ddl_text_full_db = R"(
CREATE RELATIONSHIP r (
  d1.t1.link1 -> d2.t2,
  d2.t2.link2 -> d1.t1
);
)";
    ASSERT_NO_THROW(parser.parse_line(ddl_text_full_db));
    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create);
    auto create_stmt = dynamic_cast<create_statement_t*>(parser.statements[0].get());
    EXPECT_EQ(create_stmt->type, create_type_t::create_relationship);

    const string ddl_text_no_db = R"(
CREATE RELATIONSHIP r (
  t1.link1 -> t2,
  t2.link2 -> t1
);
)";
    ASSERT_NO_THROW(parser.parse_line(ddl_text_no_db));
    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create);
    create_stmt = dynamic_cast<create_statement_t*>(parser.statements[0].get());
    EXPECT_EQ(create_stmt->type, create_type_t::create_relationship);

    const string ddl_text_partial_db = R"(
CREATE RELATIONSHIP r (
  d1.t1.link1 -> t2,
  t2.link2 -> d1.t1
);
)";
    ASSERT_NO_THROW(parser.parse_line(ddl_text_partial_db));
    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create);
    create_stmt = dynamic_cast<create_statement_t*>(parser.statements[0].get());
    EXPECT_EQ(create_stmt->type, create_type_t::create_relationship);

    const string ddl_text_negative_case_no_name = R"(
CREATE RELATIONSHIP (
  t1.link1 -> t2,
  t2.link2 -> t1
);
)";
    ASSERT_THROW(parser.parse_line(ddl_text_negative_case_no_name), parsing_error);

    const string ddl_text_negative_case_single_link = R"(
CREATE RELATIONSHIP r (
  t1.link -> t2,
);
)";
    ASSERT_THROW(parser.parse_line(ddl_text_negative_case_single_link), parsing_error);
}

TEST(catalog_ddl_parser_test, create_index)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_line("CREATE INDEX IF NOT EXISTS idx ON d.t (name);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create);

    auto create_stmt = dynamic_cast<create_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_index);
    EXPECT_EQ(create_stmt->name, "idx");
    EXPECT_EQ(create_stmt->database, "d");
    EXPECT_EQ(create_stmt->index_table, "t");
    EXPECT_EQ(create_stmt->unique_index, false);
    EXPECT_EQ(create_stmt->index_type, index_type_t::range);
}

TEST(catalog_ddl_parser_test, create_unique_index)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_line("CREATE UNIQUE INDEX idx ON d.t (name);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create);

    auto create_stmt = dynamic_cast<create_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_index);
    EXPECT_EQ(create_stmt->name, "idx");
    EXPECT_EQ(create_stmt->database, "d");
    EXPECT_EQ(create_stmt->index_table, "t");
    EXPECT_EQ(create_stmt->unique_index, true);
    EXPECT_EQ(create_stmt->index_type, index_type_t::range);
}

TEST(catalog_ddl_parser_test, create_hash_index)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_line("CREATE UNIQUE HASH INDEX idx ON d.t (name);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create);

    auto create_stmt = dynamic_cast<create_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_index);
    EXPECT_EQ(create_stmt->name, "idx");
    EXPECT_EQ(create_stmt->database, "d");
    EXPECT_EQ(create_stmt->index_table, "t");
    EXPECT_EQ(create_stmt->unique_index, true);
    EXPECT_EQ(create_stmt->index_type, index_type_t::hash);
}

TEST(catalog_ddl_parser_test, create_range_index)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_line("CREATE RANGE INDEX IF NOT EXISTS idx ON d.t (name);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create);

    auto create_stmt = dynamic_cast<create_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_index);
    EXPECT_EQ(create_stmt->name, "idx");
    EXPECT_EQ(create_stmt->database, "d");
    EXPECT_EQ(create_stmt->index_table, "t");
    EXPECT_EQ(create_stmt->unique_index, false);
    EXPECT_EQ(create_stmt->index_type, index_type_t::range);
}
