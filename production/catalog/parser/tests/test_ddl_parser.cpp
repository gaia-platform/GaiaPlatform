////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <array>

#include <gtest/gtest.h>

#include "gaia_internal/catalog/catalog.hpp"

#include "gaia_parser.hpp"

using namespace std;
using namespace gaia::catalog::ddl;

TEST(catalog__ddl_parser__test, create_table)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_string("CREATE TABLE t (c INT32);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create_list);

    auto create_list = dynamic_cast<create_list_t*>(parser.statements[0].get());
    auto create_stmt = dynamic_cast<create_statement_t*>(create_list->statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_table);
    EXPECT_EQ(create_stmt->name, "t");
    EXPECT_FALSE(create_stmt->has_if_not_exists);
}

TEST(catalog__ddl_parser__test, create_table_if_not_exists)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_string("CREATE TABLE IF NOT EXISTS t (c INT32);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create_list);

    auto create_list = dynamic_cast<create_list_t*>(parser.statements[0].get());
    auto create_stmt = dynamic_cast<create_statement_t*>(create_list->statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_table);
    EXPECT_EQ(create_stmt->name, "t");
    EXPECT_TRUE(create_stmt->has_if_not_exists);
}

TEST(catalog__ddl_parser__test, create_table_multiple_fields)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_string("CREATE TABLE t (c1 INT32[], c2 DOUBLE[]);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create_list);

    auto create_list = dynamic_cast<create_list_t*>(parser.statements[0].get());
    auto create_stmt = dynamic_cast<create_statement_t*>(create_list->statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_table);

    auto create_table = dynamic_cast<create_table_t*>(create_stmt);

    EXPECT_EQ(create_table->name, "t");
    EXPECT_EQ(create_table->fields.size(), 2);

    const data_field_def_t* field;

    EXPECT_EQ(create_table->fields.at(0)->field_type, field_type_t::data);
    field = dynamic_cast<data_field_def_t*>(create_table->fields.at(0).get());
    EXPECT_EQ(field->name, "c1");
    EXPECT_EQ(field->data_type, data_type_t::e_int32);
    EXPECT_EQ(field->length, 0);
    EXPECT_EQ(field->active, false);

    EXPECT_EQ(create_table->fields.at(1)->field_type, field_type_t::data);
    field = dynamic_cast<data_field_def_t*>(create_table->fields.at(1).get());
    EXPECT_EQ(field->name, "c2");
    EXPECT_EQ(field->data_type, data_type_t::e_double);
    EXPECT_EQ(field->length, 0);
    EXPECT_EQ(field->active, false);
}

TEST(catalog__ddl_parser__test, drop_table)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_string("DROP TABLE t;"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::drop);

    auto drop_stmt = dynamic_cast<drop_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(drop_stmt->type, drop_type_t::drop_table);
    EXPECT_EQ(drop_stmt->name, "t");
    EXPECT_FALSE(drop_stmt->if_exists);

    ASSERT_NO_THROW(parser.parse_string("DROP TABLE IF EXISTS t;"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::drop);

    drop_stmt = dynamic_cast<drop_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(drop_stmt->type, drop_type_t::drop_table);
    EXPECT_EQ(drop_stmt->name, "t");
    EXPECT_TRUE(drop_stmt->if_exists);
}

TEST(catalog__ddl_parser__test, case_sensitivity)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_string("CREATE TABLE t (c INT32);"));
    ASSERT_NO_THROW(parser.parse_string("create table t (c int32);"));
    ASSERT_NO_THROW(parser.parse_string("cReAte taBle T (c int32);"));
    ASSERT_NO_THROW(parser.parse_string("CREATE TABLE T (c int32, C int32);"));

    ASSERT_NO_THROW(parser.parse_string("DROP TABLE t;"));
    ASSERT_NO_THROW(parser.parse_string("drop table t;"));
    ASSERT_NO_THROW(parser.parse_string("DrOp TaBle T;"));
}

