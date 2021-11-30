/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/catalog/ddl_execution.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <utility>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/ddl_executor.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/exceptions.hpp"

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

void check_reference_field_maps(
    std::optional<ddl::table_field_map_t>& field_map1,
    std::optional<ddl::table_field_map_t>& field_map2,
    std::string& table1,
    std::string& table2)
{
    ASSERT_PRECONDITION(!table1.empty() && !table2.empty(), "Cannot verify without both table names.");

    // Nothing to verify when both are undefined.
    if (!field_map1 && !field_map2)
    {
        return;
    }

    // When both are defined, they need to be equivalent.
    if (field_map1 && field_map2)
    {
        if (!((field_map1->first == field_map2->first && field_map1->second == field_map2->second)
              || (field_map1->first == field_map2->second && field_map1->second == field_map2->first)))
        {
            throw invalid_relationship_field_internal(
                "The matching reference definitions in table '" + table1 + "' and table '" + table2 + "'"
                + " both contain WHERE clauses that do not match each other.");
        }
    }

    // Pick any one that is defined to verify. If both are defined, we have
    // verified they are equivalent. We still only need to verify one.
    ddl::table_field_map_t& field_map = field_map1 ? field_map1.value() : field_map2.value();

    // Make sure tables on both side match corresponding table names.
    if (!((field_map.first.table == table1 && field_map.second.table == table2)
          || (field_map.first.table == table2 && field_map.second.table == table1)))
    {
        throw invalid_relationship_field_internal(
            "The WHERE clause for matching reference definitions in table '" + table1
            + "' and table '" + table2 + "'" + " must use a field from each table on opposite sides of '='.");
    }
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

    std::vector<std::unique_ptr<ddl::create_relationship_t>> relationships;

    for (auto& statement : statements)
    {
        // Skip statements other than `create table`.
        if (statement->type != ddl::create_type_t::create_table)
        {
            continue;
        }

        auto create_table = dynamic_cast<ddl::create_table_t*>(statement.get());
        for (auto& field : create_table->fields)
        {
            // Skip non-reference field(s).
            if (field->field_type != ddl::field_type_t::reference)
            {
                continue;
            }

            auto ref = dynamic_cast<ddl::ref_field_def_t*>(field.get());

            // We have not seen the referenced table so far; add the reference
            // definition to the seen reference map and continue.
            if (table_refs.count(ref->table) == 0)
            {
                table_refs.emplace(create_table->name, ref);
                continue;
            }

            // We have seen the referenced table definition and should be able
            // to find the matching reference definition for the current
            // reference definition at this point.
            auto candidate_range = table_refs.equal_range(ref->table);
            auto matching_iter = table_refs.end();
            for (auto it = candidate_range.first; it != candidate_range.second; ++it)
            {
                ddl::ref_field_def_t* candidate_ref = it->second;

                // Skip fields not referencing current table.
                if (candidate_ref->table != create_table->name)
                {
                    continue;
                }

                // The reference definitions are considered a match when any of
                // the following conditions are met.
                // - Both references do not contain a matching field.
                // - One reference definition has a matching field that matches
                //   the other while the other does not have a matching field.
                // - Both references have a matching field that matches the other.
                if ((ref->field.empty() && candidate_ref->field.empty())
                    || (ref->field.empty() && candidate_ref->field == ref->name)
                    || (candidate_ref->field.empty() && ref->field == candidate_ref->name)
                    || (!ref->field.empty() && !candidate_ref->field.empty()
                        && candidate_ref->field == ref->name
                        && ref->field == candidate_ref->name))
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
                    throw ambiguous_reference_definition_internal(ref->table, matching_iter->second->name);
                }
            }

            // We cannot find any matching references for this definition.
            if (matching_iter == table_refs.end())
            {
                throw orphaned_reference_definition_internal(create_table->name, ref->name);
            }

            // Create a standalone `create relationship` definition by combining
            // the two matching reference definitions.
            //
            ddl::ref_field_def_t* matching_ref = matching_iter->second;

            // Many to many is not supported at the moment.
            if (matching_ref->cardinality == relationship_cardinality_t::many
                && ref->cardinality == relationship_cardinality_t::many)
            {
                throw many_to_many_not_supported_internal(ref->table, matching_ref->table);
            }

            check_reference_field_maps(matching_ref->field_map, ref->field_map, ref->table, matching_ref->table);

            // Use the [link1]_[link2] as the relationship name. For 1:N
            // relationships, always place many side first.
            //
            // TODO: Detect name conflict. [GATAPLAT-306]
            //
            std::string rel_name
                = (ref->cardinality == relationship_cardinality_t::many
                       ? matching_ref->name + "_" + ref->name
                       : ref->name + "_" + matching_ref->name);

            relationships.emplace_back(std::make_unique<ddl::create_relationship_t>(rel_name));

            ddl::link_def_t ref_link{
                "", create_table->name, ref->name, "", ref->table, ref->cardinality};
            ddl::link_def_t matching_ref_link{
                "", ref->table, matching_ref->name, "", matching_ref->table, matching_ref->cardinality};
            relationships.back()->relationship = std::make_pair(matching_ref_link, ref_link);

            relationships.back()->has_if_not_exists = false;

            if (matching_ref->field_map)
            {
                relationships.back()->field_map = matching_ref->field_map;
            }
            else if (ref->field_map)
            {
                relationships.back()->field_map = ref->field_map;
            }

            // Remove the matched reference definition.
            table_refs.erase(matching_iter);
        }
    }

    // There are references left unmatched. Report the first seen one.
    if (table_refs.size() > 0)
    {
        throw orphaned_reference_definition_internal(table_refs.begin()->first, table_refs.begin()->second->name);
    }

    // Append the new relationship definition(s) to the statement list.
    statements.insert(
        statements.end(),
        std::make_move_iterator(relationships.begin()),
        std::make_move_iterator(relationships.end()));
}

