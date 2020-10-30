/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include "catalog.hpp"
#include "gaia_parser.hpp"
#include "retail_assert.hpp"

namespace gaia
{
namespace catalog
{

void execute(const string& db_name, vector<unique_ptr<ddl::statement_t>>& statements)
{
    for (auto& stmt : statements)
    {
        if (stmt->is_type(ddl::statement_type_t::create))
        {
            auto create_stmt = dynamic_cast<ddl::create_statement_t*>(stmt.get());
            bool throw_on_exist = true;
            if (create_stmt->if_not_exists)
            {
                throw_on_exist = false;
            }
            if (create_stmt->type == ddl::create_type_t::create_table)
            {
                if (!create_stmt->database.empty())
                {
                    create_table(create_stmt->database, create_stmt->name, create_stmt->fields, throw_on_exist);
                }
                else
                {
                    create_table(db_name, create_stmt->name, create_stmt->fields, throw_on_exist);
                }
            }
            else if (create_stmt->type == ddl::create_type_t::create_database)
            {
                create_database(create_stmt->name, throw_on_exist);
            }
        }
        else if (stmt->is_type(ddl::statement_type_t::drop))
        {
            auto drop_stmt = dynamic_cast<ddl::drop_statement_t*>(stmt.get());
            if (drop_stmt->type == ddl::drop_type_t::drop_table)
            {
                drop_table(drop_stmt->database, drop_stmt->name);
            }
            else if (drop_stmt->type == ddl::drop_type_t::drop_database)
            {
                drop_database(drop_stmt->name);
            }
        }
    }
}

string load_catalog(ddl::parser_t& parser, const string& ddl_filename, const string& name)
{
    string db(name);
    common::retail_assert(!ddl_filename.empty(), "No ddl file specified.");

    int parsing_result = parser.parse(ddl_filename);
    common::retail_assert(parsing_result == EXIT_SUCCESS, "Fail to parse the ddl file '" + ddl_filename + "'");

    if (db.empty())
    {
        // Strip off the path and any suffix to get database name if database name is not specified.
        db = ddl_filename;
        if (db.find("/") != string::npos)
        {
            db = db.substr(db.find_last_of("/") + 1);
        }
        if (db.find(".") != string::npos)
        {
            db = db.substr(0, db.find_last_of("."));
        }
        create_database(db, false);
    }

    execute(db, parser.statements);
    return db;
}

void load_catalog(const char* ddl_filename)
{
    ddl::parser_t parser;
    string filename(ddl_filename);
    string db_name;
    db_name = load_catalog(parser, filename, db_name);
}

} // namespace catalog
} // namespace gaia
