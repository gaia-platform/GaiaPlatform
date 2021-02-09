/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#pragma once
#include <string>
#include <vector>

#include <flatbuffers/flatbuffers.h>

#include "gaia/common.hpp"
#include "gaia/db/db.hpp"

#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/common/system_table_types.hpp"
#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/db_types.hpp"

namespace gaia
{
namespace db
{

struct catalog_db_object_view_t
{
    explicit catalog_db_object_view_t(const db_object_t* obj_ptr)
        : m_obj_ptr{obj_ptr}
    {
    }

    [[nodiscard]] common::gaia_id_t id() const
    {
        return m_obj_ptr->id;
    }

    [[nodiscard]] common::gaia_type_t type() const
    {
        return m_obj_ptr->type;
    }

protected:
    const db_object_t* m_obj_ptr;
};

struct field_view_t : catalog_db_object_view_t
{
    using catalog_db_object_view_t::catalog_db_object_view_t;
    [[nodiscard]] const char* name() const;
    [[nodiscard]] common::data_type_t data_type() const;
    [[nodiscard]] common::field_position_t position() const;
};

using buffer = const flatbuffers::Vector<uint8_t>;

struct table_view_t : catalog_db_object_view_t
{
    using catalog_db_object_view_t::catalog_db_object_view_t;
    [[nodiscard]] const char* name() const;
    [[nodiscard]] common::gaia_type_t table_type() const;
    [[nodiscard]] buffer* binary_schema() const;
    [[nodiscard]] std::vector<uint8_t> serialization_template() const;
};

struct relationship_view_t : catalog_db_object_view_t
{
    static constexpr common::reference_offset_t c_parent_gaia_table_ref_offset = 0;
    static constexpr common::reference_offset_t c_child_gaia_table_ref_offset = 2;

    using catalog_db_object_view_t::catalog_db_object_view_t;
    [[nodiscard]] const char* name() const;
    [[nodiscard]] common::gaia_id_t parent_table_id() const;
    [[nodiscard]] common::gaia_id_t child_table_id() const;
    [[nodiscard]] common::reference_offset_t first_child_offset() const;
    [[nodiscard]] common::reference_offset_t next_child_offset() const;
    [[nodiscard]] common::reference_offset_t parent_offset() const;
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
    static constexpr common::reference_offset_t c_gaia_field_parent_gaia_table_offset = 0;
    // The ref slot in gaia_field pointing to the next gaia_field.
    static constexpr common::reference_offset_t c_gaia_field_next_gaia_field_offset = 1;
    //
    // The ref slot in gaia_table pointing to the parent gaia_database.
    static constexpr common::reference_offset_t c_gaia_table_parent_gaia_database_offset = 0;
    // The ref slot in gaia_table pointing to the next gaia_table.
    static constexpr common::reference_offset_t c_gaia_table_next_gaia_table_offset = 1;
    // The ref slot in gaia_table pointing to the first gaia_field.
    static constexpr common::reference_offset_t c_gaia_table_first_gaia_field_offset = 2;
    // The ref slot in gaia_table pointing to the first parent gaia_relationship.
    static constexpr common::reference_offset_t c_gaia_table_first_parent_gaia_relationship_offset = 3;
    // The ref slot in gaia_table pointing to the first child gaia_relationship.
    static constexpr common::reference_offset_t c_gaia_table_first_child_gaia_relationship_offset = 4;
    //
    // The ref slot in gaia_relationship pointing to the parent gaia_table.
    static constexpr common::reference_offset_t c_gaia_relationship_parent_parent_gaia_table_offset = 0;
    // The ref slot in gaia_relationship pointing to the next parent gaia_relationship.
    static constexpr common::reference_offset_t c_gaia_relationship_next_parent_gaia_relationship_offset = 1;
    // The ref slot in gaia_relationship pointing to the parent child gaia_relationship.
    static constexpr common::reference_offset_t c_gaia_relationship_parent_child_gaia_table_offset = 2;
    // The ref slot in gaia_relationship pointing to the next child gaia_relationship.
    static constexpr common::reference_offset_t c_gaia_relationship_next_child_gaia_relationship_offset = 3;
    //
    // The ref slot in gaia_database pointing to the first gaia_table.
    static constexpr common::reference_offset_t c_gaia_database_first_gaia_table_offset = 0;
    //
    // The ref slot in gaia_ruleset pointing to the first gaia_rule.
    static constexpr common::reference_offset_t c_gaia_ruleset_first_gaia_rule_offset = 0;
    //
    // The ref slot in gaia_rule pointing to the parent gaia_ruleset.
    static constexpr common::reference_offset_t c_gaia_rule_parent_gaia_ruleset_offset = 0;
    // The ref slot in gaia_rule pointing to the next gaia_rule.
    static constexpr common::reference_offset_t c_gaia_rule_next_gaia_rule_offset = 1;

    [[nodiscard]] static inline const db_object_t* get_db_object_ptr(common::gaia_id_t);

    static table_view_t get_table(common::gaia_id_t table_id);
    static table_list_t list_tables();
    static field_list_t list_fields(common::gaia_id_t table_id);

    // List all the relationship(s) originating from the given table.
    static relationship_list_t list_relationship_from(common::gaia_id_t table_id);
    // List all the relationship(s) pointing to the given table.
    static relationship_list_t list_relationship_to(common::gaia_id_t table_id);
};

} // namespace db
} // namespace gaia
