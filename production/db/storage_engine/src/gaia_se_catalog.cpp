/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////
#include "gaia_se_catalog.hpp"

#include <optional>

#include "flatbuffers/reflection.h"
#include "flatbuffers_helpers.hpp"

#include "gaia_common.hpp"
#include "gaia_db.hpp"
#include "gaia_field_generated.h"
#include "gaia_hash_map.hpp"
#include "gaia_se_object.hpp"
#include "gaia_table_generated.h"
#include "generator_iterator.hpp"
#include "storage_engine_client.hpp"
#include "system_table_types.hpp"

namespace gaia
{
namespace db
{

gaia_field_t::gaia_field_t(const uint8_t* payload)
{
    m_payload = payload;
}

[[nodiscard]] const char* gaia_field_t::name() const
{
    return catalog::Getgaia_field(m_payload)->name()->c_str();
}

[[nodiscard]] data_type_t gaia_field_t::type() const
{
    return static_cast<data_type_t>(catalog::Getgaia_field(m_payload)->type());
}

[[nodiscard]] field_position_t gaia_field_t::position() const
{
    return catalog::Getgaia_field(m_payload)->position();
}

const gaia_se_object_t* gaia_catalog_t::get_se_object_ptr(gaia_id_t id)
{
    gaia_locator_t locator = gaia_hash_map::find(client::s_data, client::s_locators, id);
    retail_assert(locator && se_base::locator_exists(client::s_locators, locator));
    return se_base::locator_to_ptr(client::s_locators, client::s_data, locator);
}

vector<uint8_t> gaia_catalog_t::get_bfbs(gaia_type_t table_type)
{
    client::verify_txn_active();
    return gaia::common::flatbuffers_hex_to_buffer(
        catalog::Getgaia_table(get_se_object_ptr((table_type))->data())->binary_schema()->c_str());
}

gaia_field_list_t gaia_catalog_t::list_fields(gaia_type_t table_type)
{
    client::verify_txn_active();
    auto obj_ptr = get_se_object_ptr(table_type);
    const gaia_id_t* references = obj_ptr->references();
    gaia_id_t first_field_id = references[c_gaia_table_first_gaia_field_slot];

    auto gaia_field_generator = [field_id = first_field_id]() mutable -> std::optional<gaia_field_t> {
        if (field_id == INVALID_GAIA_ID)
        {
            return std::nullopt;
        }
        auto field_obj_ptr = get_se_object_ptr(field_id);
        gaia_field_t field{reinterpret_cast<const uint8_t*>(field_obj_ptr->data())};
        field_id = field_obj_ptr->references()[c_gaia_field_next_gaia_field_slot];
        return field;
    };
    return gaia::common::iterators::range_from_generator(gaia_field_generator);
}

} // namespace db
} // namespace gaia
