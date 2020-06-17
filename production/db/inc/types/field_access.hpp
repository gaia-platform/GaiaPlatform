/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "flatbuffers/reflection.h"

#include <gaia_exception.hpp>
#include <type_holder.hpp>
#include <type_cache.hpp>

namespace gaia
{
namespace db
{
namespace types
{

class invalid_schema: public gaia::common::gaia_exception
{
public:
    invalid_schema();
};

class missing_root_type: public gaia::common::gaia_exception
{
public:
    missing_root_type();
};

class invalid_serialized_data: public gaia::common::gaia_exception
{
public:
    invalid_serialized_data();
};

class invalid_field_position: public gaia::common::gaia_exception
{
public:
    invalid_field_position(uint16_t position);
};

class unhandled_field_type: public gaia::common::gaia_exception
{
public:
    unhandled_field_type(size_t field_type);
};

class invalid_serialized_field_data: public gaia::common::gaia_exception
{
public:
    invalid_serialized_field_data(uint16_t position);
};

// Parse the binary schema and insert its Field definitions
// into the provided field_cache.
//
// Note that the Field definitions are not copied,
// so the caller must ensure that they remain valid
// throughout the use of the field_cache instance.
void initialize_field_cache_from_binary_schema(
    field_cache_t* field_cache,
    uint8_t* binary_schema);

// Get the field value of a table record payload.
// The value will be packed in a type_holder_t structure.
//
// The binary_schema passed in will only be used if the type_cache
// does not already contain a field_cache entry for the type_id.
type_holder_t get_table_field_value(
    uint64_t type_id,
    uint8_t* serialized_data,
    uint8_t* binary_schema,
    uint16_t field_position);

}
}
}
