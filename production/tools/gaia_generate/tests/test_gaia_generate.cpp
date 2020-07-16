/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <fstream>
#include <iostream>
#include "gtest/gtest.h"
#include "catalog_manager.hpp"
#include "gaia_parser.hpp"

using namespace gaia::catalog;
using namespace std;

class gaia_generate_test : public ::testing::Test {
protected:
    void SetUp() override {
        // gaia_mem_base::init(true);
    }

    void TearDown() override {
        // Delete the shared memory segments.
        // gaia_mem_base::reset();
        catalog_manager_t::get().reset();
    }
};

// Copied from gaiac main.cpp
void execute(vector<unique_ptr<statement_t>> &statements) {
    for (auto &stmt : statements) {
        if (!stmt->is_type(statement_type_t::CREATE)) {
            continue;
        }
        auto createStmt = dynamic_cast<create_statement_t *>(stmt.get());
        if (createStmt->type == create_type_t::CREATE_TABLE) {
            gaia::catalog::create_table(createStmt->table_name, createStmt->fields);
        }
    }
}

// Using the catalog manager's create_table(), create a catalog and an EDC header from that.
TEST_F(gaia_generate_test, use_create_table) {
    ddl::field_def_list_t fields;
    fields.push_back(unique_ptr<ddl::field_definition_t>(new ddl::field_definition_t{"name", ddl::data_type_t::STRING, 1}));
    auto airport_id = catalog_manager_t::get().create_table("airport", fields);
    EXPECT_NE(0,airport_id);

    auto header_str = gaia_generate("airport");
    EXPECT_NE(0,header_str.find("struct airport_t"));
}

// Start from Gaia DDL to create an EDC header.
TEST_F(gaia_generate_test, parse_ddl) {
    parser_t parser;

    // Create a very small DDL file.
    ofstream ddl_file("tmp_airport_remove_me.ddl");
    ddl_file << "create table airport ( name string );" << endl;
    ddl_file.close();
    EXPECT_EQ(false,parser.parse("tmp_airport_remove_me.ddl"));
    unlink("tmp_airport_remove_me.ddl");
    execute(parser.statements);

    auto header_str = gaia_generate("airport");
    EXPECT_NE(0,header_str.find("struct airport_t"));
}
