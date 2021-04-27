/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/db/catalog_core.hpp"

#include <optional>
#include <vector>

#include "gaia/common.hpp"

#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/common/system_table_types.hpp"
#include "gaia_internal/db/db_types.hpp"

#include "db_helpers.hpp"
#include "db_object_helpers.hpp"
#include "gaia_field_generated.h"
#include "gaia_relationship_generated.h"
#include "gaia_table_generated.h"

using namespace gaia::common;

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

table_view_t catalog_core_t::get_table(gaia_id_t table_id)
{
    return table_view_t{id_to_ptr(table_id)};
}

table_list_t catalog_core_t::list_tables()
{
    counters_t* counters = gaia::db::get_counters();
    auto gaia_table_generator = [counters, locator = c_invalid_gaia_locator]() mutable -> std::optional<table_view_t> {
        // We need an acquire barrier before reading `last_locator`. We can
        // change this full barrier to an acquire barrier when we change to proper
        // C++ atomic types.
        __sync_synchronize();
        while (++locator && locator <= counters->last_locator)
        {
            auto ptr = locator_to_ptr(locator);
            if (ptr && ptr->type == static_cast<gaia_type_t>(catalog_table_type_t::gaia_table))
            {
                return table_view_t(ptr);
            }
            __sync_synchronize();
        }
        return std::nullopt;
    };
    return gaia::common::iterators::range_from_generator(gaia_table_generator);
}

template <typename T_catalog_obj_view>
common::iterators::range_t<common::iterators::generator_iterator_t<T_catalog_obj_view>>
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
    return gaia::common::iterators::range_from_generator(generator);
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

} // namespace db
} // namespace gaia
