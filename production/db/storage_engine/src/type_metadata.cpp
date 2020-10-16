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

type_metadata_t& type_registry_t::get_or_create(gaia_type_t type)
{
    // to improve read performance it is possible add a read only method
    // that uses a shared_lock(). Such method should be call when it is
    // possible to assume that the registry has already been populated.
    scoped_lock lock(m_registry_lock);
    auto [it, result] = m_metadata_registry.try_emplace(type, type);
    return it->second;
}

void type_registry_t::clear()
{
    m_metadata_registry.clear();
}

} // namespace db
} // namespace gaia