TEST(catalog__ddl_parser__test, create_active_field)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_string("CREATE TABLE t (id INT32[] ACTIVE, name STRING ACTIVE);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create_list);

    auto create_list = dynamic_cast<create_list_t*>(parser.statements[0].get());
    auto create_stmt = dynamic_cast<create_statement_t*>(create_list->statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_table);

    auto create_table = dynamic_cast<create_table_t*>(create_stmt);

    EXPECT_EQ(create_table->name, "t");
    EXPECT_EQ(create_table->fields.size(), 2);

    const data_field_def_t* field;

    EXPECT_EQ(create_table->fields.at(0)->field_type, field_type_t::data);
    field = dynamic_cast<data_field_def_t*>(create_table->fields.at(0).get());
    EXPECT_EQ(field->name, "id");
    EXPECT_EQ(field->data_type, data_type_t::e_int32);
    EXPECT_EQ(field->length, 0);
    EXPECT_EQ(field->active, true);

    EXPECT_EQ(create_table->fields.at(1)->field_type, field_type_t::data);
    field = dynamic_cast<data_field_def_t*>(create_table->fields.at(1).get());
    EXPECT_EQ(field->name, "name");
    EXPECT_EQ(field->data_type, data_type_t::e_string);
    EXPECT_EQ(field->length, 1);
    EXPECT_EQ(field->active, true);
}

TEST(catalog__ddl_parser__test, create_database)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_string("CREATE DATABASE db;"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create_list);

    auto create_list = dynamic_cast<create_list_t*>(parser.statements[0].get());
    auto create_stmt = dynamic_cast<create_statement_t*>(create_list->statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_database);
    EXPECT_EQ(create_stmt->name, "db");
    EXPECT_FALSE(create_stmt->has_if_not_exists);
}

TEST(catalog__ddl_parser__test, create_database_if_not_exists)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_string("CREATE DATABASE IF NOT EXISTS db;"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create_list);

    auto create_list = dynamic_cast<create_list_t*>(parser.statements[0].get());
    auto create_stmt = dynamic_cast<create_statement_t*>(create_list->statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_database);
    EXPECT_EQ(create_stmt->name, "db");
    EXPECT_TRUE(create_stmt->has_if_not_exists);
}

TEST(catalog__ddl_parser__test, create_table_in_database)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_string("CREATE TABLE d.t (id INT32);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create_list);

    auto create_list = dynamic_cast<create_list_t*>(parser.statements[0].get());
    auto create_stmt = dynamic_cast<create_statement_t*>(create_list->statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_table);

    auto create_table = dynamic_cast<create_table_t*>(create_stmt);

    EXPECT_EQ(create_table->name, "t");
    EXPECT_EQ(create_table->database, "d");
}

TEST(catalog__ddl_parser__test, drop_database)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_string("DROP DATABASE d;"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::drop);

    auto drop_stmt = dynamic_cast<drop_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(drop_stmt->type, drop_type_t::drop_database);
    EXPECT_EQ(drop_stmt->name, "d");
    EXPECT_FALSE(drop_stmt->if_exists);

    ASSERT_NO_THROW(parser.parse_string("DROP DATABASE IF EXISTS d;"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::drop);

    drop_stmt = dynamic_cast<drop_statement_t*>(parser.statements[0].get());

    EXPECT_EQ(drop_stmt->type, drop_type_t::drop_database);
    EXPECT_EQ(drop_stmt->name, "d");
    EXPECT_TRUE(drop_stmt->if_exists);
}

TEST(catalog__ddl_parser__test, illegal_characters)
{
    parser_t parser;
    EXPECT_THROW(parser.parse_string("CREATE TABLE t(id : int8);"), parsing_error);
    EXPECT_THROW(parser.parse_string("CREATE : TABLE t(id int8);"), parsing_error);
    EXPECT_THROW(parser.parse_string("CREATE TABLE t(id - int8);"), parsing_error);
}

TEST(catalog__ddl_parser__test, fixed_size_array)
{
    parser_t parser;
    ASSERT_THROW(parser.parse_string("CREATE TABLE t (c INT32[2]);"), parsing_error);
    ASSERT_THROW(parser.parse_string("CREATE TABLE t (c INT32[1]);"), parsing_error);
    ASSERT_THROW(parser.parse_string("CREATE TABLE t (c INT32[0]);"), parsing_error);
}

TEST(catalog__ddl_parser__test, vector_of_strings)
{
    parser_t parser;
    ASSERT_THROW(parser.parse_string("CREATE TABLE t (c STRING[]);"), parsing_error);
}

TEST(catalog__ddl_parser__test, code_comments)
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

    ASSERT_NO_THROW(parser.parse_string(correct_ddl_text));
    EXPECT_EQ(1, parser.statements.size());

    ASSERT_THROW(parser.parse_string(incorrect_ddl_text), parsing_error);
}

