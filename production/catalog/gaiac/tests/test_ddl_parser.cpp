/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_catalog.hpp"
#include "driver.hpp"
#include "lexer.h"
#include "parser.hpp"
#include "gtest/gtest.h"

using namespace gaia::catalog::ddl;

TEST(catalog_ddl_parser_test, create_table) {
    driver drv;
    yy_scan_string("CREATE TABLE t (c INT);");
    yy::parser parse(drv);
    EXPECT_EQ(EXIT_SUCCESS, parse());
    EXPECT_EQ(1, drv.statements.size());
    EXPECT_EQ(drv.statements[0]->type(), StatementType::kStmtCreate);
    CreateStatement *createStmt =
        reinterpret_cast<CreateStatement *>(drv.statements[0]);
    EXPECT_EQ(createStmt->type, CreateType::kCreateTable);
    EXPECT_EQ(createStmt->tableName, "t");
    yylex_destroy();
}

TEST(catalog_ddl_parser_test, create_type) {
    driver drv;
    yy_scan_string("CREATE TYPE t (c INT);");
    yy::parser parse(drv);
    EXPECT_EQ(EXIT_SUCCESS, parse());
    EXPECT_EQ(1, drv.statements.size());
    EXPECT_EQ(drv.statements[0]->type(), StatementType::kStmtCreate);
    CreateStatement *createStmt =
        reinterpret_cast<CreateStatement *>(drv.statements[0]);
    EXPECT_EQ(createStmt->type, CreateType::kCreateType);
    EXPECT_EQ(createStmt->typeName, "t");
    yylex_destroy();
}

TEST(catalog_ddl_parser_test, create_table_as) {
    driver drv;
    yy_scan_string("CREATE TYPE t (c INT); CREATE TABLE tt OF t;");
    yy::parser parse(drv);
    EXPECT_EQ(EXIT_SUCCESS, parse());
    EXPECT_EQ(2, drv.statements.size());
    EXPECT_EQ(drv.statements[0]->type(), StatementType::kStmtCreate);
    CreateStatement *createStmt =
        reinterpret_cast<CreateStatement *>(drv.statements[0]);
    EXPECT_EQ(createStmt->type, CreateType::kCreateType);
    EXPECT_EQ(createStmt->typeName, "t");
    EXPECT_EQ(drv.statements[1]->type(), StatementType::kStmtCreate);
    createStmt = reinterpret_cast<CreateStatement *>(drv.statements[1]);
    EXPECT_EQ(createStmt->type, CreateType::kCreateTableAs);
    EXPECT_EQ(createStmt->tableName, "tt");
    EXPECT_EQ(createStmt->typeName, "t");
    yylex_destroy();
}

TEST(catalog_ddl_parser_test, create_table_multiple_fields) {
    driver drv;
    yy_scan_string("CREATE TABLE t (c1 INT[], c2 DOUBLE[2], c3 tt, c4 tt[3]);");
    yy::parser parse(drv);
    EXPECT_EQ(EXIT_SUCCESS, parse());
    EXPECT_EQ(1, drv.statements.size());
    EXPECT_EQ(drv.statements[0]->type(), StatementType::kStmtCreate);
    CreateStatement *createStmt =
        reinterpret_cast<CreateStatement *>(drv.statements[0]);
    EXPECT_EQ(createStmt->type, CreateType::kCreateTable);
    EXPECT_EQ(createStmt->tableName, "t");
    EXPECT_EQ(createStmt->fields->size(), 4);

    EXPECT_EQ(createStmt->fields->at(0)->name, "c1");
    EXPECT_EQ(createStmt->fields->at(0)->type, DataType::INT);
    EXPECT_EQ(createStmt->fields->at(0)->length, 0);

    EXPECT_EQ(createStmt->fields->at(1)->name, "c2");
    EXPECT_EQ(createStmt->fields->at(1)->type, DataType::DOUBLE);
    EXPECT_EQ(createStmt->fields->at(1)->length, 2);

    EXPECT_EQ(createStmt->fields->at(2)->name, "c3");
    EXPECT_EQ(createStmt->fields->at(2)->type, DataType::TABLE);
    EXPECT_EQ(createStmt->fields->at(2)->length, 1);

    EXPECT_EQ(createStmt->fields->at(3)->name, "c4");
    EXPECT_EQ(createStmt->fields->at(3)->type, DataType::TABLE);
    EXPECT_EQ(createStmt->fields->at(3)->length, 3);
    yylex_destroy();
}
