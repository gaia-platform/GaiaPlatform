/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "field_list.hpp"

#include <algorithm>

#include "retail_assert.hpp"

using namespace gaia::db::types;
using namespace gaia::common;
using namespace std;

field_list_t::field_list_t(gaia_id_t table_id)
    : m_table_id(table_id),
    m_data(nullptr) {
    // Right now the backing structure for the field_list is a lazily initialized vector.
    // The implementation might change in future to enhance performance.
    // This is to keep empty versions of this structure small to reduce overhead during runtime.
    //
    // initialize() will be called on the first add().
}

// Copy constructor
field_list_t::field_list_t(const field_list_t& other)
    : m_table_id(other.m_table_id),
    m_data((other.m_data) ? new vector<gaia_id_t>(*other.m_data) : nullptr) {
}

// Initialize backing structure on this list.
void field_list_t::initialize() {
    retail_assert(m_data == nullptr, "field list already initialized");
    // TODO(yiwen): lookup catalog for actual size of table
    size_t reserve_size = c_max_vector_reserve;
    m_data.reset(new vector<gaia_id_t>()); // change to make_unique with C++14 and above.
    m_data->reserve(reserve_size);
}

// Set a field in the field_list.
void field_list_t::add(gaia_id_t field_id) {
    if (!m_data) {
        // Lazy initialization here.
        initialize();
    }

    auto it = std::find(m_data->cbegin(), m_data->cend(), field_id);

    if (it == m_data->cend()) {
        m_data->push_back(field_id);
    }
}

// Does the list contain the field?
bool field_list_t::contains(gaia_id_t field_id) const {
    if (is_empty()) {
        return false;
    }

    auto it = std::find(m_data->cbegin(), m_data->cend(), field_id);
    return it != m_data->end();
}

// Number of fields in this list.
size_t field_list_t::size() const {
    // Uninitialized, return 0.
    if (!m_data) {
        return 0;
    }
    return m_data->size();
}

// No fields in this list?
bool field_list_t::is_empty() const {
    return size() == 0;
}

// Check if list is valid. All fields should be associated
// with the table_id.
bool field_list_t::validate() const {
    // TODO (yiwen): implement
    return true;
}

// Intersection. Returns fields on both lists if table_ids are the same.
// TODO (yiwen): figure out semantics if table_ids do not match.
field_list_t field_list_t::intersect(field_list_t& other) const{
    // TODO (yiwen): implement.
    return field_list_t(other); // PLACEHOLDER: suppress warnings.
}

// Get table_id associated with this list.
gaia_id_t field_list_t::get_table_id() const {
    return m_table_id;
}
