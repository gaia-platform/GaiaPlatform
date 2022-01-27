/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/catalog/ddl_executor.hpp"

#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "gaia/common.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/hash.hpp"
#include "gaia_internal/common/logger.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/system_table_types.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"

#include "db_helpers.hpp"
#include "fbs_generator.hpp"
#include "json_generator.hpp"

using namespace gaia::catalog::ddl;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::direct_access;

using std::make_unique;
using std::string;

namespace gaia
{
namespace catalog
{

// If `throw_on_exists` is false, we should skip the operation when the object
// already exists, while a true setting of `audo_drop` tells us to drop the
// existing object. The two conditions are mutually exclusive.
static constexpr char c_assert_throw_and_auto_drop[]
    = "Cannot auto drop and skip on exists at the same time.";
static constexpr char c_empty_hash[] = "";

ddl_executor_t::ddl_executor_t()
{
    bootstrap_catalog();
}

ddl_executor_t& ddl_executor_t::get()
{
    static ddl_executor_t s_instance;
    return s_instance;
}

void ddl_executor_t::bootstrap_catalog()
{
    auto_transaction_t txn(false);
    create_database(c_catalog_db_name, false);

    bool is_system = true;
    bool throw_on_exists = false;
    bool auto_drop = false;
    {
        // create table gaia_database (name string);
        //     name string,
        //     hash string,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("hash", data_type_t::e_string, 1));
        create_table_impl(
            c_catalog_db_name, c_gaia_database_table_name, fields, is_system, throw_on_exists, auto_drop,
            static_cast<gaia_type_t::value_type>(catalog_core_table_type_t::gaia_database));
    }
    {
        // create table gaia_table (
        //     name string,
        //     type uint32,
        //     is_system bool,
        //     binary_schema uint8[],
        //     serialization_template uint8[],
        //     hash string,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("type", data_type_t::e_uint32, 1));
        fields.emplace_back(make_unique<data_field_def_t>("is_system", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<data_field_def_t>("binary_schema", data_type_t::e_uint8, 0));
        fields.emplace_back(make_unique<data_field_def_t>("serialization_template", data_type_t::e_uint8, 0));
        fields.emplace_back(make_unique<data_field_def_t>("hash", data_type_t::e_string, 1));
        create_table_impl(
            c_catalog_db_name, c_gaia_table_table_name, fields, is_system, throw_on_exists, auto_drop,
            static_cast<gaia_type_t::value_type>(catalog_core_table_type_t::gaia_table));
        // create relationship gaia_catalog_database_table (
        //     catalog.gaia_database.gaia_tables -> catalog.gaia_table[],
        //     catalog.gaia_table.database -> catalog.gaia_database
        // );
        create_relationship(
            "gaia_catalog_database_table",
            {c_catalog_db_name, c_gaia_database_table_name, "gaia_tables", c_catalog_db_name, c_gaia_table_table_name, relationship_cardinality_t::many},
            {c_catalog_db_name, c_gaia_table_table_name, "database", c_catalog_db_name, c_gaia_database_table_name, relationship_cardinality_t::one},
            std::nullopt,
            false);
    }
    {
        // create table gaia_field (
        //     name string,
        //     type uint8,
        //     repeated_count uint16,
        //     position uint16,
        //     deprecated bool,
        //     active bool,
        //     unique bool,
        //     optional bool,
        //     hash string,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("type", data_type_t::e_uint8, 1));
        fields.emplace_back(make_unique<data_field_def_t>("repeated_count", data_type_t::e_uint16, 1));
        fields.emplace_back(make_unique<data_field_def_t>("position", data_type_t::e_uint16, 1));
        fields.emplace_back(make_unique<data_field_def_t>("deprecated", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<data_field_def_t>("active", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<data_field_def_t>("unique", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<data_field_def_t>("optional", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<data_field_def_t>("hash", data_type_t::e_string, 1));
        create_table_impl(
            c_catalog_db_name, c_gaia_field_table_name, fields, is_system, throw_on_exists, auto_drop,
            static_cast<gaia_type_t::value_type>(catalog_core_table_type_t::gaia_field));
        // create relationship gaia_catalog_table_field (
        //     catalog.gaia_table.gaia_fields -> catalog.gaia_field[],
        //     catalog.gaia_field.table -> catalog.gaia_table
        // );
        create_relationship(
            "gaia_catalog_table_field",
            {c_catalog_db_name, c_gaia_table_table_name, "gaia_fields", c_catalog_db_name, c_gaia_field_table_name, relationship_cardinality_t::many},
            {c_catalog_db_name, c_gaia_field_table_name, "table", c_catalog_db_name, c_gaia_table_table_name, relationship_cardinality_t::one},
            std::nullopt,
            false);
    }
    {
        // create table gaia_relationship (
        //     to_parent_link_name string,
        //     to_child_link_name string,
        //     cardinality uint8,
        //     parent_required bool,
        //     deprecated bool,
        //     first_child_offset uint16,
        //     next_child_offset uint16,
        //     prev_child_offset uint16,
        //     parent_offset uint16,
        //     parent_field_positions uint16[],
        //     child_field_positions uint16[],
        //     hash string,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("to_parent_link_name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("to_child_link_name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("cardinality", data_type_t::e_uint8, 1));
        fields.emplace_back(make_unique<data_field_def_t>("parent_required", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<data_field_def_t>("deprecated", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<data_field_def_t>("first_child_offset", data_type_t::e_uint16, 1));
        fields.emplace_back(make_unique<data_field_def_t>("next_child_offset", data_type_t::e_uint16, 1));
        fields.emplace_back(make_unique<data_field_def_t>("prev_child_offset", data_type_t::e_uint16, 1));
        fields.emplace_back(make_unique<data_field_def_t>("parent_offset", data_type_t::e_uint16, 1));
        fields.emplace_back(make_unique<data_field_def_t>("parent_field_positions", data_type_t::e_uint16, 0));
        fields.emplace_back(make_unique<data_field_def_t>("child_field_positions", data_type_t::e_uint16, 0));
        fields.emplace_back(make_unique<data_field_def_t>("hash", data_type_t::e_string, 1));
        // See gaia::db::relationship_t for more details about relationships.
        create_table_impl(
            c_catalog_db_name, c_gaia_relationship_table_name, fields, is_system, throw_on_exists, auto_drop,
            static_cast<gaia_type_t::value_type>(catalog_core_table_type_t::gaia_relationship));
        // create relationship gaia_catalog_relationship_parent (
        //     catalog.gaia_table.outgoing_relationships -> catalog.gaia_relationship[],
        //     catalog.gaia_relationship.parent -> catalog.gaia_table
        // );
        create_relationship(
            "gaia_catalog_relationship_parent",
            {c_catalog_db_name, c_gaia_table_table_name, "outgoing_relationships", c_catalog_db_name, c_gaia_relationship_table_name, relationship_cardinality_t::many},
            {c_catalog_db_name, c_gaia_relationship_table_name, "parent", c_catalog_db_name, c_gaia_table_table_name, relationship_cardinality_t::one},
            std::nullopt,
            false);
        // create relationship gaia_catalog_relationship_child (
        //     catalog.gaia_table.incoming_relationships -> catalog.gaia_relationship[],
        //     catalog.gaia_relationship.child -> catalog.gaia_table
        // );
        create_relationship(
            "gaia_catalog_relationship_child",
            {c_catalog_db_name, c_gaia_table_table_name, "incoming_relationships", c_catalog_db_name, c_gaia_relationship_table_name, relationship_cardinality_t::many},
            {c_catalog_db_name, c_gaia_relationship_table_name, "child", c_catalog_db_name, c_gaia_table_table_name, relationship_cardinality_t::one},
            std::nullopt,
            false);
    }
    {
        // create table gaia_index (
        //     name string,
        //     unique bool,
        //     type uint8,
        //     fields uint64[],
        //     hash string,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("unique", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<data_field_def_t>("type", data_type_t::e_uint8, 1));
        fields.emplace_back(make_unique<data_field_def_t>("fields", data_type_t::e_uint64, 0));
        fields.emplace_back(make_unique<data_field_def_t>("hash", data_type_t::e_string, 1));
        create_table_impl(
            c_catalog_db_name, c_gaia_index_table_name, fields, is_system, throw_on_exists, auto_drop,
            static_cast<gaia_type_t::value_type>(catalog_core_table_type_t::gaia_index));

        create_relationship(
            "gaia_catalog_table_index",
            {c_catalog_db_name, c_gaia_table_table_name, "gaia_indexes", c_catalog_db_name, c_gaia_index_table_name, relationship_cardinality_t::many},
            {c_catalog_db_name, c_gaia_index_table_name, "table", c_catalog_db_name, c_gaia_table_table_name, relationship_cardinality_t::one},
            std::nullopt,
            false);
    }
    {
        field_def_list_t fields;
        create_table_impl(
            c_catalog_db_name, c_gaia_ref_anchor_table_name, fields, is_system, throw_on_exists, auto_drop,
            static_cast<gaia_type_t::value_type>(catalog_core_table_type_t::gaia_ref_anchor));
    }

    create_database(c_event_log_db_name, false);
    {
        // create table event_log (
        //     event_type: uint32,
        //     type_id: uint64,
        //     record_id: uint64,
        //     column_id: uint16,
        //     timestamp: uint64,
        //     rules_invoked: bool
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("event_type", data_type_t::e_uint32, 1));
        fields.emplace_back(make_unique<data_field_def_t>("type_id", data_type_t::e_uint32, 1));
        fields.emplace_back(make_unique<data_field_def_t>("record_id", data_type_t::e_uint64, 1));
        fields.emplace_back(make_unique<data_field_def_t>("column_id", data_type_t::e_uint16, 1));
        fields.emplace_back(make_unique<data_field_def_t>("timestamp", data_type_t::e_uint64, 1));
        fields.emplace_back(make_unique<data_field_def_t>("rules_invoked", data_type_t::e_bool, 1));
        create_table_impl(
            c_event_log_db_name, c_event_log_table_name, fields, is_system, throw_on_exists, auto_drop,
            static_cast<gaia_type_t::value_type>(system_table_type_t::event_log));
    }

    // Create the special empty database. Tables created without specifying a
    // database name will be created in it.
    m_empty_db_id = create_database(c_empty_db_name, false);
    m_db_context = c_empty_db_name;
    txn.commit();
}

gaia_id_t ddl_executor_t::create_database(const string& name, bool throw_on_exists, bool auto_drop)
{
    ASSERT_PRECONDITION(throw_on_exists || !auto_drop, c_assert_throw_and_auto_drop);

    // TODO: switch to index for fast lookup.
    auto db_iter = gaia_database_t::list().where(gaia_database_expr::name == name).begin();
    if (db_iter != gaia_database_t::list().end())
    {
        if (auto_drop)
        {
            drop_database(name, false);
        }
        else if (throw_on_exists)
        {
            throw db_already_exists_internal(name);
        }
        else
        {
            return db_iter->gaia_id();
        }
    }

    gaia_log::catalog().debug("Creating database '{}'", name);

    gaia_id_t id = gaia_database_t::insert_row(name.c_str(), c_empty_hash);

    gaia_log::catalog().debug("Created database '{}', id:'{}'", name, id);

    switch_db_context(name);

    return id;
}

gaia_id_t ddl_executor_t::create_table(
    const string& db_name,
    const string& name,
    const field_def_list_t& fields,
    bool throw_on_exists,
    bool auto_drop)
{
    return create_table_impl(db_name, name, fields, false, throw_on_exists, auto_drop);
}

gaia_id_t ddl_executor_t::get_table_id(const std::string& db, const std::string& table)
{
    gaia_id_t db_id = find_db_id(db);
    if (db_id == c_invalid_gaia_id)
    {
        throw db_does_not_exist_internal(db);
    }

    // TODO: switch to index for fast lookup.
    auto table_iter = gaia_database_t::get(db_id).gaia_tables().where(gaia_table_expr::name == table).begin();
    if (table_iter->gaia_id() == c_invalid_gaia_id)
    {
        throw table_does_not_exist_internal(get_full_table_name(db, table));
    }
    else
    {
        return table_iter->gaia_id();
    }
}

gaia_id_t ddl_executor_t::create_relationship(
    const std::string& name,
    const ddl::link_def_t& link1,
    const ddl::link_def_t& link2,
    const std::optional<ddl::table_field_map_t>& field_map,
    bool throw_on_exists,
    bool auto_drop)
{
    ASSERT_PRECONDITION(throw_on_exists || !auto_drop, c_assert_throw_and_auto_drop);

    // TODO: switch to index for fast lookup.
    auto rel_iter = gaia_relationship_t::list().where(gaia_relationship_expr::name == name).begin();
    if (rel_iter != gaia_relationship_t::list().end())
    {
        if (auto_drop)
        {
            this->drop_relationship_no_ri(*rel_iter);
        }
        else if (throw_on_exists)
        {
            throw relationship_already_exists_internal(name);
        }
        else
        {
            return rel_iter->gaia_id();
        }
    }

    gaia_id_t link1_src_table_id = get_table_id(in_context(link1.from_database), link1.from_table);
    gaia_id_t link1_dest_table_id = get_table_id(in_context(link1.to_database), link1.to_table);

    gaia_id_t link2_src_table_id = get_table_id(in_context(link2.from_database), link2.from_table);
    gaia_id_t link2_dest_table_id = get_table_id(in_context(link2.to_database), link2.to_table);

    if (link1_src_table_id != link2_dest_table_id)
    {
        throw relationship_tables_do_not_match_internal(name, link1.from_table, link2.to_table);
    }

    if (link1_dest_table_id != link2_src_table_id)
    {
        throw relationship_tables_do_not_match_internal(name, link1.to_table, link2.from_table);
    }

    if (gaia_table_t::get(link1_src_table_id).database() != gaia_table_t::get(link1_dest_table_id).database())
    {
        throw no_cross_db_relationship_internal(name);
    }

    if (link1.cardinality == relationship_cardinality_t::many
        && link2.cardinality == relationship_cardinality_t::many)
    {
        throw many_to_many_not_supported_internal(name);
    }

    relationship_cardinality_t cardinality
        = (link1.cardinality == relationship_cardinality_t::one
           && link2.cardinality == relationship_cardinality_t::one)
        ? relationship_cardinality_t::one
        : relationship_cardinality_t::many;

    gaia_id_t parent_table_id, child_table_id;
    const char *to_parent_link_name, *to_child_link_name;

    // The first link defines the parent and child in a one-to-one relationship.
    // In a one-to-many relationship, the one-side is always parent. If the
    // second link has singular cardinality, the first link will always define
    // the parent and child in the relationship.
    if (link2.cardinality == relationship_cardinality_t::one)
    {
        parent_table_id = link1_src_table_id;
        child_table_id = link1_dest_table_id;
        to_parent_link_name = link2.name.c_str();
        to_child_link_name = link1.name.c_str();
    }
    else
    {
        parent_table_id = link2_src_table_id;
        child_table_id = link2_dest_table_id;
        to_parent_link_name = link1.name.c_str();
        to_child_link_name = link2.name.c_str();
    }

    uint8_t parent_available_offset = find_available_offset(parent_table_id);
    uint8_t child_available_offset;
    if (parent_table_id == child_table_id)
    {
        // This is a self-relationship, both parent and child pointers are in the same table.
        child_available_offset = parent_available_offset + 1;
        validate_new_reference_offset(child_available_offset);
    }
    else
    {
        child_available_offset = find_available_offset(child_table_id);
    }

    reference_offset_t first_child_offset = parent_available_offset;

    reference_offset_t parent_offset = child_available_offset;
    reference_offset_t next_child_offset = parent_offset + 1;
    validate_new_reference_offset(next_child_offset);
    reference_offset_t prev_child_offset = next_child_offset + 1;
    validate_new_reference_offset(prev_child_offset);

    bool is_parent_required = false;
    bool is_deprecated = false;

    field_position_list_t parent_field_positions, child_field_positions;

    if (field_map)
    {
        if (cardinality == relationship_cardinality_t::one)
        {
            throw invalid_relationship_field_internal(
                "Defining a 1:1 relationship using value linked references (between table '"
                + link1.from_table + "' and table '" + link1.to_table + "') is not supported.");
        }

        if (field_map->first.fields.size() != 1 || field_map->second.fields.size() != 1)
        {
            throw invalid_relationship_field_internal("Defining relationships using composite keys are not supported currently.");
        }
        gaia_id_t first_table_id = get_table_id(in_context(field_map->first.database), field_map->first.table);
        gaia_id_t second_table_id = get_table_id(in_context(field_map->second.database), field_map->second.table);

        std::vector<gaia_id_t> parent_field_ids, child_field_ids;
        if (first_table_id == parent_table_id && second_table_id == child_table_id)
        {
            parent_field_ids = find_table_field_ids(first_table_id, field_map->first.fields);
            child_field_ids = find_table_field_ids(second_table_id, field_map->second.fields);
        }
        else if (first_table_id == child_table_id && second_table_id == parent_table_id)
        {
            parent_field_ids = find_table_field_ids(second_table_id, field_map->second.fields);
            child_field_ids = find_table_field_ids(first_table_id, field_map->first.fields);
        }
        else
        {
            throw invalid_relationship_field_internal("The field's table(s) do not match the tables of the relationship");
        }

        // Parent side fields must be unique.
        for (gaia_id_t field_id : parent_field_ids)
        {
            auto field = gaia_field_t::get(field_id);
            if (!field.unique())
            {
                throw invalid_relationship_field_internal(
                    string("The field '") + field.name() + "' defined in table '" + field.table().name()
                    + "' is used to define a relationship and must be declared UNIQUE.");
            }
        }

        // In 1:1 relationships, child side fields also need to be unique.
        if (cardinality == relationship_cardinality_t::one)
        {
            for (gaia_id_t field_id : child_field_ids)
            {
                auto field = gaia_field_t::get(field_id);
                if (!field.unique())
                {
                    throw invalid_relationship_field_internal(
                        string("The field '") + field.name() + "' defined in the table '" + field.table().name()
                        + "' is used to define a 1:1 relationship and must be declared UNIQUE.");
                }
            }
        }
        else
        {
            // For 1:N relationships, we need to create a value index on the
            // fields of the child side if they are not already indexed.
            bool index_exists = false;
            for (const gaia_index_t& index : gaia_table_t::get(child_table_id).gaia_indexes())
            {
                if (index.fields().size() != child_field_ids.size())
                {
                    continue;
                }
                bool fields_match = true;
                for (size_t i = 0; i < child_field_ids.size(); i++)
                {
                    if (child_field_ids[i] != index.fields()[i])
                    {
                        fields_match = false;
                        break;
                    }
                }
                if (fields_match)
                {
                    index_exists = true;
                    break;
                }
            }
            if (!index_exists)
            {
                bool is_unique_index = false;
                index_type_t index_type = index_type_t::range;
                create_index(name, is_unique_index, index_type, child_table_id, child_field_ids);
            }
        }

        for (gaia_id_t field_id : parent_field_ids)
        {
            parent_field_positions.push_back(gaia_field_t::get(field_id).position());
        }
        for (gaia_id_t field_id : child_field_ids)
        {
            child_field_positions.push_back(gaia_field_t::get(field_id).position());
        }
    }

    // These casts works because a field_position_t is a thin wrapper over uint16_t,
    // but its success is not guaranteed by the language and is undefined behavior (UB).
    // TODO: Replace reinterpret_cast with bit_cast when it becomes available.
    auto* parent_field_position_values
        = reinterpret_cast<std::vector<field_position_t::value_type>*>(&parent_field_positions);
    auto* child_field_position_values
        = reinterpret_cast<std::vector<field_position_t::value_type>*>(&child_field_positions);

    gaia_id_t relationship_id = gaia_relationship_t::insert_row(
        name.c_str(),
        to_parent_link_name,
        to_child_link_name,
        static_cast<uint8_t>(cardinality),
        is_parent_required,
        is_deprecated,
        first_child_offset,
        next_child_offset,
        prev_child_offset,
        parent_offset,
        *parent_field_position_values,
        *child_field_position_values,
        c_empty_hash);

    gaia_table_t::get(parent_table_id).outgoing_relationships().insert(relationship_id);
    gaia_table_t::get(child_table_id).incoming_relationships().insert(relationship_id);

    return relationship_id;
}

void ddl_executor_t::drop_relationship_no_ri(gaia_relationship_t& relationship)
{
    // Clear all links in table records on both sides of the relationship. We do
    // this by disconnecting all child records from their parent.
    for (auto record : gaia_ptr_t::find_all_range(relationship.child().type()))
    {
        gaia_ptr::remove_from_reference_container(record, relationship.parent_offset());
    }

    // Unlink parent.
    if (relationship.parent())
    {
        relationship.parent()
            .outgoing_relationships()
            .remove(relationship);
    }

    // Unlink child.
    if (relationship.child())
    {
        relationship.child()
            .incoming_relationships()
            .remove(relationship);
    }

    relationship.delete_row();
}

void ddl_executor_t::drop_relationship(const std::string& name, bool throw_unless_exists)
{
    // TODO: switch to index for fast lookup.
    auto rel_iter = gaia_relationship_t::list().where(gaia_relationship_expr::name == name).begin();
    if (rel_iter == gaia_relationship_t::list().end())
    {
        if (throw_unless_exists)
        {
            throw relationship_does_not_exist_internal(name);
        }
        return;
    }

    drop_relationship_no_ri(*rel_iter);
}

void ddl_executor_t::drop_relationships_no_ri(gaia_id_t table_id)
{
    auto table_record = gaia_table_t::get(table_id);

    for (auto& relationship_id : list_parent_relationships(table_id))
    {
        auto relationship = gaia_relationship_t::get(relationship_id);
        drop_relationship_no_ri(relationship);
    }

    for (gaia_id_t relationship_id : list_child_relationships(table_id))
    {
        auto relationship = gaia_relationship_t::get(relationship_id);
        drop_relationship_no_ri(relationship);
    }
}

void ddl_executor_t::drop_table(gaia_id_t table_id, bool enforce_referential_integrity)
{
    auto table_record = gaia_table_t::get(table_id);

    if (enforce_referential_integrity)
    {
        // Under the referential integrity requirements, we do not allow
        // dropping a table that is referenced by any other table.
        for (gaia_id_t relationship_id : list_parent_relationships(table_id))
        {
            auto relationship = gaia_relationship_t::get(relationship_id);
            if (relationship.child().gaia_id() != table_id)
            {
                throw referential_integrity_violation_internal::drop_referenced_table(
                    table_record.name(),
                    relationship.child().name());
            }
        }

        for (gaia_id_t relationship_id : list_child_relationships(table_id))
        {
            auto relationship = gaia_relationship_t::get(relationship_id);
            if (relationship.parent().gaia_id() != table_id)
            {
                throw referential_integrity_violation_internal::drop_referenced_table(
                    table_record.name(),
                    relationship.parent().name());
            }
        }
    }

    // At this point, we have either passed the referential integrity check or
    // are ignoring referential integrity. Either way, it is safe to delete all
    // relationships associated with the table.
    drop_relationships_no_ri(table_id);

    // Delete all data records of the table.
    for (gaia_ptr_t data_record : gaia_ptr_t::find_all_range(table_record.type()))
    {
        data_record.reset();
    }

    for (gaia_id_t field_id : list_fields(table_id))
    {
        // Unlink the field and the table.
        table_record.gaia_fields().remove(field_id);
        // Remove the field.
        gaia_field_t::get(field_id).delete_row();
    }

    std::vector<gaia_id_t> index_ids;
    for (const auto& index : gaia_table_t::get(table_id).gaia_indexes())
    {
        index_ids.push_back(index.gaia_id());
    }
    for (gaia_id_t id : index_ids)
    {
        // Unlink the index.
        table_record.gaia_indexes().remove(id);
        // Remove the index.
        gaia_index_t::get(id).delete_row();
    }

    // Unlink the table from its database.
    table_record.database().gaia_tables().remove(table_record);
    // Remove the table.
    table_record.delete_row();
}

void ddl_executor_t::drop_database(const string& name, bool throw_unless_exists)
{
    gaia_id_t db_id = find_db_id(name);
    if (db_id == c_invalid_gaia_id)
    {
        if (throw_unless_exists)
        {
            throw db_does_not_exist_internal(name);
        }
        return;
    }

    auto db_record = gaia_database_t::get(db_id);
    std::vector<gaia_id_t> table_ids;
    for (auto& table : db_record.gaia_tables())
    {
        table_ids.push_back(table.gaia_id());
    }
    for (gaia_id_t table_id : table_ids)
    {
        drop_table(table_id, false);
    }
    db_record.delete_row();
}

void ddl_executor_t::drop_table(const string& db_name, const string& name, bool throw_unless_exists)
{
    gaia_id_t db_id = find_db_id(in_context(db_name));
    if (db_id == c_invalid_gaia_id)
    {
        if (throw_unless_exists)
        {
            throw db_does_not_exist_internal(db_name);
        }
        return;
    }

    // TODO: switch to index for fast lookup.
    auto table_iter = gaia_database_t::get(db_id).gaia_tables().where(gaia_table_expr::name == name).begin();
    if (table_iter->gaia_id() == c_invalid_gaia_id)
    {
        if (throw_unless_exists)
        {
            throw table_does_not_exist_internal(name);
        }
        return;
    }

    drop_table(table_iter->gaia_id(), true);
}

void ddl_executor_t::validate_new_reference_offset(reference_offset_t reference_offset)
{
    if (reference_offset == c_invalid_reference_offset)
    {
        throw max_reference_count_reached_internal();
    }
}

reference_offset_t ddl_executor_t::find_parent_available_offset(const gaia_table_t::outgoing_relationships_list_t& relationships)
{
    if (relationships.begin() == relationships.end())
    {
        return 0;
    }

    reference_offset_t max_offset = 0;
    for (const auto& relationship : relationships)
    {
        max_offset = std::max(max_offset.value(), relationship.first_child_offset());

        ASSERT_INVARIANT(max_offset != c_invalid_reference_offset, "Invalid reference offset detected!");
    }

    reference_offset_t next_available_offset = max_offset + 1;

    validate_new_reference_offset(next_available_offset);

    return next_available_offset;
}

reference_offset_t ddl_executor_t::find_child_available_offset(const gaia_table_t::incoming_relationships_list_t& relationships)
{
    if (relationships.begin() == relationships.end())
    {
        return 0;
    }

    reference_offset_t max_offset = 0;
    for (const auto& relationship : relationships)
    {
        max_offset = std::max(
            {max_offset.value(),
             relationship.prev_child_offset(),
             relationship.parent_offset()});

        ASSERT_INVARIANT(max_offset != c_invalid_reference_offset, "Invalid reference offset detected!");
    }

    reference_offset_t next_available_offset = max_offset + 1;

    validate_new_reference_offset(next_available_offset);

    return next_available_offset;
}

reference_offset_t ddl_executor_t::find_available_offset(gaia::common::gaia_id_t table_id)
{
    gaia_table_t table = gaia_table_t::get(table_id);
    return std::max(
        find_child_available_offset(table.incoming_relationships()),
        find_parent_available_offset(table.outgoing_relationships()));
}

uint32_t generate_table_type(const string& db_name, const string& table_name)
{
    // An identifier length is limited to a flex token which is limited to the
    // size of the bison/flex input buffer (YY_BUF_SIZE). We currently use
    // default setting which is 16k. The assertions below make sure the token
    // length does not exceed the `len` parameter of the hash function.
    ASSERT_PRECONDITION(db_name.length() <= std::numeric_limits<int>::max(), "The DB name is too long.");
    ASSERT_PRECONDITION(table_name.length() <= std::numeric_limits<int>::max(), "The table name is too long.");

    return hash::murmur3_32(table_name.data(), static_cast<int>(table_name.length()))
        ^ (hash::murmur3_32(db_name.data(), static_cast<int>(db_name.length())) << 1);
}

gaia_id_t ddl_executor_t::create_table_impl(
    const string& db_name,
    const string& table_name,
    const field_def_list_t& fields,
    bool is_system,
    bool throw_on_exists,
    bool auto_drop,
    gaia_type_t fixed_type)
{
    ASSERT_PRECONDITION(throw_on_exists || !auto_drop, c_assert_throw_and_auto_drop);

    gaia_id_t db_id = find_db_id(in_context(db_name));
    if (db_id == c_invalid_gaia_id)
    {
        throw db_does_not_exist_internal(db_name);
    }

    // TODO: switch to index for fast lookup.
    auto table_iter = gaia_database_t::get(db_id).gaia_tables().where(gaia_table_expr::name == table_name).begin();
    if (table_iter->gaia_id() != c_invalid_gaia_id)
    {
        if (auto_drop)
        {
            drop_table(table_iter->gaia_id(), false);
        }
        else if (throw_on_exists)
        {
            throw table_already_exists_internal(table_name);
        }
        else
        {
            if (!is_system)
            {
                // Log a warning message for skipping non-system table creation.
                //
                // The warning should not apply to system table creation
                // because: 1) users cannot create system tables by design; 2)
                // current catalog implementation will try to re-create all
                // system tables on every startup and expect the creation to be
                // skipped normally if the tables already exist.
                gaia_log::catalog().warn(
                    "Table '{}' (id: {}) already exists, skipping.", table_name, table_iter->gaia_id());
            }
            return table_iter->gaia_id();
        }
    }

    gaia_log::catalog().debug("Creating table '{}'", table_name);

    // Check for any duplication in field names.
    // We do this before generating fbs because FlatBuffers schema
    // also does not allow duplicate field names and we may generate
    // invalid fbs without checking duplication first.
    std::set<string> field_names;
    for (const auto& field : fields)
    {
        string field_name = field->name;

        if (field_names.find(field_name) != field_names.end())
        {
            throw duplicate_field_internal(field_name);
        }
        field_names.insert(field_name);
    }

    gaia_type_t table_type
        = (fixed_type == c_invalid_gaia_type)
        ? gaia_type_t(generate_table_type(in_context(db_name), table_name))
        : fixed_type;

    gaia_table_writer table_w;
    table_w.name = table_name.c_str();
    table_w.type = table_type;
    table_w.is_system = is_system;
    gaia_id_t table_id = table_w.insert_row();
    gaia_table_t gaia_table = gaia_table_t::get(table_id);

    // Connect the table to the database.
    gaia_database_t::get(db_id).gaia_tables().insert(table_id);

    gaia_log::catalog().debug("Created table '{}', type:'{}', id:'{}'", table_name, table_type, table_id);

    uint16_t data_field_position = 0;
    for (const auto& field : fields)
    {
        if (field->field_type != field_type_t::data)
        {
            continue;
        }

        const data_field_def_t* data_field = dynamic_cast<data_field_def_t*>(field.get());
        gaia_id_t field_id = gaia_field_t::insert_row(
            field->name.c_str(),
            static_cast<uint8_t>(data_field->data_type),
            data_field->length,
            data_field_position,
            false,
            data_field->active,
            data_field->unique,
            data_field->optional,
            c_empty_hash);

        // Connect the field to the table it belongs to.
        gaia_table.gaia_fields().insert(field_id);
        // Create an unique range index for the unique field.
        if (data_field->unique)
        {
            gaia_id_t index_id = gaia_index_t::insert_row(
                string(table_name + '_' + field->name).c_str(),
                true,
                static_cast<uint8_t>(index_type_t::range),
                {field_id},
                c_empty_hash);
            gaia_table.gaia_indexes().insert(index_id);
        }
        data_field_position++;
    }

    // After creating the table, generates the bfbs and the serialization template.
    // using the newly generated table_id.
    string fbs{generate_fbs(table_id)};
    const std::vector<uint8_t> bfbs = generate_bfbs(fbs);

    bool ignore_optional = true;
    string fbs_without_optional{generate_fbs(table_id, ignore_optional)};
    const std::vector<uint8_t> serialization_template = generate_bin(fbs_without_optional, generate_json(table_id));

    table_w = gaia_table.writer();
    table_w.serialization_template = serialization_template;
    table_w.binary_schema = bfbs;
    table_w.update_row();

    return table_id;
}

gaia_id_t ddl_executor_t::find_db_id(const string& db_name) const
{
    if (db_name.empty())
    {
        return m_empty_db_id;
    }
    // TODO: switch to index for fast lookup.
    return gaia_database_t::list().where(gaia_database_expr::name == db_name).begin()->gaia_id();
}

string ddl_executor_t::get_full_table_name(const string& db, const string& table)
{
    if (db.empty() || db == c_empty_db_name)
    {
        return table;
    }
    else
    {
        return db + c_db_table_name_connector + table;
    }
}

void ddl_executor_t::switch_db_context(const string& db_name)
{
    if (!db_name.empty() && find_db_id(db_name) == c_invalid_gaia_id)
    {
        throw db_does_not_exist_internal(db_name);
    }

    m_db_context = db_name;
}

std::vector<gaia_id_t> ddl_executor_t::find_table_field_ids(
    gaia_id_t table_id, const std::vector<std::string>& field_names)
{
    std::vector<gaia_id_t> field_ids;

    std::unordered_map<std::string, gaia_id_t> table_fields;
    for (const auto& field : gaia_table_t::get(table_id).gaia_fields())
    {
        table_fields[field.name()] = field.gaia_id();
    }

    std::unordered_set<std::string> field_name_set;
    for (const auto& name : field_names)
    {
        if (table_fields.find(name) == table_fields.end())
        {
            throw field_does_not_exist_internal(name);
        }
        if (field_name_set.find(name) != field_name_set.end())
        {
            throw duplicate_field_internal(name);
        }
        else
        {
            field_name_set.insert(name);
        }
        field_ids.push_back(table_fields.at(name));
    }
    return field_ids;
}

gaia_id_t ddl_executor_t::create_index(
    const std::string& index_name,
    bool unique,
    index_type_t type,
    const std::string& db_name,
    const std::string& table_name,
    const std::vector<std::string>& field_names,
    bool throw_on_exists,
    bool auto_drop)
{
    ASSERT_PRECONDITION(throw_on_exists || !auto_drop, c_assert_throw_and_auto_drop);

    gaia_id_t table_id = get_table_id(in_context(db_name), table_name);

    for (const auto& index : gaia_table_t::get(table_id).gaia_indexes())
    {
        if (index.name() == index_name)
        {
            if (auto_drop)
            {
                drop_index(index_name, false);
            }
            else if (throw_on_exists)
            {
                throw index_already_exists_internal(index_name);
            }
            else
            {
                return index.gaia_id();
            }
        }
    }

    std::vector<gaia_id_t> index_field_ids = find_table_field_ids(table_id, field_names);
    return create_index(index_name, unique, type, table_id, index_field_ids);
}

gaia_id_t ddl_executor_t::create_index(
    const std::string& name,
    bool unique,
    index_type_t type,
    gaia_id_t table_id,
    const std::vector<gaia_id_t>& field_ids)
{
    // This cast works because a gaia_id_t is a thin wrapper over uint64_t,
    // but its success is not guaranteed by the language and is undefined behavior (UB).
    // TODO: Replace reinterpret_cast with bit_cast when it becomes available.
    auto* field_id_values
        = reinterpret_cast<const std::vector<gaia_id_t::value_type>*>(&field_ids);

    gaia_id_t index_id = gaia_index_t::insert_row(
        name.c_str(),
        unique,
        static_cast<uint8_t>(type),
        *field_id_values,
        c_empty_hash);

    gaia_table_t::get(table_id).gaia_indexes().insert(index_id);

    // Creating a unique index on a single field automatically makes the field
    // unique. Do nothing for multiple-field index creation because we do not
    // support unique constraints for composite keys at the moment.
    if (unique && field_ids.size() == 1)
    {
        auto field_writer = gaia_field_t::get(field_ids.front()).writer();
        field_writer.unique = true;
        field_writer.update_row();
    }

    return index_id;
}

void ddl_executor_t::drop_index(const std::string& name, bool throw_unless_exists)
{
    // TODO: switch to index for fast lookup.
    auto index_iter = gaia_index_t::list().where(gaia_index_expr::name == name).begin();
    if (index_iter == gaia_index_t::list().end())
    {
        if (throw_unless_exists)
        {
            throw index_does_not_exist_internal(name);
        }
        return;
    }
    index_iter->table().gaia_indexes().remove(index_iter->gaia_id());
    index_iter->delete_row();
}

} // namespace catalog
} // namespace gaia
