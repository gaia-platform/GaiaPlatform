/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "catalog_core.hpp"

#include <optional>
#include <vector>

#include "flatbuffers/reflection.h"

#include "db_types.hpp"
#include "flatbuffers_helpers.hpp"
#include "gaia_common.hpp"
#include "gaia_db.hpp"
#include "gaia_db_internal.hpp"
#include "gaia_field_generated.h"
#include "gaia_table_generated.h"
#include "generator_iterator.hpp"
#include "retail_assert.hpp"
#include "se_helpers.hpp"
#include "se_object.hpp"
#include "se_object_helpers.hpp"
#include "system_table_types.hpp"

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

[[nodiscard]] std::vector<uint8_t> table_view_t::binary_schema() const
{
    return gaia::common::flatbuffers_hex_to_buffer(
        catalog::Getgaia_table(m_obj_ptr->data())->binary_schema()->c_str());
}

[[nodiscard]] std::vector<uint8_t> table_view_t::serialization_template() const
{
    return gaia::common::flatbuffers_hex_to_buffer(
        catalog::Getgaia_table(m_obj_ptr->data())->serialization_template()->c_str());
}

table_view_t catalog_core_t::get_table(gaia_id_t table_id)
{
    if (!is_transaction_active())
    {
        throw transaction_not_open();
    }

    return table_view_t{id_to_ptr(table_id)};
}

table_list_t catalog_core_t::list_tables()
{
    if (!is_transaction_active())
    {
        throw transaction_not_open();
    }

    data* data = gaia::db::get_shared_data();
    auto gaia_table_generator = [data, locator = c_invalid_gaia_locator]() mutable -> std::optional<table_view_t> {
        // We need an acquire barrier before reading `last_locator`. We can
        // change this full barrier to an acquire barrier when we change to proper
        // C++ atomic types.
        __sync_synchronize();
        while (++locator && locator <= data->last_locator)
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

field_list_t catalog_core_t::list_fields(gaia_id_t table_id)
{
    if (!is_transaction_active())
    {
        throw transaction_not_open();
    }

    auto obj_ptr = id_to_ptr(table_id);
    const gaia_id_t* references = obj_ptr->references();
    gaia_id_t first_field_id = references[c_gaia_table_first_gaia_field_slot];
    auto gaia_field_generator = [field_id = first_field_id]() mutable -> std::optional<field_view_t> {
        if (field_id == c_invalid_gaia_id)
        {
            return std::nullopt;
        }
        auto field_obj_ptr = id_to_ptr(field_id);
        field_view_t field_view{field_obj_ptr};
        field_id = field_obj_ptr->references()[c_gaia_field_next_gaia_field_slot];
        return field_view;
    };
    return gaia::common::iterators::range_from_generator(gaia_field_generator);
}

} // namespace db
} // namespace gaia
