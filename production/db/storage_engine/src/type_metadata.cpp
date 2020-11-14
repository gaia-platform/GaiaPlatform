/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "type_metadata.hpp"

#include <shared_mutex>

#include "gaia_catalog.h"
#include "gaia_common.hpp"
#include "logger.hpp"
#include "system_table_types.hpp"
#include "type_id_record_id_cache.hpp"

using namespace gaia::catalog;

namespace gaia
{
namespace db
{

// Child relationship contains 2 pointer for every relationship.
constexpr std::size_t c_child_relation_num_ptrs = 2;

/*
 * type_metadata_t
 */

optional<relationship_t> type_metadata_t::find_parent_relationship(reference_offset_t first_child_offset) const
{
    shared_lock lock(m_metadata_lock);

    const auto& parent_relationship = m_parent_relationships.find(first_child_offset);

    if (parent_relationship == m_parent_relationships.end())
    {
        return std::nullopt;
    }

    return *parent_relationship->second.get();
}

optional<relationship_t> type_metadata_t::find_child_relationship(reference_offset_t parent_offset) const
{
    shared_lock lock(m_metadata_lock);

    auto child_relationship = m_child_relationships.find(parent_offset);

    if (child_relationship == m_child_relationships.end())
    {
        return std::nullopt;
    }

    return *child_relationship->second.get();
}

void type_metadata_t::add_parent_relationship(const shared_ptr<relationship_t>& relationship)
{
    unique_lock lock(m_metadata_lock);

    m_parent_relationships.insert({relationship->first_child_offset, relationship});
}

void type_metadata_t::add_child_relationship(const shared_ptr<relationship_t>& relationship)
{
    unique_lock lock(m_metadata_lock);

    m_child_relationships.insert({relationship->parent_offset, relationship});
}

gaia_type_t type_metadata_t::get_type() const
{
    return m_type;
}

size_t type_metadata_t::num_references() const
{
    shared_lock lock(m_metadata_lock);

    return m_parent_relationships.size() + (c_child_relation_num_ptrs * m_child_relationships.size());
}

bool type_metadata_t::is_initialized()
{
    return m_initialized;
}

void type_metadata_t::mark_as_initialized()
{
    m_initialized.store(true);
}

/*
 * type_registry_t
 */

void type_registry_t::init()
{
    // EDC uses type_registry_t for RI, type_registry_t uses EDC to pull information about types,
    // the information about types are inserted using EDC.
    // To break this circular dependency the relationships about core catalog types are hardcoded here.
    // This is a temporary solution until we decouple the creation of core catalog types from EDC.
    auto database = static_cast<gaia_type_t>(catalog_table_type_t::gaia_database);
    auto table = static_cast<gaia_type_t>(catalog_table_type_t::gaia_table);
    auto field = static_cast<gaia_type_t>(catalog_table_type_t::gaia_field);
    auto relationship = static_cast<gaia_type_t>(catalog_table_type_t::gaia_relationship);
    auto rule = static_cast<gaia_type_t>(catalog_table_type_t::gaia_rule);
    auto ruleset = static_cast<gaia_type_t>(catalog_table_type_t::gaia_ruleset);

    auto db_table_relationship = make_shared<relationship_t>(relationship_t{
        .parent_type = database,
        .child_type = table,
        .first_child_offset = c_first_gaia_table_gaia_table,
        .next_child_offset = c_next_gaia_table_gaia_table,
        .parent_offset = c_parent_gaia_table_gaia_database,
        .cardinality = cardinality_t::many,
        .parent_required = false});

    auto table_field_relationship = make_shared<relationship_t>(relationship_t{
        .parent_type = table,
        .child_type = field,
        .first_child_offset = c_first_gaia_field_gaia_field,
        .next_child_offset = c_next_gaia_field_gaia_field,
        .parent_offset = c_parent_gaia_field_gaia_table,
        .cardinality = cardinality_t::many,
        .parent_required = false});

    auto relationship_parent_table_relationship = make_shared<relationship_t>(relationship_t{
        .parent_type = table,
        .child_type = relationship,
        .first_child_offset = c_first_parent_gaia_relationship,
        .next_child_offset = c_next_parent_gaia_relationship,
        .parent_offset = c_parent_parent_gaia_table,
        .cardinality = cardinality_t::many,
        .parent_required = false});

    auto relationship_child_table_relationship = make_shared<relationship_t>(relationship_t{
        .parent_type = table,
        .child_type = relationship,
        .first_child_offset = c_first_child_gaia_relationship,
        .next_child_offset = c_next_child_gaia_relationship,
        .parent_offset = c_parent_child_gaia_table,
        .cardinality = cardinality_t::many,
        .parent_required = false});

    auto ruleset_rule_relationship = make_shared<relationship_t>(relationship_t{
        .parent_type = ruleset,
        .child_type = rule,
        .first_child_offset = c_first_gaia_rule_gaia_rule,
        .next_child_offset = c_next_gaia_rule_gaia_rule,
        .parent_offset = c_parent_gaia_rule_gaia_ruleset,
        .cardinality = cardinality_t::many,
        .parent_required = false});

    auto& db_metadata = get_or_create_no_lock(database);
    db_metadata.add_parent_relationship(db_table_relationship);
    db_metadata.mark_as_initialized();

    auto& table_metadata = get_or_create_no_lock(table);
    table_metadata.add_child_relationship(db_table_relationship);
    table_metadata.add_parent_relationship(table_field_relationship);
    table_metadata.add_parent_relationship(relationship_child_table_relationship);
    table_metadata.add_parent_relationship(relationship_parent_table_relationship);
    table_metadata.mark_as_initialized();

    auto& field_metadata = get_or_create_no_lock(field);
    field_metadata.add_child_relationship(table_field_relationship);
    field_metadata.mark_as_initialized();

    auto& relationship_metadata = get_or_create_no_lock(relationship);
    relationship_metadata.add_child_relationship(relationship_parent_table_relationship);
    relationship_metadata.add_child_relationship(relationship_child_table_relationship);
    relationship_metadata.mark_as_initialized();

    auto& ruleset_metadata = get_or_create_no_lock(ruleset);
    ruleset_metadata.add_parent_relationship(ruleset_rule_relationship);
    ruleset_metadata.mark_as_initialized();

    auto& rule_metadata = get_or_create_no_lock(rule);
    rule_metadata.add_child_relationship(ruleset_rule_relationship);
    rule_metadata.mark_as_initialized();
}

gaia_id_t type_registry_t::get_record_id(gaia_type_t type)
{
    return type_id_record_id_cache_t::instance().get_record_id(type);
}

bool type_registry_t::exists(gaia_type_t type) const
{
    shared_lock lock(m_registry_lock);

    return m_metadata_registry.find(type) != m_metadata_registry.end();
}

const type_metadata_t& type_registry_t::get(gaia_type_t type)
{
    unique_lock lock(m_registry_lock);
    auto it = m_metadata_registry.find(type);

    if (it != m_metadata_registry.end() && it->second->is_initialized())
    {
        return *it->second;
    }

    return create(type);
}

void type_registry_t::clear()
{
    m_metadata_registry.clear();
    init();
}

type_metadata_t& type_registry_t::test_get_or_create(gaia_type_t type)
{
    auto& metadata = get_or_create_no_lock(type);
    metadata.mark_as_initialized();
    return metadata;
}

static shared_ptr<relationship_t> create_relationship(gaia_relationship_t relationship)
{
    return make_shared<relationship_t>(relationship_t{
        .parent_type = static_cast<gaia_type_t>(relationship.parent_gaia_table().type()),
        .child_type = static_cast<gaia_type_t>(relationship.child_gaia_table().type()),
        .first_child_offset = relationship.first_child_offset(),
        .next_child_offset = relationship.next_child_offset(),
        .parent_offset = relationship.parent_offset(),
        .cardinality = cardinality_t::many,
        .parent_required = false});
}

type_metadata_t& type_registry_t::create(gaia_type_t type)
{
    gaia_log::db().trace("Creating metadata for type: {}", type);

    gaia_id_t record_id = get_record_id(type);
    gaia_table_t child_table = gaia_table_t::get(record_id);

    if (!child_table)
    {
        throw type_not_found(type);
    }

    auto& metadata = get_or_create_no_lock(type);

    for (auto& relationship : child_table.child_gaia_relationship_list())
    {
        if (metadata.find_child_relationship(relationship.parent_offset()))
        {
            continue;
        }

        auto rel = create_relationship(relationship);

        gaia_log::db().trace(
            " with relationship parent: {}, child: {}, first_child_offset: {}, parent_offset: {}, next_child_offset: {}",
            rel->parent_type, rel->child_type, rel->first_child_offset, rel->parent_offset, rel->next_child_offset);

        auto& parent_meta = get_or_create_no_lock(rel->parent_type);
        parent_meta.add_parent_relationship(rel);

        metadata.add_child_relationship(rel);
    }

    for (auto& relationship : child_table.parent_gaia_relationship_list())
    {
        if (metadata.find_parent_relationship(relationship.first_child_offset()))
        {
            continue;
        }

        auto rel = create_relationship(relationship);

        gaia_log::db().trace(
            " with relationship parent: {}, child: {}, first_child_offset: {}, parent_offset: {}, next_child_offset: {}",
            rel->parent_type, rel->child_type, rel->first_child_offset, rel->parent_offset, rel->next_child_offset);

        auto& child_meta = get_or_create_no_lock(rel->child_type);
        child_meta.add_child_relationship(rel);

        metadata.add_parent_relationship(rel);
    }

    metadata.mark_as_initialized();
    return metadata;
}

type_metadata_t& type_registry_t::get_or_create_no_lock(gaia_type_t type)
{
    auto it = m_metadata_registry.find(type);

    if (it != m_metadata_registry.end())
    {
        return *it->second;
    }

    auto it2 = m_metadata_registry.emplace(type, make_unique<type_metadata_t>(type));
    return *it2.first->second;
}

} // namespace db
} // namespace gaia
