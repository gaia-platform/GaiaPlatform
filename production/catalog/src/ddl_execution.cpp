/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "gaia_internal/catalog/ddl_execution.hpp"

#include <filesystem>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/common/retail_assert.hpp"

#include "gaia_parser.hpp"

namespace gaia
{
namespace catalog
{

/**
 * For a given create statement list starting from the given index, move all
 * statements of the given type to the front.
 */
size_t move_to_front(
    std::vector<std::unique_ptr<gaia::catalog::ddl::create_statement_t>>& statements,
    ddl::create_type_t type,
    size_t index = 0)
{
    if (statements.size() == 0 || index >= statements.size())
    {
        return index;
    }
    // The index used to reverse iterate the list.
    size_t reverse_index = 1;
    do
    {
        if (statements[index]->type == type)
        {
            index++;
            continue;
        }
        if (statements[statements.size() - reverse_index]->type != type)
        {
            reverse_index++;
            continue;
        }
        if (index < (statements.size() - reverse_index))
        {
            statements[index].swap(statements[statements.size() - reverse_index]);
            index++;
            reverse_index++;
        }
    } while (index + reverse_index < statements.size());
    return index;
}

/**
 * Rearrange the statements into the three buckets in the order of 1) create
 * table statements, 2) create index statements, and 3) create relationship
 * statements.
 */
void rearrange_create_statement_list(
    std::vector<std::unique_ptr<gaia::catalog::ddl::create_statement_t>>& statements)
{
    // Move all create table statements to the front.
    size_t index = move_to_front(statements, ddl::create_type_t::create_table);
    // Move all create index statements before create relationship statements.
    move_to_front(statements, ddl::create_type_t::create_index, index);
}

void execute(std::vector<std::unique_ptr<ddl::statement_t>>& statements)
{
    for (auto& stmt : statements)
    {
        if (stmt->is_type(ddl::statement_type_t::create_list))
        {
            auto create_list = dynamic_cast<ddl::create_list_t*>(stmt.get());

            // TODO: preprocess the list to transform in-table relationship
            //       definition into standalone create relationship statements.

            rearrange_create_statement_list(create_list->statements);

            for (auto& sub_stmt : create_list->statements)
            {
                auto create_stmt = dynamic_cast<ddl::create_statement_t*>(sub_stmt.get());
                bool throw_on_exist = true;
                if (create_stmt->if_not_exists)
                {
                    throw_on_exist = false;
                }
                if (create_stmt->type == ddl::create_type_t::create_table)
                {
                    auto create_table_stmt = dynamic_cast<ddl::create_table_t*>(sub_stmt.get());
                    create_table(
                        create_table_stmt->database,
                        create_table_stmt->name,
                        create_table_stmt->fields,
                        throw_on_exist);
                }
                else if (create_stmt->type == ddl::create_type_t::create_database)
                {
                    create_database(create_stmt->name, throw_on_exist);
                }
                else if (create_stmt->type == ddl::create_type_t::create_relationship)
                {
                    auto create_relationship_stmt = dynamic_cast<ddl::create_relationship_t*>(sub_stmt.get());
                    create_relationship(
                        create_relationship_stmt->name,
                        create_relationship_stmt->relationship.first,
                        create_relationship_stmt->relationship.second,
                        create_relationship_stmt->field_map,
                        throw_on_exist);
                }
                else if (create_stmt->type == ddl::create_type_t::create_index)
                {
                    auto create_index_stmt = dynamic_cast<ddl::create_index_t*>(sub_stmt.get());
                    create_index(
                        create_index_stmt->name,
                        create_index_stmt->unique_index,
                        create_index_stmt->index_type,
                        create_index_stmt->database,
                        create_index_stmt->index_table,
                        create_index_stmt->index_fields,
                        throw_on_exist);
                }
            }
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
            else if (drop_stmt->type == ddl::drop_type_t::drop_relationship)
            {
                drop_relationship(drop_stmt->name, !drop_stmt->if_exists);
            }
            else if (drop_stmt->type == ddl::drop_type_t::drop_index)
            {
                drop_index(drop_stmt->name, !drop_stmt->if_exists);
            }
        }
        else if (stmt->is_type(ddl::statement_type_t::use))
        {
            auto use_stmt = dynamic_cast<ddl::use_statement_t*>(stmt.get());
            use_database(use_stmt->name);
        }
    }
}

void load_catalog(ddl::parser_t& parser, const std::string& ddl_filename)
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

void load_catalog(const char* ddl_filename)
{
    ddl::parser_t parser;
    std::string filename(ddl_filename);
    load_catalog(parser, filename);
}

} // namespace catalog
} // namespace gaia
