/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/db/catalog_core.hpp"

#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "gaia/common.hpp"

#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/common/system_table_types.hpp"
#include "gaia_internal/db/db_types.hpp"

#include "db_helpers.hpp"
#include "db_object_helpers.hpp"
#include "gaia_field_generated.h"
#include "gaia_index_generated.h"
#include "gaia_relationship_generated.h"
#include "gaia_table_generated.h"

using namespace gaia::common;
using namespace gaia::common::iterators;

namespace gaia
{
namespace db
{

[[nodiscard]] const char* field_view_t::name() const
{
    return catalog::Getgaia_field(m_obj_ptr->data())->name()->c_str();
}

[[nodiscard]] data_type_t field_view_t::data_type() const
{
    return static_cast<data_type_t>(catalog::Getgaia_field(m_obj_ptr->data())->type());
}

[[nodiscard]] field_position_t field_view_t::position() const
{
    return catalog::Getgaia_field(m_obj_ptr->data())->position();
}

[[nodiscard]] const char* table_view_t::name() const
{
    return catalog::Getgaia_table(m_obj_ptr->data())->name()->c_str();
}

[[nodiscard]] gaia_type_t table_view_t::table_type() const
{
    return catalog::Getgaia_table(m_obj_ptr->data())->type();
}

[[nodiscard]] buffer* table_view_t::binary_schema() const
{
    return catalog::Getgaia_table(m_obj_ptr->data())->binary_schema();
}

[[nodiscard]] buffer* table_view_t::serialization_template() const
{
    return catalog::Getgaia_table(m_obj_ptr->data())->serialization_template();
}

[[nodiscard]] const char* relationship_view_t::name() const
{
    return catalog::Getgaia_relationship(m_obj_ptr->data())->name()->c_str();
}

[[nodiscard]] const char* relationship_view_t::to_child_name() const
{
    return catalog::Getgaia_relationship(m_obj_ptr->data())->to_child_name()->c_str();
}

[[nodiscard]] const char* relationship_view_t::to_parent_name() const
{
    return catalog::Getgaia_relationship(m_obj_ptr->data())->to_parent_name()->c_str();
}

[[nodiscard]] reference_offset_t relationship_view_t::first_child_offset() const
{
    return catalog::Getgaia_relationship(m_obj_ptr->data())->first_child_offset();
}

[[nodiscard]] reference_offset_t relationship_view_t::next_child_offset() const
{
    return catalog::Getgaia_relationship(m_obj_ptr->data())->next_child_offset();
}

[[nodiscard]] reference_offset_t relationship_view_t::parent_offset() const
{
    return catalog::Getgaia_relationship(m_obj_ptr->data())->parent_offset();
}

[[nodiscard]] gaia_id_t relationship_view_t::parent_table_id() const
{
    return m_obj_ptr->references()[c_parent_gaia_table_ref_offset];
}

[[nodiscard]] gaia_id_t relationship_view_t::child_table_id() const
{
    return m_obj_ptr->references()[c_child_gaia_table_ref_offset];
}

[[nodiscard]] const flatbuffers::Vector<uint16_t>* relationship_view_t::parent_field_positions() const
{
    return catalog::Getgaia_relationship(m_obj_ptr->data())->parent_field_positions();
}

[[nodiscard]] const flatbuffers::Vector<uint16_t>* relationship_view_t::child_field_positions() const
{
    return catalog::Getgaia_relationship(m_obj_ptr->data())->child_field_positions();
}

[[nodiscard]] const char* index_view_t::name() const
{
    return catalog::Getgaia_index(m_obj_ptr->data())->name()->c_str();
}

[[nodiscard]] bool index_view_t::unique() const
{
    return catalog::Getgaia_index(m_obj_ptr->data())->unique();
}

[[nodiscard]] gaia::catalog::index_type_t index_view_t::type() const
{
    return static_cast<gaia::catalog::index_type_t>(
        catalog::Getgaia_index(m_obj_ptr->data())->type());
}

[[nodiscard]] const flatbuffers::Vector<common::gaia_id_t>* index_view_t::fields() const
{
    return catalog::Getgaia_index(m_obj_ptr->data())->fields();
}

[[nodiscard]] gaia_id_t index_view_t::table_id() const
{
    return m_obj_ptr->references()[c_parent_table_ref_offset];
}

table_view_t catalog_core_t::get_table(gaia_id_t table_id)
{
    return table_view_t{id_to_ptr(table_id)};
}

table_generator_t::table_generator_t(generator_iterator_t<gaia_ptr_t>&& iterator)
    : m_gaia_ptr_iterator(std::move(iterator))
{
}

std::optional<table_view_t> table_generator_t::operator()()
{
    if (m_gaia_ptr_iterator)
    {
        gaia_ptr_t gaia_ptr = *m_gaia_ptr_iterator;
        if (gaia_ptr)
        {
            ++m_gaia_ptr_iterator;
            return table_view_t(gaia_ptr.to_ptr());
        }
    }
    return std::nullopt;
}

table_list_t
catalog_core_t::list_tables()
{
    auto gaia_ptr_iterator = table_generator_t(gaia_ptr_t::find_all_iterator(static_cast<gaia_type_t>(catalog_table_type_t::gaia_table)));
    return range_from_generator(gaia_ptr_iterator);
}

template <typename T_catalog_obj_view>
generator_range_t<T_catalog_obj_view>
list_catalog_obj_reference_chain(gaia_id_t table_id, uint16_t first_offset, uint16_t next_offset)
{
    auto obj_ptr = id_to_ptr(table_id);
    const gaia_id_t* references = obj_ptr->references();
    gaia_id_t first_obj_id = references[first_offset];
    auto generator = [id = first_obj_id, next_offset]() mutable -> std::optional<T_catalog_obj_view> {
        if (id == c_invalid_gaia_id)
        {
            return std::nullopt;
        }
        auto obj_ptr = id_to_ptr(id);
        T_catalog_obj_view obj_view{obj_ptr};
        id = obj_ptr->references()[next_offset];
        return obj_view;
    };
    return range_from_generator(generator);
}

field_list_t catalog_core_t::list_fields(gaia_id_t table_id)
{
    return list_catalog_obj_reference_chain<field_view_t>(
        table_id, c_gaia_table_first_gaia_field_offset, c_gaia_field_next_gaia_field_offset);
}

relationship_list_t catalog_core_t::list_relationship_from(gaia_id_t table_id)
{
    return list_catalog_obj_reference_chain<relationship_view_t>(
        table_id,
        c_gaia_table_first_parent_gaia_relationship_offset,
        c_gaia_relationship_next_parent_gaia_relationship_offset);
}

relationship_list_t catalog_core_t::list_relationship_to(gaia_id_t table_id)
{
    return list_catalog_obj_reference_chain<relationship_view_t>(
        table_id,
        c_gaia_table_first_child_gaia_relationship_offset,
        c_gaia_relationship_next_child_gaia_relationship_offset);
}

index_list_t catalog_core_t::list_indexes(gaia_id_t table_id)
{
    return list_catalog_obj_reference_chain<index_view_t>(
        table_id,
        c_gaia_table_first_gaia_index_offset,
        c_gaia_index_next_gaia_index_offset);
}

} // namespace db
} // namespace gaia
