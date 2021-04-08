/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/catalog/ddl_executor.hpp"

#include <memory>

#include "gaia/common.hpp"
#include "gaia/direct_access/auto_transaction.hpp"
#include "gaia/exception.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/common/logger_internal.hpp"
#include "gaia_internal/common/pluralize.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/system_table_types.hpp"

#include "db_helpers.hpp"
#include "fbs_generator.hpp"
#include "json_generator.hpp"

using namespace gaia::catalog::ddl;
using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::direct_access;

using std::make_unique;
using std::shared_lock;
using std::string;
using std::unique_lock;

namespace gaia
{
namespace catalog
{

ddl_executor_t::ddl_executor_t()
{
    reset();
}

ddl_executor_t& ddl_executor_t::get()
{
    static ddl_executor_t s_instance;
    return s_instance;
}

void ddl_executor_t::bootstrap_catalog()
{

    create_database("catalog", false);
    {
        // create table gaia_database (name string);
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
        create_table_impl(
            "catalog", "gaia_database", fields, true, false,
            static_cast<gaia_type_t>(catalog_table_type_t::gaia_database));
    }
    {
        // create table gaia_table (
        //     name string,
        //     type uint32,
        //     is_system bool,
        //     binary_schema uint8[],
        //     serialization_template uint8[],
        //     database references gaia_database,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("type", data_type_t::e_uint32, 1));
        fields.emplace_back(make_unique<data_field_def_t>("is_system", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<data_field_def_t>("binary_schema", data_type_t::e_uint8, 0));
        fields.emplace_back(make_unique<data_field_def_t>("serialization_template", data_type_t::e_uint8, 0));
        fields.emplace_back(
            make_unique<ref_field_def_t>(
                "database", "catalog", "gaia_database"));
        create_table_impl(
            "catalog", "gaia_table", fields, true, false,
            static_cast<gaia_type_t>(catalog_table_type_t::gaia_table));
    }
    {
        // create table gaia_field (
        //     name string,
        //     type uint8,
        //     repeated_count uint16,
        //     position uint16,
        //     deprecated bool,
        //     active bool,
        //     table references gaia_table,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("type", data_type_t::e_uint8, 1));
        fields.emplace_back(make_unique<data_field_def_t>("repeated_count", data_type_t::e_uint16, 1));
        fields.emplace_back(make_unique<data_field_def_t>("position", data_type_t::e_uint16, 1));
        fields.emplace_back(make_unique<data_field_def_t>("deprecated", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<data_field_def_t>("active", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<ref_field_def_t>(
            "table", "catalog", "gaia_table"));
        create_table_impl(
            "catalog", "gaia_field", fields, true, false,
            static_cast<gaia_type_t>(catalog_table_type_t::gaia_field));
    }
    {
        // create table gaia_relationship (
        //     to_parent_link_name string,
        //     to_child_link_name string,
        //     parent references gaia_table,
        //     child references gaia_table,
        //     cardinality uint8,
        //     parent_required bool,
        //     deprecated bool,
        //     first_child_offset uint16,
        //     next_child_offset uint16,
        //     parent_offset uint16,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("to_parent_link_name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("to_child_link_name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<ref_field_def_t>("parent", "catalog", "gaia_table"));
        fields.emplace_back(make_unique<ref_field_def_t>("child", "catalog", "gaia_table"));
        fields.emplace_back(make_unique<data_field_def_t>("cardinality", data_type_t::e_uint8, 1));
        fields.emplace_back(make_unique<data_field_def_t>("parent_required", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<data_field_def_t>("deprecated", data_type_t::e_bool, 1));
        // See gaia::db::relationship_t for more details about relationships.
        // (parent)-[first_child_offset]->(child)
        fields.emplace_back(make_unique<data_field_def_t>("first_child_offset", data_type_t::e_uint16, 1));
        // (child)-[next_child_offset]->(child)
        fields.emplace_back(make_unique<data_field_def_t>("next_child_offset", data_type_t::e_uint16, 1));
        // (child)-[parent_offset]->(parent)
        fields.emplace_back(make_unique<data_field_def_t>("parent_offset", data_type_t::e_uint16, 1));
        create_table_impl(
            "catalog", "gaia_relationship",
            fields, true, false, static_cast<gaia_id_t>(catalog_table_type_t::gaia_relationship));
    }
    {
        // create table gaia_ruleset (
        //     name string,
        //     active_on_startup bool,
        //     table_ids uint64[],
        //     source_location string,
        //     serial_stream string,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("active_on_startup", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<data_field_def_t>("table_ids", data_type_t::e_uint64, 0));
        fields.emplace_back(make_unique<data_field_def_t>("source_location", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("serial_stream", data_type_t::e_string, 1));
        create_table_impl(
            "catalog", "gaia_ruleset", fields, true, false,
            static_cast<gaia_type_t>(catalog_table_type_t::gaia_ruleset));
    }
    {
        // create table gaia_rule (
        //     name string,
        //     ruleset references gaia_ruleset,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(
            make_unique<ref_field_def_t>(
                "ruleset", "catalog", "gaia_ruleset"));
        create_table_impl(
            "catalog", "gaia_rule", fields, true, false,
            static_cast<gaia_type_t>(catalog_table_type_t::gaia_rule));
    }
}

void ddl_executor_t::create_system_tables()
{
    create_database("event_log", false);
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
            "event_log", "event_log", fields, true, false,
            static_cast<gaia_type_t>(system_table_type_t::event_log));
    }
}

void ddl_executor_t::reset()
{
    reload_cache();
    bootstrap_catalog();
    create_system_tables();
    // Create the special global database.
    // Tables created without specifying a database name will belong to the global database.
    m_empty_db_id = create_database(c_empty_db_name, false);
}

void ddl_executor_t::clear_cache()
{
    m_table_names.clear();
    m_db_names.clear();
}

void ddl_executor_t::reload_cache()
{
    unique_lock lock(m_lock);

    clear_cache();

    gaia::db::begin_transaction();
    for (const auto& db : gaia_database_t::list())
    {
        m_db_names[db.name()] = db.gaia_id();
    }

    for (auto& table : gaia_table_t::list())
    {
        m_table_names[get_full_table_name(table.database().name(), table.name())] = table.gaia_id();
    }
    gaia::db::commit_transaction();
}

gaia_id_t ddl_executor_t::create_database(const string& name, bool throw_on_exist)
{
    unique_lock lock(m_lock);
    if (m_db_names.find(name) != m_db_names.end())
    {
        if (throw_on_exist)
        {
            throw db_already_exists(name);
        }
        else
        {
            return m_db_names.at(name);
        }
    }
    gaia::db::begin_transaction();
    gaia_id_t id = gaia_database_t::insert_row(name.c_str());
    gaia::db::commit_transaction();
    m_db_names[name] = id;
    return id;
}

gaia_id_t ddl_executor_t::create_table(
    const string& db_name,
    const string& name,
    const field_def_list_t& fields,
    bool throw_on_exists)
{
    return create_table_impl(db_name, name, fields, false, throw_on_exists);
}

gaia_id_t ddl_executor_t::get_table_id(const std::string& db, const std::string& table)
{
    shared_lock lock(m_lock);
    string full_table_name = get_full_table_name(db, table);
    if (m_table_names.find(full_table_name) == m_table_names.end())
    {
        throw table_not_exists(full_table_name);
    }
    return m_table_names[full_table_name];
}

gaia_id_t ddl_executor_t::create_relationship(
    const std::string& name,
    const ddl::link_def_t& link1,
    const ddl::link_def_t& link2)
{
    shared_lock lock(m_lock);

    gaia_id_t link1_src_table_id = get_table_id(link1.from_database, link1.from_table);
    gaia_id_t link1_dest_table_id = get_table_id(link1.to_database, link1.to_table);

    gaia_id_t link2_src_table_id = get_table_id(link2.from_database, link2.from_table);
    gaia_id_t link2_dest_table_id = get_table_id(link2.to_database, link2.to_table);

    if (link1_src_table_id != link2_dest_table_id)
    {
        throw tables_not_match(name, link1.from_table, link2.to_table);
    }

    if (link1_dest_table_id != link2_src_table_id)
    {
        throw tables_not_match(name, link1.to_table, link2.from_table);
    }

    // The first link defines the parent and child in the relationship.
    gaia_id_t parent_table_id = link1_src_table_id;
    gaia_id_t child_table_id = link1_dest_table_id;

    retail_assert(
        link2.cardinality != relationship_cardinality_t::many,
        "Many to many relatinships are not supported right now!");

    auto_transaction_t txn(false);

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

    const char* to_parent_link_name = link2.name.c_str();
    const char* to_child_link_name = link1.name.c_str();

    reference_offset_t first_child_offset = parent_available_offset;

    reference_offset_t parent_offset = child_available_offset;
    reference_offset_t next_child_offset = child_available_offset + 1;
    validate_new_reference_offset(next_child_offset);

    bool is_parent_required = false;
    bool is_deprecated = false;

    gaia_id_t relationship_id = c_invalid_gaia_id;
    relationship_id = gaia_relationship_t::insert_row(
        to_parent_link_name,
        to_child_link_name,
        static_cast<uint8_t>(link1.cardinality),
        is_parent_required,
        is_deprecated,
        first_child_offset,
        next_child_offset,
        parent_offset);

    gaia_table_t::get(parent_table_id).gaia_relationships_child().insert(relationship_id);
    gaia_table_t::get(child_table_id).gaia_relationships_parent().insert(relationship_id);

    txn.commit();

    return relationship_id;
}

void drop_relationship_no_ri(gaia_relationship_t relationship)
{
    // Unlink parent.
    if (relationship.parent())
    {
        relationship.parent()
            .gaia_relationships_parent()
            .remove(relationship);
    }

    // Unlink child.
    if (relationship.child())
    {
        relationship.child()
            .gaia_relationships_child()
            .remove(relationship);
    }

    relationship.delete_row();
}

void ddl_executor_t::drop_relationships_no_txn(gaia_id_t table_id, bool enforce_referential_integrity)
{
    auto table_record = gaia_table_t::get(table_id);

    for (auto& relationship_id : list_parent_relationships(table_id))
    {
        auto relationship = gaia_relationship_t::get(relationship_id);

        if (!enforce_referential_integrity)
        {
            drop_relationship_no_ri(relationship);
            continue;
        }

        // Cannot drop a parent relationship the link with the children still exists,
        // and it is not a self-reference. In a self-reference relationship the same table
        // is both parent and child, thus you can delete it.
        if (relationship.child()
            && (relationship.child().gaia_id() != table_id))
        {
            throw referential_integrity_violation::drop_parent_table(
                table_record.name(),
                relationship.child().name());
        }

        // There are 2 options here:
        // 1. The child side of this relationship has already been deleted.
        //    Now we are deleting the parent, hence the relationship object
        //    can be deleted too.
        // 2. This is a self-reference hence both links have to be removed
        //    before deleting it.
        drop_relationship_no_ri(relationship);
    }

    // Unlink the child side of the relationship.
    for (gaia_id_t relationship_id : list_child_relationships(table_id))
    {
        auto relationship = gaia_relationship_t::get(relationship_id);

        if (!enforce_referential_integrity)
        {
            drop_relationship_no_ri(relationship);
            continue;
        }

        // If the parent side of the relationship still exists we can't
        // delete the relationship object. The parent table need to keep
        // track of the total amount of relationships to correctly
        // maintain the references array.
        if (relationship.parent())
        {
            // Mark the relationship as deprecated.
            auto writer = relationship.writer();
            writer.deprecated = true;
            writer.update_row();

            // Unlink the child side of the relationship.
            relationship.child()
                .gaia_relationships_child()
                .remove(relationship);
        }
        else
        {
            // Parent is already unlinked (maybe the field has been deleted).
            relationship.delete_row();
        }
    }
}

void ddl_executor_t::drop_table_no_txn(gaia_id_t table_id, bool enforce_referential_integrity)
{
    auto table_record = gaia_table_t::get(table_id);

    drop_relationships_no_txn(table_id, enforce_referential_integrity);

    for (gaia_id_t field_id : list_fields(table_id))
    {
        // Unlink the field and the table.
        table_record.gaia_fields().remove(field_id);
        // Remove the field.
        gaia_field_t::get(field_id).delete_row();
    }

    for (gaia_id_t reference_id : list_references(table_id))
    {
        // Unlink the reference and the owner table.
        table_record.gaia_fields().remove(reference_id);
        auto reference_record = gaia_field_t::get(reference_id);
        // Remove the reference.
        reference_record.delete_row();
    }

    // Unlink the table from its database.
    table_record.database().gaia_tables().remove(table_record);
    // Remove the table.
    table_record.delete_row();
}

void ddl_executor_t::drop_database(const string& name)
{
    unique_lock lock(m_lock);
    gaia_id_t db_id = find_db_id_no_lock(name);
    if (db_id == c_invalid_gaia_id)
    {
        throw db_not_exists(name);
    }
    {
        auto_transaction_t txn;
        auto db_record = gaia_database_t::get(db_id);
        std::vector<gaia_id_t> table_ids;
        for (auto& table : db_record.gaia_tables())
        {
            table_ids.push_back(table.gaia_id());
        }
        for (gaia_id_t table_id : table_ids)
        {
            drop_table_no_txn(table_id, false);
        }
        db_record.delete_row();
        txn.commit();
    }
    m_db_names.erase(name);
}

void ddl_executor_t::drop_table(const string& db_name, const string& name)
{

    unique_lock lock(m_lock);

    if (!db_name.empty() && m_db_names.find(db_name) == m_db_names.end())
    {
        throw db_not_exists(db_name);
    }

    string full_table_name = get_full_table_name(db_name, name);
    gaia_id_t db_id = find_db_id_no_lock(db_name);
    retail_assert(db_id != c_invalid_gaia_id, "Invalid database id!");

    if (m_table_names.find(full_table_name) == m_table_names.end())
    {
        throw table_not_exists(name);
    }
    gaia_id_t table_id = m_table_names[full_table_name];

    {
        auto_transaction_t txn;
        drop_table_no_txn(table_id, true);
        txn.commit();
    }

    // Invalidate catalog caches.
    m_table_names.erase(full_table_name);
}

void ddl_executor_t::validate_new_reference_offset(reference_offset_t reference_offset)
{
    if (reference_offset == c_invalid_reference_offset)
    {
        throw max_reference_count_reached();
    }
}

reference_offset_t ddl_executor_t::find_parent_available_offset(const gaia_table_t::gaia_relationships_parent_list_t& relationships)
{
    if (relationships.begin() == relationships.end())
    {
        return 0;
    }

    reference_offset_t max_offset = 0;
    for (const auto& relationship : relationships)
    {
        max_offset = std::max(max_offset, relationship.first_child_offset());

        retail_assert(max_offset != c_invalid_reference_offset, "Invalid reference offset detected!");
    }

    reference_offset_t next_available_offset = max_offset + 1;

    validate_new_reference_offset(next_available_offset);

    return next_available_offset;
}

reference_offset_t ddl_executor_t::find_child_available_offset(const gaia_table_t::gaia_relationships_child_list_t& relationships)
{
    if (relationships.begin() == relationships.end())
    {
        return 0;
    }

    reference_offset_t max_offset = 0;
    for (const auto& relationship : relationships)
    {
        max_offset = std::max({max_offset, relationship.next_child_offset(), relationship.parent_offset()});

        retail_assert(max_offset != c_invalid_reference_offset, "Invalid reference offset detected!");
    }

    reference_offset_t next_available_offset = max_offset + 1;

    validate_new_reference_offset(next_available_offset);

    return next_available_offset;
}

reference_offset_t ddl_executor_t::find_available_offset(gaia::common::gaia_id_t table_id)
{
    gaia_table_t table = gaia_table_t::get(table_id);
    return std::max(
        find_child_available_offset(table.gaia_relationships_child()),
        find_parent_available_offset(table.gaia_relationships_parent()));
}

void ddl_executor_t::disambiguate_child_link_names(gaia_table_t& parent_table)
{
    std::map<gaia_id_t, std::list<gaia_relationship_t>> child_relationships;

    for (auto& r : parent_table.gaia_relationships_parent())
    {
        child_relationships[r.child().gaia_id()].push_back(r);
    }

    for (auto& cildren_rel : child_relationships)
    {
        // If there is more than one relationship from the parent_table
        // and a child table, we need to update the to_child_link_name.
        if (cildren_rel.second.size() > 1)
        {
            for (auto& r : cildren_rel.second)
            {
                std::string new_to_child_name = to_plural(r.child().name()) + "_" + r.to_parent_link_name();

                if (r.to_child_link_name() == new_to_child_name)
                {
                    continue;
                }

                gaia_log::catalog().trace(
                    " Changing '{}.{}' to '{}.{}'",
                    parent_table.name(), r.to_child_link_name(), parent_table.name(), new_to_child_name);

                auto writer = r.writer();
                writer.to_child_link_name = new_to_child_name;
                writer.update_row();
            }
        }
    }
}

gaia_id_t ddl_executor_t::create_table_impl(
    const string& db_name,
    const string& table_name,
    const field_def_list_t& fields,
    bool is_system,
    bool throw_on_exist,
    gaia_type_t fixed_type)
{
    unique_lock lock(m_lock);

    if (!db_name.empty() && m_db_names.find(db_name) == m_db_names.end())
    {
        throw db_not_exists(db_name);
    }

    string full_table_name = get_full_table_name(db_name, table_name);
    gaia_id_t db_id = find_db_id_no_lock(db_name);
    retail_assert(db_id != c_invalid_gaia_id, "Invalid database id!");

    if (m_table_names.find(full_table_name) != m_table_names.end())
    {
        if (throw_on_exist)
        {
            throw table_already_exists(full_table_name);
        }
        else
        {
            gaia_id_t id = m_table_names.at(full_table_name);
            if (!is_system)
            {
                // Log a warning message for skipping non-system table creation.
                //
                // The warnning should not apply to system table creation
                // because current implementation will try to re-create all
                // system tables on every startup and expect the creation to be
                // skipped normally if the tables already exist.
                gaia_log::catalog().warn("Table '{}' (id: {}) already exists, skipping.", full_table_name, id);
            }
            return id;
        }
    }

    gaia_log::catalog().debug("Creating table '{}'", full_table_name);

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
            throw duplicate_field(field_name);
        }
        field_names.insert(field_name);
    }

    string fbs{generate_fbs(db_name, table_name, fields)};
    const std::vector<uint8_t> bfbs = generate_bfbs(fbs);
    const std::vector<uint8_t> bin = generate_bin(fbs, generate_json(fields));

    gaia::db::begin_transaction();
    gaia_type_t table_type = fixed_type == c_invalid_gaia_type ? allocate_type() : fixed_type;

    gaia_id_t table_id = gaia_table_t::insert_row(
        table_name.c_str(),
        table_type,
        is_system,
        bfbs,
        bin);

    gaia_log::catalog().debug(" type:'{}', id:'{}'", table_type, table_id);

    // Connect the table to the database.
    gaia_database_t::get(db_id).gaia_tables().insert(table_id);

    uint16_t data_field_position = 0;
    for (const auto& field : fields)
    {
        if (field->field_type == field_type_t::reference)
        {
            // The (table) record id that defines the parent table type.
            gaia_id_t parent_type_record_id = c_invalid_gaia_id;

            const ref_field_def_t* ref_field = dynamic_cast<ref_field_def_t*>(field.get());

            if (ref_field->full_table_name() == full_table_name
                || (ref_field->db_name().empty() && ref_field->table_name() == table_name))
            {
                // We allow a table definition to reference itself (self-referencing).
                parent_type_record_id = table_id;
            }
            else if (m_table_names.find(ref_field->full_table_name()) != m_table_names.end())
            {
                // A table definition can reference any of existing tables.
                parent_type_record_id = m_table_names[ref_field->full_table_name()];
            }
            else if (!db_name.empty() && m_table_names.count(db_name + c_db_table_name_connector + ref_field->table_name()))
            {
                // A table definition can reference existing tables in its own database
                // without specifying the database name.
                parent_type_record_id = m_table_names[db_name + c_db_table_name_connector + ref_field->table_name()];
            }
            else
            {
                // We cannot find the referenced table.
                // Forward declaration is not supported.
                gaia::db::rollback_transaction();
                throw table_not_exists(ref_field->full_table_name());
            }

            gaia_table_t parent_table = gaia_table_t::get(parent_type_record_id);
            uint8_t parent_available_offset = find_available_offset(parent_table.gaia_id());
            uint8_t child_available_offset;
            std::string to_child_link_name = to_plural(table_name);

            gaia_log::catalog().trace(" relationship parent:'{}', child:'{}', to_parent_name:'{}', to_child_name:'{}'", parent_table.name(), table_name, ref_field->name.c_str(), to_child_link_name);

            if (parent_type_record_id == table_id)
            {
                // This is a self-relationship, both parent and child pointers are in the same table.
                child_available_offset = parent_available_offset + 1;

                validate_new_reference_offset(child_available_offset);
            }
            else
            {
                child_available_offset = find_available_offset(table_id);
            }

            reference_offset_t first_child_offset = parent_available_offset;

            reference_offset_t parent_offset = child_available_offset;
            reference_offset_t next_child_offset = child_available_offset + 1;
            validate_new_reference_offset(next_child_offset);

            bool is_parent_required = false;
            bool is_deprecated = false;

            gaia_id_t relationship_id = gaia_relationship_t::insert_row(
                ref_field->name.c_str(),
                to_child_link_name.c_str(),
                static_cast<uint8_t>(relationship_cardinality_t::many),
                is_parent_required,
                is_deprecated,
                first_child_offset,
                next_child_offset,
                parent_offset);
            auto rel = gaia_relationship_t::get(relationship_id);
            gaia_log::catalog().trace("  first_offset:{} next_offset:{} parent_offset:{}", rel.first_child_offset(), rel.next_child_offset(), rel.parent_offset());
            gaia_table_t::get(table_id).gaia_relationships_child().insert(relationship_id);
            parent_table.gaia_relationships_parent().insert(relationship_id);
            disambiguate_child_link_names(parent_table);
        }
        else
        {
            gaia_log::catalog().trace(" field:'{}', type:'{}'", field->name.c_str(), field->field_type);

            const data_field_def_t* data_field = dynamic_cast<data_field_def_t*>(field.get());
            gaia_id_t field_id = gaia_field_t::insert_row(
                field->name.c_str(),
                static_cast<uint8_t>(data_field->data_type),
                data_field->length,
                data_field_position,
                false,
                data_field->active);
            // Connect the field to the table it belongs to.
            gaia_table_t::get(table_id).gaia_fields().insert(field_id);
            data_field_position++;
        }
    }
    gaia::db::commit_transaction();

    m_table_names[full_table_name] = table_id;
    return table_id;
}

gaia_id_t ddl_executor_t::find_db_id(const string& dbname) const
{
    shared_lock lock(m_lock);
    return find_db_id_no_lock(dbname);
}

inline gaia_id_t ddl_executor_t::find_db_id_no_lock(const string& dbname) const
{
    if (dbname.empty())
    {
        return m_empty_db_id;
    }
    else if (m_db_names.count(dbname))
    {
        return m_db_names.at(dbname);
    }
    else
    {
        return c_invalid_gaia_id;
    }
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

} // namespace catalog
} // namespace gaia
