/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once
#include <string>
#include <vector>

#include "db_types.hpp"
#include "gaia_common.hpp"
#include "gaia_db.hpp"
#include "gaia_se_object.hpp"
#include "generator_iterator.hpp"
#include "system_table_types.hpp"

namespace gaia
{
namespace db
{

class gaia_field_t
{
private:
    const uint8_t* m_payload;

public:
    explicit gaia_field_t(const uint8_t* payload);
    [[nodiscard]] const char* name() const;
    [[nodiscard]] data_type_t type() const;
    [[nodiscard]] field_position_t position() const;
};

using gaia_field_list_t = common::iterators::range_t<common::iterators::generator_iterator_t<gaia_field_t>>;

class gaia_catalog_t
{
private:
    //
    // Constants for refernece slots in catalog records.
    // They need to be updated when the corresponding catalog table definition change.
    //
    // The ref slot in gaia_table pointing to the first gaia_field
    static constexpr uint16_t c_gaia_table_first_gaia_field_slot = 0;
    // The ref slot in gaia_field pointing to the gaia_table
    static constexpr int c_gaia_field_parent_gaia_table_slot = 0;
    // The ref slot in gaia_field pointing to the next gaia_field
    static constexpr int c_gaia_field_next_gaia_field_slot = 1;
    //
    //

    [[nodiscard]] static inline const gaia_se_object_t* get_se_object_ptr(gaia_id_t);

public:
    static vector<uint8_t> get_bfbs(gaia_type_t table_type);
    static gaia_field_list_t list_fields(gaia_type_t table_type);
};

} // namespace db
} // namespace gaia
