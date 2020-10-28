/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "catalog_view.hpp"

#include <optional>

#include "flatbuffers/reflection.h"

#include "db_types.hpp"
#include "flatbuffers_helpers.hpp"
#include "gaia_common.hpp"
#include "gaia_db.hpp"
#include "gaia_field_generated.h"
#include "gaia_hash_map.hpp"
#include "gaia_se_object.hpp"
#include "gaia_table_generated.h"
#include "generator_iterator.hpp"
#include "retail_assert.hpp"
#include "storage_engine_client.hpp"
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

[[nodiscard]] vector<uint8_t> table_view_t::binary_schema() const
{
    return gaia::common::flatbuffers_hex_to_buffer(
        catalog::Getgaia_table(m_obj_ptr->data())->binary_schema()->c_str());
}

const gaia_se_object_t* gaia_catalog_t::get_se_object_ptr(gaia_id_t id)
{
    gaia_locator_t locator = gaia_hash_map::find(client::s_data, client::s_locators, id);
    retail_assert(locator && se_base::locator_exists(client::s_locators, locator));
    return se_base::locator_to_ptr(client::s_locators, client::s_data, locator);
}

table_view_t gaia_catalog_t::get_table(gaia_id_t table_id)
{
    client::verify_txn_active();
    return table_view_t{get_se_object_ptr(table_id)};
}

table_list_t gaia_catalog_t::list_tables()
{
    client::verify_txn_active();
    auto gaia_table_generator = [locator = INVALID_GAIA_LOCATOR]() mutable -> std::optional<table_view_t> {
        while (++locator && locator < client::s_data->locator_count + 1)
        {
            gaia_se_object_t* ptr = se_base::locator_to_ptr(client::s_locators, client::s_data, locator);
            if (ptr && ptr->type == static_cast<gaia_type_t>(catalog_table_type_t::gaia_table))
            {
                return table_view_t(ptr);
            }
        }
        return std::nullopt;
    };
    return gaia::common::iterators::range_from_generator(gaia_table_generator);
}

field_list_t gaia_catalog_t::list_fields(gaia_type_t table_type)
{
    client::verify_txn_active();
    auto obj_ptr = get_se_object_ptr(table_type);
    const gaia_id_t* references = obj_ptr->references();
    gaia_id_t first_field_id = references[c_gaia_table_first_gaia_field_slot];
    auto gaia_field_generator = [field_id = first_field_id]() mutable -> std::optional<field_view_t> {
        if (field_id == INVALID_GAIA_ID)
        {
            return std::nullopt;
        }
        auto field_obj_ptr = get_se_object_ptr(field_id);
        field_view_t field_view{field_obj_ptr};
        field_id = field_obj_ptr->references()[c_gaia_field_next_gaia_field_slot];
        return field_view;
    };
    return gaia::common::iterators::range_from_generator(gaia_field_generator);
}

} // namespace db
} // namespace gaia
