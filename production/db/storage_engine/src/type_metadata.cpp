/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "type_metadata.hpp"

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
size_t type_metadata_t::num_references() {
    return m_parent_relations.size() + (2 * m_child_relations.size());
}

type_metadata_t &type_registry_t::get_metadata(gaia_type_t type) {
    auto metadata = m_metadata_registry.find(type);

    if (metadata == m_metadata_registry.end()) {
        auto new_metadata = make_unique<type_metadata_t>(type);
        m_metadata_registry.insert({type, std::move(new_metadata)});
        return *(m_metadata_registry[type].get());
    }

    return *metadata->second;
}

void type_registry_t::clear() {
    m_metadata_registry.clear();
}

} // namespace gaia::db