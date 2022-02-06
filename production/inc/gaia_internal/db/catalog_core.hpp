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

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/system_table_types.hpp"
#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"

namespace gaia
{
namespace db
{
namespace catalog_core
{

struct catalog_db_object_view_t
{
    explicit catalog_db_object_view_t(const db_object_t* obj_ptr)
        : m_obj_ptr{obj_ptr}
    {
        ASSERT_PRECONDITION(
            obj_ptr,
            "Unexpected null pointer to a db object. "
            "The view class can only be used to read valid db catalog objects.");
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
    [[nodiscard]] buffer* serialization_template() const;
};

struct relationship_view_t : catalog_db_object_view_t
{
    static constexpr common::reference_offset_t c_parent_gaia_table_ref_offset = 0;
    static constexpr common::reference_offset_t c_child_gaia_table_ref_offset = 3;

    using catalog_db_object_view_t::catalog_db_object_view_t;
    [[nodiscard]] const char* name() const;
    [[nodiscard]] const char* to_parent_name() const;
    [[nodiscard]] const char* to_child_name() const;
    [[nodiscard]] common::gaia_id_t parent_table_id() const;
    [[nodiscard]] common::gaia_id_t child_table_id() const;
    [[nodiscard]] common::reference_offset_t first_child_offset() const;
    [[nodiscard]] common::reference_offset_t next_child_offset() const;
    [[nodiscard]] common::reference_offset_t prev_child_offset() const;
    [[nodiscard]] common::reference_offset_t parent_offset() const;
    [[nodiscard]] const flatbuffers::Vector<uint16_t>* parent_field_positions() const;
    [[nodiscard]] const flatbuffers::Vector<uint16_t>* child_field_positions() const;
    [[nodiscard]] bool is_value_linked() const;
};

struct index_view_t : catalog_db_object_view_t
{
    static constexpr common::reference_offset_t c_parent_table_ref_offset = 0;

    using catalog_db_object_view_t::catalog_db_object_view_t;
    [[nodiscard]] const char* name() const;
    [[nodiscard]] bool unique() const;
    [[nodiscard]] catalog::index_type_t type() const;
    [[nodiscard]] const flatbuffers::Vector<common::gaia_id_t>* fields() const;
    [[nodiscard]] common::gaia_id_t table_id() const;
};

using field_list_t = common::iterators::generator_range_t<field_view_t>;
using table_list_t = common::iterators::generator_range_t<table_view_t>;
using relationship_list_t = common::iterators::generator_range_t<relationship_view_t>;
using index_list_t = common::iterators::generator_range_t<index_view_t>;

// Constants for reference slots in catalog records.
// They need to be updated when the corresponding catalog table definition change.
//
// The ref slot in gaia_field pointing to the gaia_table.
constexpr common::reference_offset_t c_gaia_field_parent_gaia_table_offset = 0;
// The ref slot in gaia_field pointing to the next gaia_field.
constexpr common::reference_offset_t c_gaia_field_next_gaia_field_offset = 1;
// The ref slot in gaia_field pointing to the prev gaia_field.
constexpr common::reference_offset_t c_gaia_field_prev_gaia_field_offset = 2;
//
// The ref slot in gaia_table pointing to the parent gaia_database.
constexpr common::reference_offset_t c_gaia_table_parent_gaia_database_offset = 0;
// The ref slot in gaia_table pointing to the next gaia_table.
constexpr common::reference_offset_t c_gaia_table_next_gaia_table_offset = 1;
// The ref slot in gaia_table pointing to the prev gaia_table.
constexpr common::reference_offset_t c_gaia_table_prev_gaia_table_offset = 2;
// The ref slot in gaia_table pointing to the first gaia_field.
constexpr common::reference_offset_t c_gaia_table_first_gaia_field_offset = 3;
// The ref slot in gaia_table pointing to the first parent gaia_relationship.
constexpr common::reference_offset_t c_gaia_table_first_parent_gaia_relationship_offset = 4;
// The ref slot in gaia_table pointing to the first child gaia_relationship.
constexpr common::reference_offset_t c_gaia_table_first_child_gaia_relationship_offset = 5;
// The ref slot in gaia_table pointing to the first gaia_index.
constexpr common::reference_offset_t c_gaia_table_first_gaia_index_offset = 6;
//
// The ref slot in gaia_relationship pointing to the parent gaia_table.
constexpr common::reference_offset_t c_gaia_relationship_parent_parent_gaia_table_offset = 0;
// The ref slot in gaia_relationship pointing to the next parent gaia_relationship.
constexpr common::reference_offset_t c_gaia_relationship_next_parent_gaia_relationship_offset = 1;
// The ref slot in gaia_relationship pointing to the prev parent gaia_relationship.
constexpr common::reference_offset_t c_gaia_relationship_prev_parent_gaia_relationship_offset = 2;
// The ref slot in gaia_relationship pointing to the parent child gaia_relationship.
constexpr common::reference_offset_t c_gaia_relationship_parent_child_gaia_table_offset = 3;
// The ref slot in gaia_relationship pointing to the next child gaia_relationship.
constexpr common::reference_offset_t c_gaia_relationship_next_child_gaia_relationship_offset = 4;
// The ref slot in gaia_relationship pointing to the prev child gaia_relationship.
constexpr common::reference_offset_t c_gaia_relationship_prev_child_gaia_relationship_offset = 5;
//
// The ref slot in gaia_database pointing to the first gaia_table.
constexpr common::reference_offset_t c_gaia_database_first_gaia_table_offset = 0;
//
// The ref slot in gaia_index pointing to the parent gaia_table.
constexpr common::reference_offset_t c_gaia_index_parent_gaia_table_offset = 0;
// The ref slot in gaia_index pointing to the next gaia_index.
constexpr common::reference_offset_t c_gaia_index_next_gaia_index_offset = 1;
// The ref slot in gaia_index pointing to the prev gaia_index.
constexpr common::reference_offset_t c_gaia_index_prev_gaia_index_offset = 2;

table_view_t get_table(common::gaia_id_t table_id);
table_list_t list_tables();
field_list_t list_fields(common::gaia_id_t table_id);

// List all the relationship(s) originating from the given table.
relationship_list_t list_relationship_from(common::gaia_id_t table_id);
// List all the relationship(s) pointing to the given table.
relationship_list_t list_relationship_to(common::gaia_id_t table_id);

index_list_t list_indexes(common::gaia_id_t table_id);

// TODO: Decide if the method belongs to catalog_core or index headers.
// Helper method to find an index for a given table and field. Return
// invalid_gaia_id if cannot find index (for the field).
common::gaia_id_t find_index(common::gaia_id_t table_id, common::field_position_t field_position);

class table_generator_t : public common::iterators::generator_t<table_view_t>
{
public:
    explicit table_generator_t(common::iterators::generator_iterator_t<gaia_ptr_t>&& iterator);

    std::optional<table_view_t> operator()() final;

private:
    common::iterators::generator_iterator_t<gaia_ptr_t> m_gaia_ptr_iterator;
};

} // namespace catalog_core
} // namespace db
} // namespace gaia
