/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/catalog/ddl_execution.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <unordered_map>
#include <utility>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/ddl_executor.hpp"
#include "gaia_internal/common/retail_assert.hpp"

#include "gaia_parser.hpp"

namespace gaia
{
namespace catalog
{

/**
 * For a given create statement list starting from the given index, move all
 * statements of the given type to the front (starting from the index). Return
 * the index of the first element not of the give type after moving.
 */
size_t move_to_front(
    std::vector<std::unique_ptr<ddl::create_statement_t>>& statements,
    ddl::create_type_t type,
    size_t index = 0)
{
    ASSERT_PRECONDITION(index <= statements.size(), "Statement index out of bounds.");

    // Handle the two boundary conditions where the list is empty or the index
    // is right after the last statement.
    if (statements.size() == 0 || index == statements.size())
    {
        return statements.size();
    }

    // The index used to reverse iterate the list.
    size_t reverse_index = 1;
    do
    {
        // Advance index past statements of expected type.
        if (statements[index]->type == type)
        {
            index++;
            continue;
        }

        // Move back reverse index across statements of different type.
        if (statements[statements.size() - reverse_index]->type != type)
        {
            reverse_index++;
            continue;
        }

        // At this point index must point to a different type and reverse_index
        // to the expected type, so we'll exchange the statements.
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
void rearrange_create_list_statements(
    std::vector<std::unique_ptr<ddl::create_statement_t>>& statements)
{
    // Skip the first create database statement if it exists.
    size_t index = statements.front()->type == ddl::create_type_t::create_database ? 1 : 0;
    // Move all create table statements to the front.
    index = move_to_front(statements, ddl::create_type_t::create_table, index);
    // Move all create index statements before create relationship statements.
    move_to_front(statements, ddl::create_type_t::create_index, index);
}

/**
 * Convert in-table relationship definitions (references) into standalone
 * `create relationship` definitions.
 */
void convert_references_to_relationships(
    std::vector<std::unique_ptr<ddl::create_statement_t>>& statements)
{
    // The map tracks the seen references in a given table (whose name will be
    // the key of the map) without the corresponding matching references yet.
    std::multimap<std::string, ddl::ref_field_def_t*> table_refs;

    for (auto& stmt : statements)
    {
        // Skip statements other than `create table`.
        if (stmt->type != ddl::create_type_t::create_table)
        {
            continue;
        }

        auto create_table = dynamic_cast<ddl::create_table_t*>(stmt.get());
        for (auto& field : create_table->fields)
        {
            // Skip non-reference field(s).
            if (field->field_type != ddl::field_type_t::reference)
            {
                continue;
            }

            auto ref_field = dynamic_cast<ddl::ref_field_def_t*>(field.get());

            // We have not seen the referenced table so far; add the reference
            // definition to the seen reference map and continue.
            if (table_refs.count(ref_field->table) == 0)
            {
                table_refs.emplace(create_table->name, ref_field);
                continue;
            }

            // We have seen the referenced table definition and should be able
            // to find the matching reference definition for the current
            // reference definition at this point.
            auto candidate_range = table_refs.equal_range(ref_field->table);
            auto matching_iter = table_refs.end();
            for (auto it = candidate_range.first; it != candidate_range.second; ++it)
            {
                ddl::ref_field_def_t* candidate_ref = it->second;

                // The reference definitions are considered a match when any of
                // the following conditions are met.
                // - Both references do not contain a matching field.
                // - One reference definition has a matching field that matches
                //   the other while the other does not have a matching field.
                // - Both references have a matching field that matches the other.
                if ((ref_field->field.empty() && candidate_ref->field.empty())
                    || (ref_field->field.empty() && candidate_ref->field == ref_field->name)
                    || (candidate_ref->field.empty() && ref_field->field == candidate_ref->name)
                    || (!ref_field->field.empty() && !candidate_ref->field.empty()
                        && candidate_ref->field == ref_field->name
                        && ref_field->field == candidate_ref->name))
                {
                    if (matching_iter == table_refs.end())
                    {
                        matching_iter = it;
                        continue;
                    }

                    // We found another candidate that can match the given
                    // reference. This tells us the previously matched reference
                    // is an ambiguous definition. Report it (instead of other
                    // candidates) as ambiguous because it appears earlier.
                    throw ambiguous_reference_definition(ref_field->table, matching_iter->second->name);
                }
            }

            // We cannot find any matching references for this definition.
            if (matching_iter == table_refs.end())
            {
                throw orphaned_reference_definition(create_table->name, ref_field->name);
            }

            // Create a standalone `create relationship` definition by combining
            // the two matching reference definitions.
            //
            ddl::ref_field_def_t* matching_ref = matching_iter->second;

            // Many to many is not supported at the moment.
            if (matching_ref->cardinality == relationship_cardinality_t::many
                && ref_field->cardinality == relationship_cardinality_t::many)
            {
                throw many_to_many_not_supported(ref_field->table, matching_ref->table);
            }

            // Use the [link1]_[link2] as the relationship name.
            // TODO: Detect name conflict. [GATAPLAT-306]
            std::string rel_name
                = (matching_ref->cardinality == relationship_cardinality_t::many
                       ? ref_field->name + "_" + matching_ref->name
                       : matching_ref->name + "_" + ref_field->name);

            auto create_relationship = std::make_unique<ddl::create_relationship_t>(rel_name);

            create_relationship->relationship = std::make_pair<ddl::link_def_t, ddl::link_def_t>(
                {"", ref_field->table, matching_ref->name, "", create_table->name, matching_ref->cardinality},
                {"", create_table->name, ref_field->name, "", ref_field->table, ref_field->cardinality});

            create_relationship->if_not_exists = false;

            // TODO: Detect conflict in field map definitions when both
            //       `references` definitions contain one. [GAIAPLAT-306]
            if (matching_ref->field_map)
            {
                create_relationship->field_map = matching_ref->field_map;
            }
            if (ref_field->field_map)
            {
                create_relationship->field_map = ref_field->field_map;
            }

            // Append the new relationship definition to the statement list.
            statements.emplace_back(std::move(create_relationship));

            // Remove the matched reference definition.
            table_refs.erase(matching_iter);
        }
    }

    // There are references left unmatched. Report the first seen one.
    if (table_refs.size() > 0)
    {
        throw orphaned_reference_definition(table_refs.begin()->first, table_refs.begin()->second->name);
    }
}

void execute_create_statement_no_txn(
    ddl_executor_t& executor,
    ddl::create_statement_t* create_stmt)
{
    bool throw_on_exist = true;
    if (create_stmt->if_not_exists)
    {
        throw_on_exist = false;
    }

    if (create_stmt->type == ddl::create_type_t::create_table)
    {
        auto create_table_stmt = dynamic_cast<ddl::create_table_t*>(create_stmt);
        executor.create_table(
            create_table_stmt->database,
            create_table_stmt->name,
            create_table_stmt->fields,
            throw_on_exist);
    }
    else if (create_stmt->type == ddl::create_type_t::create_database)
    {
        executor.create_database(create_stmt->name, throw_on_exist);
    }
    else if (create_stmt->type == ddl::create_type_t::create_relationship)
    {
        auto create_relationship_stmt = dynamic_cast<ddl::create_relationship_t*>(create_stmt);
        executor.create_relationship(
            create_relationship_stmt->name,
            create_relationship_stmt->relationship.first,
            create_relationship_stmt->relationship.second,
            create_relationship_stmt->field_map,
            throw_on_exist);
    }
    else if (create_stmt->type == ddl::create_type_t::create_index)
    {
        auto create_index_stmt = dynamic_cast<ddl::create_index_t*>(create_stmt);
        executor.create_index(
            create_index_stmt->name,
            create_index_stmt->unique_index,
            create_index_stmt->index_type,
            create_index_stmt->database,
            create_index_stmt->index_table,
            create_index_stmt->index_fields,
            throw_on_exist);
    }
}

// The `create list` parsing results are stored in the same classes as the
// standalone statements. We will impose some additional requirements here.
//
// - Only the first statement can be `create database` when there is one. When
//   the first statement is a `create database` statement, the rest entities
//   will be created in that database.
//
// - No statement can contain database names in identifiers such as
//   `[db_name].[table_name]`.
//
void sanity_check_create_list_statements(
    std::vector<std::unique_ptr<ddl::create_statement_t>>& statements)
{
    ASSERT_PRECONDITION(statements.size() > 1, "The list must contain more than one statement.");

    size_t index = 0;
    for (auto& stmt : statements)
    {
        if (stmt->type == ddl::create_type_t::create_database && index > 0)
        {
            throw invalid_create_list("CREATE DATABASE should be the first statement.");
        }
        else if (stmt->type == ddl::create_type_t::create_table)
        {
            auto create_table = dynamic_cast<ddl::create_table_t*>(stmt.get());
            if (!create_table->database.empty())
            {
                throw invalid_create_list("CREATE TABLE should not specify a database.");
            }
        }
        else if (stmt->type == ddl::create_type_t::create_index)
        {
            auto create_index = dynamic_cast<ddl::create_index_t*>(stmt.get());
            if (!create_index->database.empty())
            {
                throw invalid_create_list("CREATE INDEX should not specify a database.");
            }
        }
        else if (stmt->type == ddl::create_type_t::create_relationship)
        {
            auto create_relationship = dynamic_cast<ddl::create_relationship_t*>(stmt.get());
            if (!create_relationship->relationship.first.from_database.empty()
                || !create_relationship->relationship.second.from_database.empty()
                || !create_relationship->relationship.first.to_database.empty()
                || !create_relationship->relationship.second.to_database.empty())
            {
                throw invalid_create_list("CREATE RELATIONSHIP should not specify a database in the link(s).");
            }
            if (create_relationship->field_map
                && (!create_relationship->field_map->first.database.empty()
                    || !create_relationship->field_map->second.database.empty()))
            {
                throw invalid_create_list("CREATE RELATIONSHIP should not specify a database in the field(s).");
            }
        }

        index++;
    }
}

void execute_create_list_statements(
    std::vector<std::unique_ptr<ddl::create_statement_t>>& statements)
{
    ASSERT_PRECONDITION(statements.size() > 1, "The list must contain more than one statement.");

    sanity_check_create_list_statements(statements);

    rearrange_create_list_statements(statements);

    ddl_executor_t& executor = ddl_executor_t::get();
    direct_access::auto_transaction_t txn(false);
    for (auto& stmt : statements)
    {
        auto create_stmt = dynamic_cast<ddl::create_statement_t*>(stmt.get());
        execute_create_statement_no_txn(executor, create_stmt);
        if (create_stmt->type == ddl::create_type_t::create_database)
        {
            use_database(create_stmt->name);
        }
    }
    txn.commit();
}

void execute(std::vector<std::unique_ptr<ddl::statement_t>>& statements)
{
    for (auto& stmt : statements)
    {
        if (stmt->is_type(ddl::statement_type_t::create_list))
        {
            auto create_list = dynamic_cast<ddl::create_list_t*>(stmt.get());

            ASSERT_INVARIANT(statements.size() >= 1, "The list must contain at least one statement.");

            // To handle forward references in `create list` statements,
            // preprocess all references in `create table` statements and
            // convert them into standalone `create relationship` statements;
            // when there are more than one statement, rearrange the statements
            // into an order that can be executed sequentially.
            //
            convert_references_to_relationships(create_list->statements);

            if (create_list->statements.size() == 1)
            {
                ddl_executor_t& executor = ddl_executor_t::get();
                direct_access::auto_transaction_t txn(false);
                execute_create_statement_no_txn(executor, create_list->statements.front().get());
                txn.commit();
            }
            else
            {
                execute_create_list_statements(create_list->statements);
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
