/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "type_metadata.hpp"

#include <mutex>

namespace gaia
{
namespace db
{

// Child relationship contains 2 pointer for every relationship.
constexpr std::size_t c_child_relation_num_ptrs = 2;

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

type_metadata_t& type_registry_t::get(gaia_type_t type)
{
    shared_lock lock(m_registry_lock);

    auto metadata = m_metadata_registry.find(type);

    if (metadata == m_metadata_registry.end())
    {
        throw metadata_not_found(type);
    }

    return *metadata->second;
}

type_metadata_t& type_registry_t::get_or_create(gaia_type_t type)
{
    scoped_lock lock(m_registry_lock);
    auto it = m_metadata_registry.find(type);

    if (it != m_metadata_registry.end())
    {
        return *it->second;
    }

    auto metadata = new type_metadata_t(type);
    m_metadata_registry.insert({type, unique_ptr<type_metadata_t>(metadata)});
    return *metadata;
}

void type_registry_t::add(type_metadata_t* metadata)
{
    scoped_lock lock(m_registry_lock);

    auto old_metadata = m_metadata_registry.find(metadata->get_type());

    if (old_metadata != m_metadata_registry.end())
    {
        throw duplicate_metadata(metadata->get_type());
    }

    m_metadata_registry.insert({metadata->get_type(), unique_ptr<type_metadata_t>(metadata)});
}

void type_registry_t::update(gaia_type_t type, std::function<void(type_metadata_t&)> update_op)
{
    scoped_lock lock(m_registry_lock);

    auto metadata = m_metadata_registry.find(type);

    if (metadata == m_metadata_registry.end())
    {
        throw metadata_not_found(type);
    }

    update_op(*metadata->second);
}

void type_registry_t::clear()
{
    m_metadata_registry.clear();
}

} // namespace db
} // namespace gaia