void execute_create_statement_no_txn(
    ddl_executor_t& executor,
    ddl::create_statement_t* create_stmt)
{
    bool throw_on_exist = true;
    if (create_stmt->has_if_not_exists)
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
            throw_on_exist,
            create_stmt->auto_drop);
    }
    else if (create_stmt->type == ddl::create_type_t::create_database)
    {
        executor.create_database(create_stmt->name, throw_on_exist, create_stmt->auto_drop);
    }
    else if (create_stmt->type == ddl::create_type_t::create_relationship)
    {
        auto create_relationship_stmt = dynamic_cast<ddl::create_relationship_t*>(create_stmt);
        executor.create_relationship(
            create_relationship_stmt->name,
            create_relationship_stmt->relationship.first,
            create_relationship_stmt->relationship.second,
            create_relationship_stmt->field_map,
            throw_on_exist,
            create_stmt->auto_drop);
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
            throw_on_exist,
            create_stmt->auto_drop);
    }
}

// The `create list` parsing results are stored in the same classes as the
// standalone create statements. We will check to make sure no statement can
// contain database names in identifiers such as `[db_name].[table_name]`.
//
void sanity_check_create_list_statements(
    std::vector<std::unique_ptr<ddl::create_statement_t>>& statements)
{
    ASSERT_PRECONDITION(statements.size() > 1, "The list must contain more than one statement.");

    for (auto& stmt : statements)
    {
        if (stmt->type == ddl::create_type_t::create_table)
        {
            auto create_table = dynamic_cast<ddl::create_table_t*>(stmt.get());
            if (!create_table->database.empty())
            {
                throw invalid_create_list_internal(
                    "CREATE TABLE " + create_table->name
                    + " definition cannot specify a database.");
            }
        }
        else if (stmt->type == ddl::create_type_t::create_index)
        {
            auto create_index = dynamic_cast<ddl::create_index_t*>(stmt.get());
            if (!create_index->database.empty())
            {
                throw invalid_create_list_internal(
                    "CREATE INDEX " + create_index->name
                    + " definition cannot specify a database.");
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
                throw invalid_create_list_internal(
                    "CREATE RELATIONSHIP " + create_relationship->name
                    + " definition cannot specify a database in the link(s).");
            }
            if (create_relationship->field_map
                && (!create_relationship->field_map->first.database.empty()
                    || !create_relationship->field_map->second.database.empty()))
            {
                throw invalid_create_list_internal(
                    "CREATE RELATIONSHIP " + create_relationship->name
                    + " definition cannot specify a database in the field(s).");
            }
        }
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

            ASSERT_INVARIANT(
                create_list->statements.size() >= 1,
                "The list must contain at least one statement.");

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

    std::ifstream ddl_fstream(file_path, std::ifstream::in);
    std::stringstream buffer;
    buffer << ddl_fstream.rdbuf() << ";";

    parser.parse_string(buffer.str());
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
