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

struct catalog_se_object_view_t
{
    explicit catalog_se_object_view_t(const gaia_se_object_t* obj_ptr)
        : m_obj_ptr{obj_ptr}
    {
    }

    [[nodiscard]] gaia_id_t id() const
    {
        return m_obj_ptr->id;
    }

    [[nodiscard]] gaia_type_t type() const
    {
        return m_obj_ptr->type;
    }

protected:
    const gaia_se_object_t* m_obj_ptr;
};

struct field_view_t : catalog_se_object_view_t
{
    using catalog_se_object_view_t::catalog_se_object_view_t;
    [[nodiscard]] const char* name() const;
    [[nodiscard]] data_type_t data_type() const;
    [[nodiscard]] field_position_t position() const;
};

struct table_view_t : catalog_se_object_view_t
{
    using catalog_se_object_view_t::catalog_se_object_view_t;
    [[nodiscard]] const char* name() const;
    [[nodiscard]] gaia_type_t table_type() const;
    [[nodiscard]] std::vector<uint8_t> binary_schema() const;
    [[nodiscard]] std::vector<uint8_t> serialization_template() const;
};

using field_list_t = common::iterators::range_t<common::iterators::generator_iterator_t<field_view_t>>;
using table_list_t = common::iterators::range_t<common::iterators::generator_iterator_t<table_view_t>>;

class catalog_core_t
{
private:
    // Constants for reference slots in catalog records.
    // They need to be updated when the corresponding catalog table definition change.
    //
    // The ref slot in gaia_table pointing to the first gaia_field
    static constexpr uint16_t c_gaia_table_first_gaia_field_slot = 2;
    // The ref slot in gaia_field pointing to the gaia_table
    static constexpr uint16_t c_gaia_field_parent_gaia_table_slot = 0;
    // The ref slot in gaia_field pointing to the next gaia_field
    static constexpr uint16_t c_gaia_field_next_gaia_field_slot = 1;

    [[nodiscard]] static inline const gaia_se_object_t* get_se_object_ptr(gaia_id_t);

public:
    static table_view_t get_table(gaia_id_t table_id);
    static table_list_t list_tables();
    static field_list_t list_fields(gaia_type_t table_type);
};

} // namespace db
} // namespace gaia
