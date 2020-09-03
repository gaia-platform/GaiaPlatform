/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_types.hpp"

using std::map;

namespace gaia::db {

optional<relation_t> type_metadata_t::find_parent_relation(relation_offset_t offset) const {
    auto parent_rel = m_parent_relations.find(offset);

    if (parent_rel == m_parent_relations.end()) {
        return {};
    }

    return std::ref(*parent_rel->second);
}

optional<relation_t> type_metadata_t::find_child_relation(relation_offset_t offset) const {
    auto child_rel = m_child_relations.find(offset);

    if (child_rel == m_child_relations.end()) {
        return {};
    }

    return std::ref(*child_rel->second);
}

void type_metadata_t::add_parent_relation(relation_offset_t offset, shared_ptr<relation_t> relation) {
    m_parent_relations.insert({offset, relation});
}

void type_metadata_t::add_child_relation(relation_offset_t offset, shared_ptr<relation_t> relation) {
    m_child_relations.insert({offset, relation});
}

gaia_type_t type_metadata_t::get_type() const {
    return m_type;
}

const type_metadata_t &type_registry_t::get_metadata(gaia_type_t type) const {
    auto metadata = m_metadata_registry.find(type);

    if (metadata == m_metadata_registry.end()) {
        throw metadata_not_found(type);
    }

    return *metadata->second;
}

void type_registry_t::add_metadata(gaia_type_t type, unique_ptr<type_metadata_t> metadata) {
    if (m_metadata_registry.find(type) != m_metadata_registry.end()) {
        throw duplicated_metadata(type);
    }

    m_metadata_registry.insert({type, std::move(metadata)});
}

} // namespace gaia::db