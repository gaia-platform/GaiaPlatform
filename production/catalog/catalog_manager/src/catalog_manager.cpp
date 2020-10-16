/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "catalog_manager.hpp"

#include <memory>

#include "fbs_generator.hpp"
#include "gaia_catalog.h"
#include "gaia_exception.hpp"
#include "logger.hpp"
#include "retail_assert.hpp"
#include "system_table_types.hpp"

using namespace gaia::catalog::ddl;

/**
 * Catalog public APIs
 **/
namespace gaia
{
namespace catalog
{

static constexpr char c_empty_c_str[] = "";

void initialize_catalog()
{
    catalog_manager_t::get();
}

gaia_id_t create_database(const string& name, bool throw_on_exists)
{
    return catalog_manager_t::get().create_database(name, throw_on_exists);
}

gaia_id_t create_table(const string& name,
                       const field_def_list_t& fields)
{
    return catalog_manager_t::get().create_table(c_empty_c_str, name, fields);
}

gaia_id_t create_table(
    const string& dbname,
    const string& name,
    const field_def_list_t& fields,
    bool throw_on_exists)
{
    return catalog_manager_t::get().create_table(dbname, name, fields, throw_on_exists);
}

void drop_database(const string& name)
{
    return catalog_manager_t::get().drop_database(name);
}

void drop_table(const string& name)
{
    return catalog_manager_t::get().drop_table(c_empty_c_str, name);
}

void drop_table(const string& dbname, const string& name)
{
    return catalog_manager_t::get().drop_table(dbname, name);
}

vector<gaia_id_t> list_fields(gaia_id_t table_id)
{
    return catalog_manager_t::get().list_fields(table_id);
}

vector<gaia_id_t> list_references(gaia_id_t table_id)
{
    return catalog_manager_t::get().list_references(table_id);
}

gaia_id_t find_db_id(const string& dbname)
{
    return catalog_manager_t::get().find_db_id(dbname);
}

/**
 * Class methods
 **/

catalog_manager_t::catalog_manager_t()
{
    init();
}

catalog_manager_t& catalog_manager_t::get()
{
    static catalog_manager_t s_instance;
    return s_instance;
}

void catalog_manager_t::bootstrap_catalog()
{
    create_database("catalog", false);
    {
        // create table gaia_database (name string);
        field_def_list_t fields;
        fields.emplace_back(make_unique<field_definition_t>("name", data_type_t::e_string, 1));
        create_table_impl("catalog", "gaia_database", fields, false, false, static_cast<gaia_id_t>(catalog_table_type_t::gaia_database));
    }
    {
        // create table gaia_table (
        //     name string,
        //     is_log bool,
        //     binary_schema string,
        //     references gaia_database,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<field_definition_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<field_definition_t>("is_log", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<field_definition_t>("binary_schema", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<field_definition_t>(c_empty_c_str, data_type_t::e_references, 1, "catalog.gaia_database"));
        create_table_impl("catalog", "gaia_table", fields, false, false, static_cast<gaia_id_t>(catalog_table_type_t::gaia_table));
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
        //     ref references gaia_table,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<field_definition_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<field_definition_t>("type", data_type_t::e_uint8, 1));
        fields.emplace_back(make_unique<field_definition_t>("repeated_count", data_type_t::e_uint16, 1));
        fields.emplace_back(make_unique<field_definition_t>("position", data_type_t::e_uint16, 1));
        fields.emplace_back(make_unique<field_definition_t>("deprecated", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<field_definition_t>("active", data_type_t::e_bool, 1));
        // The anonymous reference to the gaia_table defines the ownership.
        fields.emplace_back(make_unique<field_definition_t>(c_empty_c_str, data_type_t::e_references, 1, "catalog.gaia_table"));
        // TODO this will be deprecated in favor to the gaia_relationship table.
        // The "ref" named reference to the gaia_table defines the referential relationship.
        fields.emplace_back(make_unique<field_definition_t>("ref", data_type_t::e_references, 1, "catalog.gaia_table"));
        create_table_impl("catalog", "gaia_field", fields, false, false, static_cast<gaia_id_t>(catalog_table_type_t::gaia_field));
    }
    {
        // create table gaia_relationship (
        //     parent references gaia_table,
        //     child references gaia_field,
        //     cardinality unit8,
        //     parent_required bool,
        //     deprecated bool,
        //     first_child_offset unit8,
        //     next_child_offset unit8,
        //     parent_offset unit8,
        // );

        field_def_list_t fields;
        // TODO since we have pointers on both sides of the relationship and we generate the relatioship field in the parent in EDC, I believe we should
        //  explicitly track the relationship on the parent side too. This will also come in handy to customize the generated EDC:
        //  - table_t::gaia_field_list() which is arguably ugly could be table_t::fields()
        //  - database_t::gaia_table_list() which is arguably ugly could be database_t::fields()
        //  I will remove this comment and create a JIRA before pushing the code.
        fields.emplace_back(make_unique<field_definition_t>("parent", data_type_t::e_references, 1, "catalog.gaia_table"));
        fields.emplace_back(make_unique<field_definition_t>("child", data_type_t::e_references, 1, "catalog.gaia_field"));
        fields.emplace_back(make_unique<field_definition_t>("cardinality", data_type_t::e_uint8, 1));
        fields.emplace_back(make_unique<field_definition_t>("parent_required", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<field_definition_t>("deprecated", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<field_definition_t>("first_child_offset", data_type_t::e_uint8, 1));
        fields.emplace_back(make_unique<field_definition_t>("next_child_offset", data_type_t::e_uint8, 1));
        fields.emplace_back(make_unique<field_definition_t>("parent_offset", data_type_t::e_uint8, 1));
        create_table_impl("catalog", "gaia_relationship", fields, false, false, static_cast<gaia_id_t>(catalog_table_type_t::gaia_relationship));
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
        fields.emplace_back(make_unique<field_definition_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<field_definition_t>("active_on_startup", data_type_t::e_bool, 1));
        fields.emplace_back(make_unique<field_definition_t>("table_ids", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<field_definition_t>("source_location", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<field_definition_t>("serial_stream", data_type_t::e_string, 1));
        create_table_impl("catalog", "gaia_ruleset", fields, false, false, static_cast<gaia_id_t>(catalog_table_type_t::gaia_ruleset));
    }
    {
        // create table gaia_rule (
        //     name string,
        //     ruleset_id bool,
        //     references gaia_ruleset,
        // );
        field_def_list_t fields;
        fields.emplace_back(make_unique<field_definition_t>("name", data_type_t::e_string, 1));
        fields.emplace_back(make_unique<field_definition_t>(c_empty_c_str, data_type_t::e_references, 1, "catalog.gaia_ruleset"));
        create_table_impl("catalog", "gaia_rule", fields, false, false, static_cast<gaia_id_t>(catalog_table_type_t::gaia_rule));
    }
}

void catalog_manager_t::create_system_tables()
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
        fields.emplace_back(make_unique<field_definition_t>("event_type", data_type_t::e_uint32, 1));
        fields.emplace_back(make_unique<field_definition_t>("type_id", data_type_t::e_uint64, 1));
        fields.emplace_back(make_unique<field_definition_t>("record_id", data_type_t::e_uint64, 1));
        fields.emplace_back(make_unique<field_definition_t>("column_id", data_type_t::e_uint16, 1));
        fields.emplace_back(make_unique<field_definition_t>("timestamp", data_type_t::e_uint64, 1));
        fields.emplace_back(make_unique<field_definition_t>("rules_invoked", data_type_t::e_bool, 1));
        create_table_impl("event_log", "event_log", fields, true, false, static_cast<gaia_id_t>(system_table_type_t::event_log));
    }
}

void catalog_manager_t::init()
{
    reload_cache();
    reload_metadata();
    bootstrap_catalog();
    create_system_tables();
    m_empty_db_id = create_database(c_empty_db_name, false);
}

void catalog_manager_t::clear_cache()
{
    m_table_names.clear();
}

void catalog_manager_t::reload_cache()
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
        string full_table_name = string(table.gaia_database().name()) + "." + string(table.name());
        m_table_names[full_table_name] = table.gaia_id();
    }
    gaia::db::commit_transaction();
}

void register_table_in_metadata(gaia_id_t table_id)
{
    // TODO I think that instead of keep polluting the Catalog code with responsibilities
    //  we should aim towards a pub-sub architecture. The catalog publish events when
    //  something change, the subscribers (caches, metadata, etc..) get notified and act
    //  accordingly. This way the catalog does only one job. Chuan mentioned that something
    //  similar is available in the SE (triggers?)
    gaia_table_t child_table = gaia_table_t::get(table_id);

    auto& metadata = type_registry_t::instance().get_or_create(table_id);

    for (auto& field : child_table.gaia_field_list())
    {
        if (field.type() == static_cast<uint8_t>(data_type_t::e_references))
        {
            for (auto& relationship : field.child_gaia_relationship_list())
            {
                gaia_table_t parent_table = relationship.parent_gaia_table();

                auto rel = make_shared<relationship_t>(relationship_t{
                    .parent_type = parent_table.gaia_id(),
                    .child_type = child_table.gaia_id(),
                    .first_child_offset = relationship.first_child_offset(),
                    .next_child_offset = relationship.next_child_offset(),
                    .parent_offset = relationship.parent_offset(),
                    .cardinality = cardinality_t::many,
                    .parent_required = false});

                auto& parent_meta = type_registry_t::instance().get_or_create(parent_table.gaia_id());
                parent_meta.add_parent_relationship(relationship.first_child_offset(), rel);

                metadata.add_child_relationship(relationship.parent_offset(), rel);
            }
        }
    }
}

void catalog_manager_t::reload_metadata()
{
    gaia::db::begin_transaction();
    type_registry_t::instance().clear();

    for (const auto& table : gaia_table_t::list())
    {
        register_table_in_metadata(table.gaia_id());
    }
    gaia::db::commit_transaction();
}

gaia_id_t catalog_manager_t::create_database(
    const string& name,
    bool throw_on_exist)
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

gaia_id_t catalog_manager_t::create_table(
    const string& db_name,
    const string& name,
    const field_def_list_t& fields,
    bool throw_on_exists)
{
    return create_table_impl(db_name, name, fields, false, throw_on_exists);
}

void drop_relationship_no_ri(gaia_relationship_t relationship)
{
    // unlink parent
    relationship.parent_gaia_table()
        .parent_gaia_relationship_list()
        .erase(relationship);

    // unlink child
    relationship.child_gaia_field()
        .child_gaia_relationship_list()
        .erase(relationship);

    relationship.delete_row();
}

void catalog_manager_t::drop_relationships_no_txn(gaia_id_t table_id, bool referential_integrity)
{
    auto table_record = gaia_table_t::get(table_id);

    for (auto& relationship_id : list_parent_relationships(table_id))
    {
        auto relationship = gaia_relationship_t::get(relationship_id);

        if (!referential_integrity)
        {
            drop_relationship_no_ri(relationship);
            continue;
        }

        // The link with the children still exists, fail.
        if (relationship.child_gaia_field())
        {
            throw referential_integrity_violation::drop_parent_table(
                table_record.name(),
                relationship.child_gaia_field().gaia_table().name());
        }

        // The child side of this relationship has already been deleted.
        // Now we are deleting the parent, hence the relationship object
        // can be deleted too
        relationship.parent_gaia_table()
            .parent_gaia_relationship_list()
            .erase(relationship);
        relationship.delete_row();
    }

    // unlink the child side of the relationship
    for (gaia_id_t relationship_id : list_child_relationships(table_id))
    {
        auto relationship = gaia_relationship_t::get(relationship_id);

        if (!referential_integrity)
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
            auto rel_writer = relationship.writer();
            rel_writer.deprecated = true;
            rel_writer.update_row();

            // unlink the child side of the relationship.
            relationship.child_gaia_field()
                .child_gaia_relationship_list()
                .erase(relationship);
        }
        else
        {
            // Parent is already unlinked (maybe the field has been deleted)
            relationship.delete_row();
        }
    }
}

void catalog_manager_t::drop_table_no_txn(gaia_id_t table_id, bool referential_integrity)
{
    auto table_record = gaia_table_t::get(table_id);

    drop_relationships_no_txn(table_id, referential_integrity);

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
        // Unlink the reference and the referred table.
        reference_record.ref_gaia_table().ref_gaia_field_list().erase(reference_id);
        // Remove the reference.
        reference_record.delete_row();
    }

