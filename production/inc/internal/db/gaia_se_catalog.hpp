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

struct gaia_field_view_t
{
    explicit gaia_field_view_t(const gaia_se_object_t* obj_ptr)
        : m_gaia_se_object(obj_ptr)
    {
    }

    [[nodiscard]] gaia_id_t id() const;
    [[nodiscard]] gaia_type_t type() const;
    [[nodiscard]] const char* name() const;
    [[nodiscard]] data_type_t data_type() const;
    [[nodiscard]] field_position_t position() const;

private:
    const gaia_se_object_t* m_gaia_se_object;
};

struct gaia_table_view_t
{
    explicit gaia_table_view_t(const gaia_se_object_t* obj_ptr)
        : m_gaia_se_object(obj_ptr)
    {
    }

    [[nodiscard]] gaia_id_t id() const;
    [[nodiscard]] gaia_type_t type() const;
    [[nodiscard]] const char* name() const;
    [[nodiscard]] gaia_type_t table_type() const;

private:
    const gaia_se_object_t* m_gaia_se_object;
};

using gaia_field_list_t = common::iterators::range_t<common::iterators::generator_iterator_t<gaia_field_view_t>>;
using gaia_table_list_t = common::iterators::range_t<common::iterators::generator_iterator_t<gaia_table_view_t>>;

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
    static constexpr uint16_t c_gaia_field_parent_gaia_table_slot = 0;
    // The ref slot in gaia_field pointing to the next gaia_field
    static constexpr uint16_t c_gaia_field_next_gaia_field_slot = 1;
    //

    [[nodiscard]] static inline const gaia_se_object_t* get_se_object_ptr(gaia_id_t);

public:
    static vector<uint8_t> get_bfbs(gaia_type_t table_type);
    static gaia_table_list_t list_tables();
    static gaia_field_list_t list_fields(gaia_type_t table_type);
};

} // namespace db
} // namespace gaia
