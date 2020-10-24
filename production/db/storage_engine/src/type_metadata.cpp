/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "type_metadata.hpp"

#include <mutex>

#include "gaia_catalog.h"
#include "gaia_catalog.hpp"
#include "system_table_types.hpp"

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

relationship_t* type_metadata_t::find_parent_relationship(reference_offset_t first_child_offset) const
{
    auto parent_relationship = m_parent_relationships.find(first_child_offset);

    if (parent_relationship == m_parent_relationships.end())
    {
        return nullptr;
    }

    return parent_relationship->second.get();
}

relationship_t* type_metadata_t::find_child_relationship(reference_offset_t parent_offset) const
{
    auto child_relationship = m_child_relationships.find(parent_offset);

    if (child_relationship == m_child_relationships.end())
    {
        return nullptr;
    }

    return child_relationship->second.get();
}

void type_metadata_t::add_parent_relationship(reference_offset_t first_child, const shared_ptr<relationship_t>& relationship)
{
    m_parent_relationships.insert({first_child, relationship});
}

void type_metadata_t::add_child_relationship(reference_offset_t parent, const shared_ptr<relationship_t>& relationship)
{
    m_child_relationships.insert({parent, relationship});
}

gaia_type_t type_metadata_t::get_type() const
{
    return m_type;
}

size_t type_metadata_t::num_references()
{
    return m_parent_relationships.size() + (c_child_relation_num_ptrs * m_child_relationships.size());
}

bool type_metadata_t::is_initialized()
{
    return m_initialized;
}

void type_metadata_t::set_initialized()
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

bool type_registry_t::exists(gaia_type_t type)
{
    shared_lock lock(m_registry_lock);

    return m_metadata_registry.find(type) != m_metadata_registry.end();
}

type_metadata_t& type_registry_t::get(gaia_type_t type)
{
    scoped_lock lock(m_registry_lock);
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
    metadata.set_initialized();
    return metadata;
}

type_metadata_t& type_registry_t::create(gaia_type_t table_id)
{
    gaia_table_t child_table = gaia_table_t::get(table_id);

    if (!child_table)
    {
        throw type_not_found(table_id);
    }

    auto& metadata = get_or_create_no_lock(table_id);

    for (auto& field : child_table.gaia_field_list())
    {
        if (field.type() == static_cast<uint8_t>(data_type_t::e_references))
        {
            for (auto& relationship : field.child_gaia_relationship_list())
            {
                if (metadata.find_child_relationship(relationship.parent_offset()))
                {
                    continue;
                }

                gaia_table_t parent_table = relationship.parent_gaia_table();

                auto rel = make_shared<relationship_t>(relationship_t{
                    .parent_type = static_cast<gaia_type_t>(parent_table.gaia_id()),
                    .child_type = static_cast<gaia_type_t>(child_table.gaia_id()),
                    .first_child_offset = relationship.first_child_offset(),
                    .next_child_offset = relationship.next_child_offset(),
                    .parent_offset = relationship.parent_offset(),
                    .cardinality = cardinality_t::many,
                    .parent_required = false});

                auto& parent_meta = get_or_create_no_lock(parent_table.gaia_id());
                parent_meta.add_parent_relationship(relationship.first_child_offset(), rel);

                metadata.add_child_relationship(relationship.parent_offset(), rel);
            }
        }
    }

    metadata.set_initialized();
    return metadata;
}

type_metadata_t& type_registry_t::get_or_create_no_lock(gaia_type_t type)
{
    auto it = m_metadata_registry.find(type);

    if (it != m_metadata_registry.end())
    {
        return *it->second;
    }

    auto metadata = new type_metadata_t(type);
    m_metadata_registry.insert({type, unique_ptr<type_metadata_t>(metadata)});
    return *metadata;
}

} // namespace db
} // namespace gaia