TEST(catalog__ddl_parser__test, create_empty_table)
{
    parser_t parser;

    ASSERT_NO_THROW(parser.parse_string("CREATE TABLE t ();"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create_list);

    auto create_list = dynamic_cast<create_list_t*>(parser.statements[0].get());
    auto create_stmt = dynamic_cast<create_statement_t*>(create_list->statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_table);

    auto create_table = dynamic_cast<create_table_t*>(create_stmt);

    EXPECT_EQ(create_table->name, "t");
    EXPECT_FALSE(create_table->has_if_not_exists);
    EXPECT_EQ(create_table->fields.size(), 0);
}

TEST(catalog__ddl_parser__test, create_relationship)
{
    parser_t parser;

    const string ddl_text_full_db = R"(
CREATE RELATIONSHIP r (
  d1.t1.link1 -> d2.t2,
  d2.t2.link2 -> d1.t1
);
)";
    ASSERT_NO_THROW(parser.parse_string(ddl_text_full_db));
    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create_list);
    auto create_list = dynamic_cast<create_list_t*>(parser.statements[0].get());
    auto create_stmt = dynamic_cast<create_statement_t*>(create_list->statements[0].get());
    EXPECT_EQ(create_stmt->type, create_type_t::create_relationship);

    const string ddl_text_no_db = R"(
CREATE RELATIONSHIP r (
  t1.link1 -> t2,
  t2.link2 -> t1
);
)";
    ASSERT_NO_THROW(parser.parse_string(ddl_text_no_db));
    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create_list);
    create_list = dynamic_cast<create_list_t*>(parser.statements[0].get());
    create_stmt = dynamic_cast<create_statement_t*>(create_list->statements[0].get());
    EXPECT_EQ(create_stmt->type, create_type_t::create_relationship);

    const string ddl_text_partial_db = R"(
CREATE RELATIONSHIP r (
  d1.t1.link1 -> t2,
  t2.link2 -> d1.t1
);
)";
    ASSERT_NO_THROW(parser.parse_string(ddl_text_partial_db));
    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create_list);
    create_list = dynamic_cast<create_list_t*>(parser.statements[0].get());
    create_stmt = dynamic_cast<create_statement_t*>(create_list->statements[0].get());
    EXPECT_EQ(create_stmt->type, create_type_t::create_relationship);

    const string ddl_text_negative_case_no_name = R"(
CREATE RELATIONSHIP (
  t1.link1 -> t2,
  t2.link2 -> t1
);
)";
    ASSERT_THROW(parser.parse_string(ddl_text_negative_case_no_name), parsing_error);

    const string ddl_text_negative_case_single_link = R"(
CREATE RELATIONSHIP r (
  t1.link -> t2,
);
)";
    ASSERT_THROW(parser.parse_string(ddl_text_negative_case_single_link), parsing_error);
}

TEST(catalog__ddl_parser__test, create_index)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_string("CREATE INDEX IF NOT EXISTS idx ON d.t (name);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create_list);

    auto create_list = dynamic_cast<create_list_t*>(parser.statements[0].get());
    auto create_stmt = dynamic_cast<create_statement_t*>(create_list->statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_index);

    auto create_index = dynamic_cast<create_index_t*>(create_stmt);

    EXPECT_EQ(create_index->name, "idx");
    EXPECT_EQ(create_index->database, "d");
    EXPECT_EQ(create_index->index_table, "t");
    EXPECT_EQ(create_index->unique_index, false);
    EXPECT_EQ(create_index->index_type, index_type_t::range);
}

TEST(catalog__ddl_parser__test, create_unique_index)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_string("CREATE UNIQUE INDEX idx ON d.t (name);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create_list);

    auto create_list = dynamic_cast<create_list_t*>(parser.statements[0].get());
    auto create_stmt = dynamic_cast<create_statement_t*>(create_list->statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_index);

    auto create_index = dynamic_cast<create_index_t*>(create_stmt);

    EXPECT_EQ(create_index->name, "idx");
    EXPECT_EQ(create_index->database, "d");
    EXPECT_EQ(create_index->index_table, "t");
    EXPECT_EQ(create_index->unique_index, true);
    EXPECT_EQ(create_index->index_type, index_type_t::range);
}

