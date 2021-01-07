/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "ddl_executor.hpp"

#include <memory>

#include "gaia/common.hpp"
#include "gaia/exception.hpp"
#include "db_helpers.hpp"
#include "fbs_generator.hpp"
#include "json_generator.hpp"
#include "logger.hpp"
#include "retail_assert.hpp"
#include "system_table_types.hpp"

using namespace std;

using namespace gaia::catalog::ddl;
using namespace gaia::common;
using namespace gaia::direct_access;

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
    constexpr char c_anonymous_reference_field_name[] = "";

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
        //     binary_schema string,
        //     serialization_template string,
        //     references gaia_database,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("type", data_type_t::e_uint32, 1));
        fields.emplace_back(make_unique<data_field_def_t>("is_system", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<data_field_def_t>("binary_schema", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("serialization_template", data_type_t::e_string, 1));
        fields.emplace_back(
            make_unique<ref_field_def_t>(
                c_anonymous_reference_field_name, "catalog", "gaia_database"));
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
        //     references gaia_table,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("type", data_type_t::e_uint8, 1));
        fields.emplace_back(make_unique<data_field_def_t>("repeated_count", data_type_t::e_uint16, 1));
        fields.emplace_back(make_unique<data_field_def_t>("position", data_type_t::e_uint16, 1));
        fields.emplace_back(make_unique<data_field_def_t>("deprecated", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<data_field_def_t>("active", data_type_t::e_bool, 1));
        // The anonymous reference to the gaia_table defines the ownership.
        fields.emplace_back(make_unique<ref_field_def_t>(
            c_anonymous_reference_field_name, "catalog", "gaia_table"));
        create_table_impl(
            "catalog", "gaia_field", fields, true, false,
            static_cast<gaia_type_t>(catalog_table_type_t::gaia_field));
    }
    {
        // create table gaia_relationship (
        //     name string,
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
        fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
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
        //     table_ids string,
        //     source_location string,
        //     serial_stream string,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("active_on_startup", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<data_field_def_t>("table_ids", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("source_location", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<data_field_def_t>("serial_stream", data_type_t::e_string, 1));
        create_table_impl(
            "catalog", "gaia_ruleset", fields, true, false,
            static_cast<gaia_type_t>(catalog_table_type_t::gaia_ruleset));
    }
    {
        // create table gaia_rule (
        //     name string,
        //     references gaia_ruleset,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<data_field_def_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(
            make_unique<ref_field_def_t>(
                c_anonymous_reference_field_name, "catalog", "gaia_ruleset"));
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
        m_table_names[get_full_table_name(table.gaia_database().name(), table.name())] = table.gaia_id();
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

void drop_relationship_no_ri(gaia_relationship_t relationship)
{
    // Unlink parent.
    if (relationship.parent_gaia_table())
    {
        relationship.parent_gaia_table()
            .parent_gaia_relationship_list()
            .erase(relationship);
    }

    // Unlink child.
    if (relationship.child_gaia_table())
    {
        relationship.child_gaia_table()
            .child_gaia_relationship_list()
            .erase(relationship);
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
        if (relationship.child_gaia_table()
            && (relationship.child_gaia_table().gaia_id() != table_id))
        {
            throw referential_integrity_violation::drop_parent_table(
                table_record.name(),
                relationship.child_gaia_table().name());
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
        if (relationship.parent_gaia_table())
        {
            // Mark the relationship as deprecated.
            auto writer = relationship.writer();
            writer.deprecated = true;
            writer.update_row();

            // Unlink the child side of the relationship.
            relationship.child_gaia_table()
                .child_gaia_relationship_list()
                .erase(relationship);
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
        table_record.gaia_field_list().erase(field_id);
        // Remove the field.
        gaia_field_t::get(field_id).delete_row();
    }

    for (gaia_id_t reference_id : list_references(table_id))
    {
        // Unlink the reference and the owner table.
        table_record.gaia_field_list().erase(reference_id);
        auto reference_record = gaia_field_t::get(reference_id);
        // Remove the reference.
        reference_record.delete_row();
    }

    // Unlink the table from its database.
    table_record.gaia_database().gaia_table_list().erase(table_record);
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
        vector<gaia_id_t> table_ids;
        for (auto& table : db_record.gaia_table_list())
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

reference_offset_t ddl_executor_t::find_parent_available_offset(const gaia_table_t::parent_gaia_relationship_list_t& relationships)
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

reference_offset_t ddl_executor_t::find_child_available_offset(const gaia_table_t::child_gaia_relationship_list_t& relationships)
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
        find_child_available_offset(table.child_gaia_relationship_list()),
        find_parent_available_offset(table.parent_gaia_relationship_list()));
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
                gaia_log::catalog().warn("table \"{}\" (id: {}) already exists, skipping", full_table_name, id);
            }
            return id;
        }
    }

    gaia_log::catalog().debug("Creating table '{}'", full_table_name);

    // Check for any duplication in field names.
    // We do this before generating fbs because FlatBuffers schema
    // also does not allow duplicate field names and we may generate
    // invalid fbs without checking duplication first.
    set<string> field_names;
    for (const auto& field : fields)
    {
        string field_name = field->name;

        // Note: anonymous references are on path of deprecation
        if (field_name.empty())
        {
            retail_assert(field->field_type == field_type_t::reference, "Only references can have empty name");
            const ref_field_def_t* ref_field = dynamic_cast<ref_field_def_t*>(field.get());
            field_name = ref_field->parent_table.second;
        }

        if (field_names.find(field_name) != field_names.end())
        {
            throw duplicate_field(field_name);
        }
        field_names.insert(field_name);
    }

    string fbs{generate_fbs(db_name, table_name, fields)};
    string bfbs{generate_bfbs(fbs)};
    string bin{generate_bin(fbs, generate_json(fields))};

    gaia::db::begin_transaction();
    gaia_type_t table_type = fixed_type == c_invalid_gaia_type ? allocate_type() : fixed_type;

    gaia_id_t table_id = gaia_table_t::insert_row(
        table_name.c_str(),
        table_type,
        is_system,
        bfbs.c_str(),
        bin.c_str());

    gaia_log::catalog().debug(" type:'{}', id:'{}'", table_type, table_id);

    // Connect the table to the database.
    gaia_database_t::get(db_id).gaia_table_list().insert(table_id);

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
                throw table_not_exists(ref_field->full_table_name());
            }

            gaia_table_t parent_table = gaia_table_t::get(parent_type_record_id);
            uint8_t parent_available_offset = find_available_offset(parent_table.gaia_id());
            uint8_t child_available_offset;

            gaia_log::catalog().trace(" relationship parent:'{}', child:'{}', name:'{}'", parent_table.name(), table_name, ref_field->name.c_str());

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
                static_cast<uint8_t>(relationship_cardinality_t::many),
                is_parent_required,
                is_deprecated,
                first_child_offset,
                next_child_offset,
                parent_offset);
            auto rel = gaia_relationship_t::get(relationship_id);
            gaia_table_t::get(table_id).child_gaia_relationship_list().insert(relationship_id);
            parent_table.parent_gaia_relationship_list().insert(relationship_id);
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
            gaia_table_t::get(table_id).gaia_field_list().insert(field_id);
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
    if (db.empty())
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
