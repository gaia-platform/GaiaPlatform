/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include "type_metadata.hpp"

using std::map;

namespace gaia {

namespace db {

// Child relationship contains 2 pointer for every relationship.
constexpr std::size_t c_child_relation_num_ptrs = 2;

relationship_t* type_metadata_t::find_parent_relationship(relationship_offset_t first_child) const {
    auto parent_relation = m_parent_relationships.find(first_child);

    if (parent_relation == m_parent_relationships.end()) {
        return nullptr;
    }

    return parent_relation->second.get();
}

relationship_t* type_metadata_t::find_child_relationship(relationship_offset_t parent) const {
    auto child_relation = m_child_relationships.find(parent);

    if (child_relation == m_child_relationships.end()) {
        return nullptr;
    }

    return child_relation->second.get();
}

void type_metadata_t::add_parent_relationship(relationship_offset_t first_child, const shared_ptr<relationship_t>& relationship) {
    m_parent_relationships.insert({first_child, relationship});
}

void type_metadata_t::add_child_relationship(relationship_offset_t parent, const shared_ptr<relationship_t>& relationship) {
    m_child_relationships.insert({parent, relationship});
}

gaia_type_t type_metadata_t::get_type() const {
    return m_type;
}
size_t type_metadata_t::num_references() {
    return m_parent_relationships.size() + (c_child_relation_num_ptrs * m_child_relationships.size());
}

type_metadata_t& type_registry_t::get_or_create(gaia_type_t type) {
    auto [it, result] = m_metadata_registry.try_emplace(type, type);
    return it->second;
}

void type_registry_t::clear() {
    m_metadata_registry.clear();
}

} // namespace db
} // namespace gaia