TEST(catalog__ddl_parser__test, create_hash_index)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_string("CREATE UNIQUE HASH INDEX idx ON d.t (name);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create_list);

    auto create_list = dynamic_cast<create_list_t*>(parser.statements[0].get());
    auto create_stmt = dynamic_cast<create_statement_t*>(create_list->statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_index);

    auto create_index = dynamic_cast<create_index_t*>(create_stmt);

    EXPECT_EQ(create_index->name, "idx");
    EXPECT_EQ(create_index->database, "d");
    EXPECT_EQ(create_index->index_table, "t");
    EXPECT_EQ(create_index->unique_index, true);
    EXPECT_EQ(create_index->index_type, index_type_t::hash);
}

TEST(catalog__ddl_parser__test, create_range_index)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_string("CREATE RANGE INDEX IF NOT EXISTS idx ON d.t (name);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create_list);

    auto create_list = dynamic_cast<create_list_t*>(parser.statements[0].get());
    auto create_stmt = dynamic_cast<create_statement_t*>(create_list->statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_index);

    auto create_index = dynamic_cast<create_index_t*>(create_stmt);

    EXPECT_EQ(create_index->name, "idx");
    EXPECT_EQ(create_index->database, "d");
    EXPECT_EQ(create_index->index_table, "t");
    EXPECT_EQ(create_index->unique_index, false);
    EXPECT_EQ(create_index->index_type, index_type_t::range);
}

TEST(catalog__ddl_parser__test, create_one_to_one_relationship)
{
    parser_t parser;

    const string ddl_one_to_one = R"(
CREATE RELATIONSHIP r (
  d1.t1.link1 -> d2.t2,
  d2.t2.link2 -> d1.t1
);
)";

    ASSERT_NO_THROW(parser.parse_string(ddl_one_to_one));
    auto create_list = dynamic_cast<create_list_t*>(parser.statements[0].get());
    auto create_stmt = dynamic_cast<create_statement_t*>(create_list->statements[0].get());
    EXPECT_EQ(create_stmt->type, create_type_t::create_relationship);

    auto create_rel = dynamic_cast<create_relationship_t*>(create_stmt);

    auto parent = create_rel->relationship.first;
    auto child = create_rel->relationship.second;

    ASSERT_EQ(parent.name, "link1");
    ASSERT_EQ(parent.from_database, "d1");
    ASSERT_EQ(parent.from_table, "t1");
    ASSERT_EQ(parent.to_database, "d2");
    ASSERT_EQ(parent.to_table, "t2");
    ASSERT_EQ(parent.cardinality, cardinality_t::one);

    ASSERT_EQ(child.name, "link2");
    ASSERT_EQ(child.from_database, "d2");
    ASSERT_EQ(child.from_table, "t2");
    ASSERT_EQ(child.to_database, "d1");
    ASSERT_EQ(child.to_table, "t1");
    ASSERT_EQ(child.cardinality, cardinality_t::one);
}

TEST(catalog__ddl_parser__test, create_one_to_many_relationship)
{
    parser_t parser;

    const string ddl_one_to_one = R"(
CREATE RELATIONSHIP r (
  d1.t1.link1 -> d2.t2[],
  d2.t2.link2 -> d1.t1
);
)";

    ASSERT_NO_THROW(parser.parse_string(ddl_one_to_one));
    auto create_list = dynamic_cast<create_list_t*>(parser.statements[0].get());
    auto create_stmt = dynamic_cast<create_statement_t*>(create_list->statements[0].get());
    EXPECT_EQ(create_stmt->type, create_type_t::create_relationship);

    auto create_rel = dynamic_cast<create_relationship_t*>(create_stmt);

    auto parent = create_rel->relationship.first;
    auto child = create_rel->relationship.second;

    ASSERT_EQ(parent.name, "link1");
    ASSERT_EQ(parent.from_database, "d1");
    ASSERT_EQ(parent.from_table, "t1");
    ASSERT_EQ(parent.to_database, "d2");
    ASSERT_EQ(parent.to_table, "t2");
    ASSERT_EQ(parent.cardinality, cardinality_t::many);

    ASSERT_EQ(child.name, "link2");
    ASSERT_EQ(child.from_database, "d2");
    ASSERT_EQ(child.from_table, "t2");
    ASSERT_EQ(child.to_database, "d1");
    ASSERT_EQ(child.to_table, "t1");
    ASSERT_EQ(child.cardinality, cardinality_t::one);
}

