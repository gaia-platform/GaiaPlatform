/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once

#include <filesystem>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/common/retail_assert.hpp"

#include "gaia_parser.hpp"

namespace gaia
{
namespace catalog
{

inline void execute(const std::string& db_name, std::vector<std::unique_ptr<ddl::statement_t>>& statements)
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
            else if (create_stmt->type == ddl::create_type_t::create_relationship)
            {
                create_relationship(create_stmt->name, create_stmt->relationship.first, create_stmt->relationship.second);
            }
        }
        else if (stmt->is_type(ddl::statement_type_t::drop))
        {
            auto drop_stmt = dynamic_cast<ddl::drop_statement_t*>(stmt.get());
            if (drop_stmt->type == ddl::drop_type_t::drop_table)
            {
                if (!drop_stmt->database.empty())
                {
                    drop_table(drop_stmt->database, drop_stmt->name);
                }
                else
                {
                    drop_table(db_name, drop_stmt->name);
                }
            }
            else if (drop_stmt->type == ddl::drop_type_t::drop_database)
            {
                drop_database(drop_stmt->name);
            }
        }
    }
}

inline std::string get_db_name_from_filename(const std::string& ddl_filename)
{
    std::string db_name = ddl_filename;
    if (db_name.find('/') != std::string::npos)
    {
        db_name = db_name.substr(db_name.find_last_of('/') + 1);
    }
    if (db_name.find('.') != std::string::npos)
    {
        db_name = db_name.substr(0, db_name.find_last_of('.'));
    }
    return db_name;
}

inline void load_catalog(
    ddl::parser_t& parser, const std::string& ddl_filename,
    const std::string& db_name, bool create_db = false)
{
    common::retail_assert(!ddl_filename.empty(), "No DDL file specified.");
    common::retail_assert(!db_name.empty(), "No database specified.");

    auto file_path = std::filesystem::path(ddl_filename);

    if (!std::filesystem::exists(file_path) && std::filesystem::is_directory(file_path))
    {
        throw std::invalid_argument("Invalid DDL file: '" + std::string(file_path.c_str()) + "'.");
    }

    int parsing_result = parser.parse(file_path.string());
    common::retail_assert(parsing_result == EXIT_SUCCESS, "Failed to parse the DDL file '" + ddl_filename + "'.");

    if (create_db)
    {
        create_database(db_name, false);
    }

    execute(db_name, parser.statements);
}

inline void load_catalog(const char* ddl_filename)
{
    ddl::parser_t parser;
    std::string filename(ddl_filename);
    std::string db_name = get_db_name_from_filename(ddl_filename);
    load_catalog(parser, filename, db_name, true);
}

} // namespace catalog
} // namespace gaia
