/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "field_list.hpp"

#include <algorithm>
#include <memory>

#include "gaia/direct_access/auto_transaction.hpp"

#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/retail_assert.hpp"

using namespace gaia::db::payload_types;
using namespace gaia::common;
using namespace std;

field_list_t::field_list_t(gaia_id_t type_id)
    : m_type_id(type_id), m_data(nullptr)
{
    // Right now the backing structure for the field_list is a lazily initialized vector.
    // The implementation might change in future to enhance performance.
    // This is to keep empty versions of this structure small to reduce overhead during runtime.
    //
    // initialize() will be called on the first add().
}

field_list_t::field_list_t(const field_list_t& other)
    : m_type_id(other.m_type_id), m_data((other.m_data) ? make_unique<vector<field_position_t>>(*other.m_data) : nullptr)
{
}

field_list_t& field_list_t::operator=(const field_list_t& other)
{
    if (this != &other)
    {
        m_type_id = other.m_type_id;
        m_data = (other.m_data) ? make_unique<vector<field_position_t>>(*other.m_data) : nullptr;
    }
    return *this;
}

field_position_t field_list_t::operator[](size_t idx) const
{
    if (idx >= size())
    {
        throw field_list_index_out_of_bounds();
    }
    return (*m_data)[idx];
}

// Initialize backing structure on this list.
void field_list_t::initialize()
{
    retail_assert(m_data == nullptr, "field list already initialized");
    size_t num_fields = 0;
    auto_transaction_t txn;

    for (auto field : gaia::catalog::gaia_field_t::list())
    {
        if (field.table_gaia_table().gaia_id() == m_type_id)
        {
            num_fields++;
        }
    }
    txn.commit();

    size_t reserve_size = (c_max_vector_reserve < num_fields) ? c_max_vector_reserve : num_fields;
    m_data = make_unique<vector<field_position_t>>();
    m_data->reserve(reserve_size);
}

// Set a field in the field_list.
void field_list_t::add(field_position_t field_pos)
{
    if (!m_data)
    {
        // Lazy initialization here.
        initialize();
    }

    auto it = std::find(m_data->cbegin(), m_data->cend(), field_pos);

    if (it == m_data->cend())
    {
        m_data->push_back(field_pos);
    }
}

// Does the list contain the field?
bool field_list_t::contains(field_position_t field_pos) const
{
    if (is_empty())
    {
        return false;
    }

    auto it = std::find(m_data->cbegin(), m_data->cend(), field_pos);
    return it != m_data->end();
}

// Number of fields in this list.
size_t field_list_t::size() const
{
    // Uninitialized, return 0.
    if (!m_data)
    {
        return 0;
    }
    return m_data->size();
}

// No fields in this list?
bool field_list_t::is_empty() const
{
    return size() == 0;
}

// Check if list is valid. All fields should be associated
// with the table_id.
bool field_list_t::validate() const
{
    // TODO (yiwen): implement
    return true;
}

// Intersection. Returns fields on both lists if table_ids are the same.
field_list_t field_list_t::intersect(const field_list_t& other) const
{
    retail_assert(m_type_id == other.m_type_id, "Incompatible field lists for intersect");
    // TODO (yiwen): implement.
    return field_list_t(other); // PLACEHOLDER: suppress warnings.
}

// Get table_id associated with this list.
gaia_id_t field_list_t::get_type_id() const
{
    return m_type_id;
}