TEST(catalog__ddl_parser__test, create_unique_field)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_string("CREATE TABLE t (id UINT64 UNIQUE, name STRING UNIQUE ACTIVE, ssn STRING ACTIVE UNIQUE);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create_list);

    auto create_list = dynamic_cast<create_list_t*>(parser.statements[0].get());
    auto create_stmt = dynamic_cast<create_statement_t*>(create_list->statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_table);

    auto create_table = dynamic_cast<create_table_t*>(create_stmt);

    EXPECT_EQ(create_table->name, "t");
    EXPECT_EQ(create_table->fields.size(), 3);

    const data_field_def_t* field;

    EXPECT_EQ(create_table->fields.at(0)->field_type, field_type_t::data);
    field = dynamic_cast<data_field_def_t*>(create_table->fields.at(0).get());
    EXPECT_EQ(field->name, "id");
    EXPECT_EQ(field->data_type, data_type_t::e_uint64);
    EXPECT_EQ(field->length, 1);
    EXPECT_EQ(field->active, false);
    EXPECT_EQ(field->unique, true);

    EXPECT_EQ(create_table->fields.at(1)->field_type, field_type_t::data);
    field = dynamic_cast<data_field_def_t*>(create_table->fields.at(1).get());
    EXPECT_EQ(field->name, "name");
    EXPECT_EQ(field->data_type, data_type_t::e_string);
    EXPECT_EQ(field->length, 1);
    EXPECT_EQ(field->active, true);
    EXPECT_EQ(field->unique, true);

    EXPECT_EQ(create_table->fields.at(2)->field_type, field_type_t::data);
    field = dynamic_cast<data_field_def_t*>(create_table->fields.at(2).get());
    EXPECT_EQ(field->name, "ssn");
    EXPECT_EQ(field->data_type, data_type_t::e_string);
    EXPECT_EQ(field->length, 1);
    EXPECT_EQ(field->active, true);
    EXPECT_EQ(field->unique, true);
}

TEST(catalog__ddl_parser__test, create_relationship_using_fields)
{
    parser_t parser;

    const string create_relationship_ddl = R"(
CREATE RELATIONSHIP r (
  d1.t1.link1 -> d2.t2[],
  d2.t2.link2 -> d1.t1,
  USING t1(c1), t2(c2)
);
)";

    ASSERT_NO_THROW(parser.parse_string(create_relationship_ddl));
    auto create_list = dynamic_cast<create_list_t*>(parser.statements[0].get());
    auto create_stmt = dynamic_cast<create_statement_t*>(create_list->statements[0].get());
    EXPECT_EQ(create_stmt->type, create_type_t::create_relationship);

    auto create_rel = dynamic_cast<create_relationship_t*>(create_stmt);

    auto parent = create_rel->relationship.first;
    auto child = create_rel->relationship.second;

    ASSERT_EQ(parent.name, "link1");
    ASSERT_EQ(parent.from_database, "d1");
    ASSERT_EQ(parent.from_table, "t1");
    ASSERT_EQ(parent.to_database, "d2");
    ASSERT_EQ(parent.to_table, "t2");
    ASSERT_EQ(parent.cardinality, cardinality_t::many);

    ASSERT_EQ(child.name, "link2");
    ASSERT_EQ(child.from_database, "d2");
    ASSERT_EQ(child.from_table, "t2");
    ASSERT_EQ(child.to_database, "d1");
    ASSERT_EQ(child.to_table, "t1");
    ASSERT_EQ(child.cardinality, cardinality_t::one);

    ASSERT_TRUE(create_rel->field_map);
    ASSERT_EQ(create_rel->field_map->first.table, "t1");
    ASSERT_EQ(create_rel->field_map->second.table, "t2");

    ASSERT_EQ(create_rel->field_map->first.fields.front(), "c1");
    ASSERT_EQ(create_rel->field_map->second.fields.front(), "c2");

    // Some negative test cases.
    const string negative_1 = R"(
CREATE RELATIONSHIP r (
  d1.t1.link1 -> d2.t2[],
  d2.t2.link2 -> d1.t1,
  USING t1(c1), t2(c2), t3(c3)
);
)";

    const string negative_2 = R"(
