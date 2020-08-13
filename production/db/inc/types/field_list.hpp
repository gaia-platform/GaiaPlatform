/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "gaia_common.hpp"
#include "gaia_exception.hpp"
#include "types.hpp"

using namespace std;
using namespace gaia::common;

namespace gaia
{
namespace db
{
namespace types
{

class field_list_index_out_of_bounds : gaia::common::gaia_exception {
public:

field_list_index_out_of_bounds() {
    m_message = "Field list out of bounds.";
}
};

// This structure defines list for fields in the table.
class field_list_t {
    private:
    static const size_t c_max_vector_reserve = 8;

    gaia_id_t m_type_id;

    // Backing structure for this list.
    unique_ptr< vector<field_position_t> > m_data;
    // Initialize the backing structure.
    void initialize();

    public:
    // Constructor, accepts type_id for this list as the input.
    field_list_t(gaia_id_t type_id);

    // Copy and move.
    field_list_t(const field_list_t& other);
    field_list_t& operator=(const field_list_t& other);
    field_list_t(field_list_t&& other) = default;
    field_list_t& operator=(field_list_t&& other) = default;

    field_position_t operator[](size_t idx) const;

    // Is this field a member?
    bool contains(field_position_t field_pos) const;

    // Number of fields in this list.
    size_t size() const;

    // No changes in this list?
    bool is_empty() const;

    // Add individual fields.
    void add(field_position_t field_pos);

    // Additional binary operations.
    field_list_t intersect(const field_list_t& other) const;

    // Validate: check if this list is valid against the catalog.
    bool validate() const;

    // Getter: type_id.
    gaia_id_t get_type_id() const;
};

}
}
}
