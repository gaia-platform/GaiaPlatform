/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "type_metadata.hpp"

#include <shared_mutex>

#include "gaia_catalog.h"
#include "gaia_catalog.hpp"
#include "logger.hpp"
#include "system_table_types.hpp"
#include "type_id_record_id_cache.hpp"

using gaia::catalog::data_type_t;
using gaia::catalog::gaia_table_t;

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

void type_metadata_t::add_parent_relationship(reference_offset_t first_child, const shared_ptr<relationship_t>& relationship)
{
    unique_lock lock(m_metadata_lock);

    m_parent_relationships.insert({first_child, relationship});
}

void type_metadata_t::add_child_relationship(reference_offset_t parent, const shared_ptr<relationship_t>& relationship)
{
    unique_lock lock(m_metadata_lock);

    m_child_relationships.insert({parent, relationship});
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
    vector<catalog_table_type_t> system_types = {
        catalog_table_type_t::gaia_database,
        catalog_table_type_t::gaia_table,
        catalog_table_type_t::gaia_field,
        catalog_table_type_t::gaia_relationship,
        catalog_table_type_t::gaia_rule,
        catalog_table_type_t::gaia_ruleset};

    for (auto type : system_types)
    {
        get_or_create_no_lock(static_cast<gaia_type_t>(type));
    }
}

gaia_id_t type_registry_t::get_record_id(gaia_type_t type)
{
    type_id_record_id_cache_t type_table_cache;
    return type_table_cache.get_record_id(type);
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

type_metadata_t& type_registry_t::create(gaia_type_t type)
{
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

        gaia_table_t parent_table = relationship.parent_gaia_table();

        auto rel = make_shared<relationship_t>(relationship_t{
            .parent_type = static_cast<gaia_type_t>(parent_table.type()),
            .child_type = static_cast<gaia_type_t>(child_table.type()),
            .first_child_offset = relationship.first_child_offset(),
            .next_child_offset = relationship.next_child_offset(),
            .parent_offset = relationship.parent_offset(),
            .cardinality = cardinality_t::many,
            .parent_required = false});

        auto& parent_meta = get_or_create_no_lock(parent_table.type());
        parent_meta.add_parent_relationship(relationship.first_child_offset(), rel);

        metadata.add_child_relationship(relationship.parent_offset(), rel);
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