CREATE RELATIONSHIP r (
  d1.t1.link1 -> d2.t2[],
  d2.t2.link2 -> d1.t1,
  USING t1(c1), t2
);
)";

    const string negative_3 = R"(
CREATE RELATIONSHIP r (
  d1.t1.link1 -> d2.t2[],
  d2.t2.link2 -> d1.t1,
  USING t1(c1)
);
)";

    EXPECT_THROW(parser.parse_string(negative_1), parsing_error);
    EXPECT_THROW(parser.parse_string(negative_2), parsing_error);
    EXPECT_THROW(parser.parse_string(negative_3), parsing_error);
}

TEST(catalog__ddl_parser__test, create_statement_list)
{
    parser_t parser;

    const string create_list_ddl = R"(
CREATE RELATIONSHIP r (
  t1.link1 -> t2[],
  t2.link2 -> t1,
  USING t1(c1), t2(c2)
)
CREATE table t1 (
  c1 int32
)
CREATE table  t2(
  c2 int32
)
;
)";
    const string create_db_list_ddl = R"(
CREATE DATABASE d1
CREATE RELATIONSHIP r (
  t1.link1 -> t2[],
  t2.link2 -> t1,
  USING t1(c1), t2(c2)
)
CREATE table t1 (
  c1 int32
)
CREATE table  t2(
  c2 int32
)
;
)";

    ASSERT_NO_THROW(parser.parse_string(create_list_ddl));
    ASSERT_NO_THROW(parser.parse_string(create_db_list_ddl));

    // Negative test cases.
    const string negative_1 = R"(
CREATE RELATIONSHIP r (
  t1.link1 -> t2[],
  t2.link2 -> t1,
  USING t1(c1), t2(c2)
)
CREATE DATABASE d1
CREATE table t1 (
  c1 int32
)
CREATE table  t2(
  c2 int32
)
;
)";
    const string negative_2 = R"(
CREATE DATABASE d1
CREATE RELATIONSHIP r (
  t1.link1 -> t2[],
  t2.link2 -> t1,
  USING t1(c1), t2(c2)
)
CREATE DATABASE d2
CREATE table t1 (
  c1 int32
)
CREATE table  t2(
  c2 int32
)
;
)";
    const string negative_3 = R"(
CREATE RELATIONSHIP r (
  t1.link1 -> t2[],
  t2.link2 -> t1,
  USING t1(c1), t2(c2)
)
CREATE table t1 (
  c1 int32
)
CREATE table  t2(
  c2 int32
)
)";
    ASSERT_THROW(parser.parse_string(negative_1), parsing_error);
    ASSERT_THROW(parser.parse_string(negative_2), parsing_error);
    ASSERT_THROW(parser.parse_string(negative_3), parsing_error);
}

TEST(catalog__ddl_parser__test, in_table_relationship)
{
    parser_t parser;

    const string one_to_many_ddl = R"(
create table doctor (
 name string,
 phone_no string unique,
 patients references patient[]
)
create table patient (
 name string,
 doctor_phone_no string,
 doctor references doctor where doctor_phone_no = phone_no
);
)";
    const string hybrid_index_ddl = R"(
create table doctor (
 name string,
 email uint32 unique,
 primary_care_patients references patient[],
 secondary_care_patients references patient[]
)
create table patient (
 name string,
 primary_care_doctor_email uint32,
 secondary_care_doctor_email uint32,
 primary_care_doctor references doctor
  using primary_care_patients
  where patient.primary_care_doctor_email = doctor.email,
 secondary_care_doctor references doctor
  using secondary_care_patients
  where patient.secondary_care_doctor_email = doctor.email
);
)";

    ASSERT_NO_THROW(parser.parse_string(one_to_many_ddl));
    ASSERT_NO_THROW(parser.parse_string(hybrid_index_ddl));
}

TEST(catalog__ddl_parser__test, invalid_create_list)
{
    array ddls{
        R"(
-- referenced table name cannot contain the database name
create table t1(c1 int32, t2 references d.t2)
create table t2(c2 int32, t1 references d.t1);
)",
        R"(
-- database can only be the first statement
create table t1(c1 int32)
create database d
create table t2(c2 int32);
)",
    };

    for (const auto& ddl : ddls)
    {
        parser_t parser;
        ASSERT_THROW(parser.parse_string(ddl), parsing_error);
    }
}

