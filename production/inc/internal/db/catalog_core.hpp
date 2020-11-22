/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once
#include <string>
#include <vector>

#include "gaia/db/gaia_db.hpp"
#include "gaia/gaia_common.hpp"
#include "db_types.hpp"
#include "generator_iterator.hpp"
#include "se_object.hpp"
#include "system_table_types.hpp"

namespace gaia
{
namespace db
{

struct catalog_se_object_view_t
{
    explicit catalog_se_object_view_t(const se_object_t* obj_ptr)
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
    const se_object_t* m_obj_ptr;
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

struct relationship_view_t : catalog_se_object_view_t
{
    static constexpr reference_offset_t c_parent_gaia_table_ref_offset = 0;
    static constexpr reference_offset_t c_child_gaia_table_ref_offset = 2;

    using catalog_se_object_view_t::catalog_se_object_view_t;
    [[nodiscard]] const char* name() const;
    [[nodiscard]] gaia_id_t parent_table_id() const;
    [[nodiscard]] gaia_id_t child_table_id() const;
    [[nodiscard]] reference_offset_t first_child_offset() const;
    [[nodiscard]] reference_offset_t next_child_offset() const;
    [[nodiscard]] reference_offset_t parent_offset() const;
};

using field_list_t = common::iterators::range_t<common::iterators::generator_iterator_t<field_view_t>>;
using table_list_t = common::iterators::range_t<common::iterators::generator_iterator_t<table_view_t>>;
using relationship_list_t = common::iterators::range_t<common::iterators::generator_iterator_t<relationship_view_t>>;

struct catalog_core_t
{
    // Constants for reference slots in catalog records.
    // They need to be updated when the corresponding catalog table definition change.
    //
    // The ref slot in gaia_field pointing to the gaia_table.
    static constexpr reference_offset_t c_gaia_field_parent_gaia_table_offset = 0;
    // The ref slot in gaia_field pointing to the next gaia_field.
    static constexpr reference_offset_t c_gaia_field_next_gaia_field_offset = 1;
    //
    // The ref slot in gaia_table pointing to the parent gaia_database.
    static constexpr reference_offset_t c_gaia_table_parent_gaia_database_offset = 0;
    // The ref slot in gaia_table pointing to the next gaia_table.
    static constexpr reference_offset_t c_gaia_table_next_gaia_table_offset = 1;
    // The ref slot in gaia_table pointing to the first gaia_field.
    static constexpr reference_offset_t c_gaia_table_first_gaia_field_offset = 2;
    // The ref slot in gaia_table pointing to the first parent gaia_relationship.
    static constexpr reference_offset_t c_gaia_table_first_parent_gaia_relationship_offset = 3;
    // The ref slot in gaia_table pointing to the first child gaia_relationship.
    static constexpr reference_offset_t c_gaia_table_first_child_gaia_relationship_offset = 4;
    //
    // The ref slot in gaia_relationship pointing to the parent gaia_table.
    static constexpr reference_offset_t c_gaia_relationship_parent_parent_gaia_table_offset = 0;
    // The ref slot in gaia_relationship pointing to the next parent gaia_relationship.
    static constexpr reference_offset_t c_gaia_relationship_next_parent_gaia_relationship_offset = 1;
    // The ref slot in gaia_relationship pointing to the parent child gaia_relationship.
    static constexpr reference_offset_t c_gaia_relationship_parent_child_gaia_table_offset = 2;
    // The ref slot in gaia_relationship pointing to the next child gaia_relationship.
    static constexpr reference_offset_t c_gaia_relationship_next_child_gaia_relationship_offset = 3;
    //
    // The ref slot in gaia_database pointing to the first gaia_table.
    static constexpr reference_offset_t c_gaia_database_first_gaia_table_offset = 0;
    //
    // The ref slot in gaia_ruleset pointing to the first gaia_rule.
    static constexpr reference_offset_t c_gaia_ruleset_first_gaia_rule_offset = 0;
    //
    // The ref slot in gaia_rule pointing to the parent gaia_ruleset.
    static constexpr reference_offset_t c_gaia_rule_parent_gaia_ruleset_offset = 0;
    // The ref slot in gaia_rule pointing to the next gaia_rule.
    static constexpr reference_offset_t c_gaia_rule_next_gaia_rule_offset = 1;

    [[nodiscard]] static inline const se_object_t* get_se_object_ptr(gaia_id_t);

    static table_view_t get_table(gaia_id_t table_id);
    static table_list_t list_tables();
    static field_list_t list_fields(gaia_id_t table_id);

    // List all the relationship(s) originating from the given table.
    static relationship_list_t list_relationship_from(gaia_id_t table_id);
    // List all the relationship(s) pointing to the given table.
    static relationship_list_t list_relationship_to(gaia_id_t table_id);
};

} // namespace db
} // namespace gaia
