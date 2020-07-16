/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_catalog.hpp"
#include "gaia_parser.hpp"
#include "yy_parser.hpp"
#include "gtest/gtest.h"

using namespace gaia::catalog::ddl;

TEST(catalog_ddl_parser_test, create_table) {
    parser_t parser;
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("CREATE TABLE t (c INT32);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::CREATE);

    auto createStmt = dynamic_cast<create_statement_t *>(parser.statements[0].get());

    EXPECT_EQ(createStmt->type, create_type_t::CREATE_TABLE);
    EXPECT_EQ(createStmt->table_name, "t");
}

TEST(catalog_ddl_parser_test, create_table_multiple_fields) {
    parser_t parser;
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("CREATE TABLE t (c1 INT32[], c2 FLOAT64[2]);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::CREATE);

    auto createStmt = dynamic_cast<create_statement_t *>(parser.statements[0].get());

    EXPECT_EQ(createStmt->type, create_type_t::CREATE_TABLE);
    EXPECT_EQ(createStmt->table_name, "t");
    EXPECT_EQ(createStmt->fields.size(), 2);

    EXPECT_EQ(createStmt->fields.at(0)->name, "c1");
    EXPECT_EQ(createStmt->fields.at(0)->type, data_type_t::INT32);
    EXPECT_EQ(createStmt->fields.at(0)->length, 0);

    EXPECT_EQ(createStmt->fields.at(1)->name, "c2");
    EXPECT_EQ(createStmt->fields.at(1)->type, data_type_t::FLOAT64);
    EXPECT_EQ(createStmt->fields.at(1)->length, 2);
}

TEST(catalog_ddl_parser_test, create_table_references) {
    parser_t parser;
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("CREATE TABLE t (c1 REFERENCES t1, c2 REFERENCES t2[3], c3 REFERENCES t3[]);"));

    EXPECT_EQ(1, parser.statements.size());
    EXPECT_EQ(parser.statements[0]->type(), statement_type_t::CREATE);

    auto createStmt = dynamic_cast<create_statement_t *>(parser.statements[0].get());

    EXPECT_EQ(createStmt->type, create_type_t::CREATE_TABLE);
    EXPECT_EQ(createStmt->table_name, "t");
    EXPECT_EQ(createStmt->fields.size(), 3);

    EXPECT_EQ(createStmt->fields.at(0)->name, "c1");
    EXPECT_EQ(createStmt->fields.at(0)->type, data_type_t::REFERENCES);
    EXPECT_EQ(createStmt->fields.at(0)->length, 1);

    EXPECT_EQ(createStmt->fields.at(1)->name, "c2");
    EXPECT_EQ(createStmt->fields.at(1)->type, data_type_t::REFERENCES);
    EXPECT_EQ(createStmt->fields.at(1)->length, 3);

    EXPECT_EQ(createStmt->fields.at(2)->name, "c3");
    EXPECT_EQ(createStmt->fields.at(2)->type, data_type_t::REFERENCES);
    EXPECT_EQ(createStmt->fields.at(2)->length, 0);
}

TEST(catalog_ddl_parser_test, case_sensitivity) {
    parser_t parser;
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("CREATE TABLE t (c INT32);"));
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("create table t (c int32);"));
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("cReAte taBle T (c int32);"));
    ASSERT_EQ(EXIT_SUCCESS, parser.parse_line("CREATE TABLE T (c int32, C int32);"));
}