TEST(catalog__ddl_parser__test, empty_statements)
{
    array ddls{
        R"(
-- empty statements in the front
;;;;
create table t(c int32);
)",
        R"(
-- empty statements in the middle
create table t1(c1 int32);
;;;;
create table t2(c2 int32);
)",
        R"(
-- empty statements in the end
create table t1(c1 int32);
create table t2(c2 int32);
;;;;
)",
    };

    for (const auto& ddl : ddls)
    {
        parser_t parser;
        ASSERT_NO_THROW(parser.parse_string(ddl));
    }
}

TEST(catalog__ddl_parser__test, create_optional_field)
{
    parser_t parser;
    ASSERT_NO_THROW(parser.parse_string("CREATE TABLE t ("
                                        "id UINT64 UNIQUE,"
                                        "name STRING OPTIONAL,"
                                        "ssn STRING OPTIONAL UNIQUE ACTIVE,"
                                        "address STRING ACTIVE OPTIONAL,"
                                        "weights UINT16[] OPTIONAL);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::create_list);

    auto create_list = dynamic_cast<create_list_t*>(parser.statements[0].get());
    auto create_stmt = dynamic_cast<create_statement_t*>(create_list->statements[0].get());

    EXPECT_EQ(create_stmt->type, create_type_t::create_table);

    auto create_table = dynamic_cast<create_table_t*>(create_stmt);

    EXPECT_EQ(create_table->name, "t");
    EXPECT_EQ(create_table->fields.size(), 5);

    const data_field_def_t* field;

    EXPECT_EQ(create_table->fields.at(0)->field_type, field_type_t::data);
    field = dynamic_cast<data_field_def_t*>(create_table->fields.at(0).get());
    EXPECT_EQ(field->name, "id");
    EXPECT_EQ(field->data_type, data_type_t::e_uint64);
    EXPECT_EQ(field->length, 1);
    EXPECT_EQ(field->active, false);
    EXPECT_EQ(field->unique, true);
    EXPECT_EQ(field->optional, false);

    EXPECT_EQ(create_table->fields.at(1)->field_type, field_type_t::data);
    field = dynamic_cast<data_field_def_t*>(create_table->fields.at(1).get());
    EXPECT_EQ(field->name, "name");
    EXPECT_EQ(field->data_type, data_type_t::e_string);
    EXPECT_EQ(field->length, 1);
    EXPECT_EQ(field->active, false);
    EXPECT_EQ(field->unique, false);
    EXPECT_EQ(field->optional, true);

    EXPECT_EQ(create_table->fields.at(2)->field_type, field_type_t::data);
    field = dynamic_cast<data_field_def_t*>(create_table->fields.at(2).get());
    EXPECT_EQ(field->name, "ssn");
    EXPECT_EQ(field->data_type, data_type_t::e_string);
    EXPECT_EQ(field->length, 1);
    EXPECT_EQ(field->active, true);
    EXPECT_EQ(field->unique, true);
    EXPECT_EQ(field->optional, true);

    EXPECT_EQ(create_table->fields.at(3)->field_type, field_type_t::data);
    field = dynamic_cast<data_field_def_t*>(create_table->fields.at(3).get());
    EXPECT_EQ(field->name, "address");
    EXPECT_EQ(field->data_type, data_type_t::e_string);
    EXPECT_EQ(field->length, 1);
    EXPECT_EQ(field->active, true);
    EXPECT_EQ(field->unique, false);
    EXPECT_EQ(field->optional, true);

    EXPECT_EQ(create_table->fields.at(4)->field_type, field_type_t::data);
    field = dynamic_cast<data_field_def_t*>(create_table->fields.at(4).get());
    EXPECT_EQ(field->name, "weights");
    EXPECT_EQ(field->data_type, data_type_t::e_uint16);
    EXPECT_EQ(field->length, 0);
    EXPECT_EQ(field->active, false);
    EXPECT_EQ(field->unique, false);
    EXPECT_EQ(field->optional, true);

    array false_ddls{
        R"(
create table t1(c1 int32, t2 optional int32)
)",
        R"(
create table t1(c1 int32, t2 references t2)
create table t2(c2 int32, t1 references t1 optional);
)",
    };

    for (const auto& ddl : false_ddls)
    {
        parser_t parser;
        ASSERT_THROW(parser.parse_string(ddl), parsing_error);
    }
}