    // Unlink the table from its database.
    table_record.gaia_database().gaia_table_list().erase(table_record);
    // Remove the table.
    table_record.delete_row();
}

void catalog_manager_t::drop_database(const string& name)
{
    unique_lock lock(m_lock);
    gaia_id_t db_id = find_db_id_no_lock(name);
    if (db_id == INVALID_GAIA_ID)
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

void catalog_manager_t::drop_table(
    const string& db_name,
    const string& name)
{

    unique_lock lock(m_lock);

    if (!db_name.empty() && m_db_names.find(db_name) == m_db_names.end())
    {
        throw db_not_exists(db_name);
    }

    string full_table_name = (db_name.empty() ? c_empty_c_str : db_name + ".") + name;
    gaia_id_t db_id = find_db_id_no_lock(db_name);
    retail_assert(db_id != INVALID_GAIA_ID);

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

    {
        // Invalidate catalog caches.
        m_table_names.erase(full_table_name);
        type_registry_t::instance().remove(table_id);
    }
}

static gaia_ptr insert_gaia_table_row(
    gaia_id_t table_id,
    const char* name,
    bool is_log,
    const char* binary_schema)
{

    // NOTE: The number of table references must be updated manually for bootstrapping,
    //       when the references of the gaia_table change. The constant from existing catalog
    //       extended data classes "gaia_catalog.h" may be incorrect when that happens.
    static constexpr size_t c_gaia_table_num_refs = c_num_gaia_table_ptrs;

    flatbuffers::FlatBufferBuilder fbb(c_flatbuffer_builder_size);
    fbb.Finish(Creategaia_tableDirect(fbb, name, is_log, binary_schema));

    return gaia_ptr::create(
        table_id,
        static_cast<gaia_type_t>(catalog_table_type_t::gaia_table),
        c_gaia_table_num_refs,
        fbb.GetSize(),
        fbb.GetBufferPointer());
}

/**
 * Find the next available offset in a container of relationship_t.
 * relationship_t contain the offset for both parent and child
 * side of the relationship, for this reason you need to specify
 * what side of the relationship you are considering.
 */
template <typename T_relationship_container>
uint8_t find_available_offset(T_relationship_container& relationships, bool parent)
{
    uint8_t max_offset{0};

    if (relationships.begin() != relationships.end())
    {

        for (const auto& relationship : relationships)
        {
            if (parent)
            {
                max_offset = std::max(max_offset, relationship.first_child_offset());
            }
            else
            {
                max_offset = std::max(
                    max_offset,
                    std::max(relationship.next_child_offset(), relationship.parent_offset()));
            }
        }

        max_offset++;
    }

    return max_offset;
}

uint8_t find_available_offset(gaia_table_t& table)
{
    uint8_t max_offset{0};

    // scan child relationships
    for (auto field : table.gaia_field_list())
    {
        if (field.type() == static_cast<uint8_t>(data_type_t::e_references))
        {
            max_offset = std::max(
                max_offset,
                find_available_offset(field.child_gaia_relationship_list(), false));
        }
    }

    // scan parent relationships
    return std::max(
        max_offset,
        find_available_offset(table.parent_gaia_relationship_list(), true));
}

gaia_id_t catalog_manager_t::create_table_impl(
    const string& dbname,
    const string& table_name,
    const field_def_list_t& fields,
    bool is_log,
    bool throw_on_exist,
    gaia_id_t id)
{

    unique_lock lock(m_lock);

    if (!dbname.empty() && m_db_names.find(dbname) == m_db_names.end())
    {
        throw db_not_exists(dbname);
    }

    string full_table_name = (dbname.empty() ? c_empty_c_str : dbname + ".") + table_name;
    gaia_id_t db_id = find_db_id_no_lock(dbname);
    retail_assert(db_id != INVALID_GAIA_ID);

    if (m_table_names.find(full_table_name) != m_table_names.end())
    {
        gaia_log::catalog().debug("Table '{}' already exists", full_table_name);
        if (throw_on_exist)
        {
            throw table_already_exists(full_table_name);
        }
        else
        {
            return m_table_names.at(full_table_name);
        }
    }

    // Check for any duplication in field names.
    // We do this before generating fbs because FlatBuffers schema
    // also does not allow duplicate field names and we may generate
    // invalid fbs without checking duplication first.
    set<string> field_names;
    for (const auto& field : fields)
    {
        if (field_names.find(field->name) != field_names.end())
        {
            throw duplicate_field(field->name);
        }
        field_names.insert(field->name);
    }

    string bfbs{generate_bfbs(generate_fbs(dbname, table_name, fields))};

    gaia::db::begin_transaction();
    gaia_id_t table_id;
    if (id == INVALID_GAIA_ID)
    {
        gaia_log::catalog().debug("Creating table: {}", full_table_name);
        table_id = gaia_table_t::insert_row(
            table_name.c_str(),
            is_log,
            bfbs.c_str());
    }

    else
    {
        gaia_log::catalog().info("Creating system table {}", full_table_name);
        table_id = id;
        insert_gaia_table_row(
            table_id,
            table_name.c_str(),
            is_log,
            bfbs.c_str());
    }

    // Connect the table to the database
    gaia_database_t::get(db_id).gaia_table_list().insert(table_id);

    uint16_t field_position = 0, reference_position = 0;
    for (const auto& field : fields)
    {
        gaia_id_t parent_type_id{0};
        uint16_t position;
        if (field->type == data_type_t::e_references)
        {
            if (field->table_type_name == full_table_name || field->table_type_name == table_name)
            {
                // We allow a table definition to reference itself (self-referencing).
                parent_type_id = table_id;
            }
            else if (m_table_names.find(field->table_type_name) != m_table_names.end())
            {
                // A table definition can reference any of existing tables.
                parent_type_id = m_table_names[field->table_type_name];
            }
            else if (!dbname.empty() && m_table_names.count(dbname + "." + field->table_type_name))
            {
                // A table definition can reference existing tables in its own database without specifying the database name.
                parent_type_id = m_table_names[dbname + "." + field->table_type_name];
            }
            else
            {
                // We cannot find the referenced table.
                // Forward declaration is not supported right now.
                throw table_not_exists(field->table_type_name);
            }
            // The field ID/position values must be a contiguous range from 0 onward.
            position = reference_position++;
        }
        else
        {
            position = field_position++;
        }
        gaia_id_t field_id = gaia_field_t::insert_row(
            field->name.c_str(),
            static_cast<uint8_t>(field->type),
            field->length,
            position,
            false,
            field->active);
        // Connect the field to the table it belongs to.
        gaia_table_t table = gaia_table_t::get(table_id);
        table.gaia_field_list().insert(field_id);

        if (field->type == data_type_t::e_references)
        {
            gaia_table_t parent_table = gaia_table_t::get(parent_type_id);
            gaia_field_t child_field = gaia_field_t::get(field_id);

            // Connect the referred table to the reference field.
            parent_table.ref_gaia_field_list().insert(field_id);

            uint8_t parent_available_offset = find_available_offset(parent_table);
            uint8_t child_max_offset = find_available_offset(table);

            gaia_id_t relationship_id = gaia_relationship_t::insert_row(
                static_cast<uint8_t>(2),                            // cardinality
                false,                                              // parent_required
                false,                                              // deprecated
                static_cast<uint8_t>(parent_available_offset),      // first_child_offset
                static_cast<uint8_t>(child_max_offset),             // next_child_offset
                static_cast<uint8_t>(child_max_offset + uint8_t{1}) // parent_offset
            );
            auto rel = gaia_relationship_t::get(relationship_id);

            child_field.child_gaia_relationship_list().insert(relationship_id);
            parent_table.parent_gaia_relationship_list().insert(relationship_id);
        }
    }

    register_table_in_metadata(table_id);
    gaia::db::commit_transaction();

    m_table_names[full_table_name] = table_id;

    return table_id;
}

gaia_id_t catalog_manager_t::find_db_id(const string& dbname) const
{
    shared_lock lock(m_lock);
    return find_db_id_no_lock(dbname);
}

inline gaia_id_t catalog_manager_t::find_db_id_no_lock(const string& dbname) const
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
        return INVALID_GAIA_ID;
    }
}

