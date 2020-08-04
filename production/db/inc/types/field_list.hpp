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

    gaia_id_t m_table_id;

    // Backing structure for this list.
    unique_ptr< vector<gaia_id_t> > m_data;
    // Initialize the backing structure.
    void initialize();

    public:
    // Constructor, accepts table_id for this list as the input.
    field_list_t(gaia_id_t table_id);

    // Copy and move.
    field_list_t(const field_list_t& other);
    field_list_t& operator=(const field_list_t& other);
    field_list_t(field_list_t&& other) = default;
    field_list_t& operator=(field_list_t&& other) = default;

    gaia_id_t operator[](size_t idx) const;

    // Is this field a member?
    bool contains(gaia_id_t field_id) const;

    // Number of fields in this list.
    size_t size() const;

    // No changes in this list?
    bool is_empty() const;

    // Add individual fields.
    void add(gaia_id_t field_id);

    // Additional binary operations.
    field_list_t intersect(const field_list_t& other) const;

    // Validate: check if this list is valid against the catalog.
    bool validate() const;

    // Getter: table_id.
    gaia_id_t get_table_id() const;
};

}
}
}
