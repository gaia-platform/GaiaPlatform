/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstdint>

#include <memory>
#include <vector>

//TEMPORARY defs
typedef uint64_t gaia_id_t;

using namespace std;

namespace gaia
{
namespace db
{
namespace types
{

// This structure defines list for fields in the table.
class field_list_t {
    private:
    const size_t c_max_vector_reserve = 8;

    gaia_id_t m_table_id;

    // Backing structure for this list.
    unique_ptr< vector<gaia_id_t> > m_data;
    // Initialize the backing structure.
    void initialize();

    public:
    // Constructor, accepts table_id for this list as the input.
    field_list_t(gaia_id_t table_id);

    // Copy constructor
    field_list_t(const field_list_t& old);

    // Is this field a member?
    bool contains(gaia_id_t field_id) const;

    // Number of fields in this list.
    size_t size() const;

    // No changes in this list?
    bool empty() const;

    // Add individual fields.
    void add(gaia_id_t field_id);

    // Additional binary operations
    field_list_t intersect(field_list_t& other);

    // Validate: check if this list is valid against the catalog
    bool validate() const;

    // Getter: table_id
    gaia_id_t get_table_id() const;
};

}
}
}