vector<gaia_id_t> catalog_manager_t::list_fields(gaia_id_t table_id) const
{
    vector<gaia_id_t> fields;
    // Direct access reference list API guarantees LIFO. As long as we only
    // allow appending new fields to table definitions, reversing the field list
    // order should result in fields being listed in the ascending order of
    // their positions.
    for (const auto& field : gaia_table_t::get(table_id).gaia_field_list())
    {
        if (field.type() != static_cast<uint8_t>(data_type_t::e_references))
        {
            fields.insert(fields.begin(), field.gaia_id());
        }
    }
    return fields;
}

vector<gaia_id_t> catalog_manager_t::list_references(gaia_id_t table_id) const
{
    vector<gaia_id_t> references;
    // Direct access reference list API guarantees LIFO. As long as we only
    // allow appending new references to table definitions, reversing the
    // reference field list order should result in references being listed in
    // the ascending order of their positions.
    for (const auto& field : gaia_table_t::get(table_id).gaia_field_list())
    {
        if (field.type() == static_cast<uint8_t>(data_type_t::e_references))
        {
            references.insert(references.begin(), field.gaia_id());
        }
    }
    return references;
}

vector<gaia_id_t> catalog_manager_t::list_child_relationships(gaia_id_t table_id) const
{
    vector<gaia_id_t> relationships;

    for (gaia_id_t ref_id : list_references(table_id))
    {
        for (const gaia_relationship_t& child_relationship :
             gaia_field_t::get(ref_id).child_gaia_relationship_list())
        {
            relationships.push_back(child_relationship.gaia_id());
        }
    }

    return relationships;
}

vector<gaia::common::gaia_id_t> catalog_manager_t::list_parent_relationships(gaia::common::gaia_id_t table_id) const
{
    vector<gaia_id_t> relationships;

    for (const gaia_relationship_t& parent_relationship :
         gaia_table_t::get(table_id).parent_gaia_relationship_list())
    {
        relationships.push_back(parent_relationship.gaia_id());
    }

    return relationships;
}

} // namespace catalog
} // namespace gaia
