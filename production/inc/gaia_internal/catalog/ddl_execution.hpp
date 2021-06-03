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

inline void execute(std::vector<std::unique_ptr<ddl::statement_t>>& statements)
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
                create_table(create_stmt->database, create_stmt->name, create_stmt->fields, throw_on_exist);
            }
            else if (create_stmt->type == ddl::create_type_t::create_database)
            {
                create_database(create_stmt->name, throw_on_exist);
            }
            else if (create_stmt->type == ddl::create_type_t::create_relationship)
            {
                create_relationship(
                    create_stmt->name,
                    create_stmt->relationship.first,
                    create_stmt->relationship.second,
                    throw_on_exist);
            }
            else if (create_stmt->type == ddl::create_type_t::create_index)
            {
                create_index(
                    create_stmt->name,
                    create_stmt->unique_index,
                    create_stmt->index_type,
                    create_stmt->database,
                    create_stmt->index_table,
                    create_stmt->index_fields,
                    throw_on_exist);
            }
            else if (stmt->is_type(ddl::statement_type_t::drop))
            {
                auto drop_stmt = dynamic_cast<ddl::drop_statement_t*>(stmt.get());
                if (drop_stmt->type == ddl::drop_type_t::drop_table)
                {
                    drop_table(drop_stmt->database, drop_stmt->name, !drop_stmt->if_exists);
                }
                else if (drop_stmt->type == ddl::drop_type_t::drop_database)
                {
                    drop_database(drop_stmt->name, !drop_stmt->if_exists);
                }
            }
        }
        else if (stmt->is_type(ddl::statement_type_t::use))
        {
            auto use_stmt = dynamic_cast<ddl::use_statement_t*>(stmt.get());
            use_database(use_stmt->name);
        }
    }
}

inline void load_catalog(ddl::parser_t& parser, const std::string& ddl_filename)
{
    ASSERT_PRECONDITION(!ddl_filename.empty(), "No DDL file specified.");

    auto file_path = std::filesystem::path(ddl_filename);

    if (!std::filesystem::exists(file_path) && std::filesystem::is_directory(file_path))
    {
        throw std::invalid_argument("Invalid DDL file: '" + std::string(file_path.c_str()) + "'.");
    }

    parser.parse(file_path.string());
    execute(parser.statements);
}

inline void load_catalog(const char* ddl_filename)
{
    ddl::parser_t parser;
    std::string filename(ddl_filename);
    load_catalog(parser, filename);
}

} // namespace catalog
} // namespace gaia
