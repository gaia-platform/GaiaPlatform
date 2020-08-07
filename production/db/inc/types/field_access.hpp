/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "flatbuffers/reflection.h"

#include <data_holder.hpp>
#include <types.hpp>
#include <gaia_exception.hpp>
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
    invalid_field_position(field_position_t position);
};

class unhandled_field_type: public gaia::common::gaia_exception
{
public:
    unhandled_field_type(size_t field_type);
};

class invalid_serialized_field_data: public gaia::common::gaia_exception
{
public:
    invalid_serialized_field_data(field_position_t position);
};

// GENERAL API NOTES
//
// These functions provide read/write access to table record field values.
//
// The caller is responsible for handling concurrency issues
// and for protecting the memory passed in to the API
// during its execution.
//
// The access of this API to the type_cache is protected by the type_cache API.

// Parse the binary schema and insert its Field definitions
// into the provided field_cache.
//
// Note that the Field definitions are not copied,
// so the caller must ensure that they remain valid
// throughout the use of the field_cache instance.
void initialize_field_cache_from_binary_schema(
    field_cache_t* field_cache,
    const uint8_t* binary_schema);

// Get the field value of a table record payload.
// The value will be packed in a data_holder_t structure.
//
// The binary_schema passed in will only be used if the type_cache
// does not already contain a field_cache entry for the type_id.
//
// If the binary_schema is needed but not provided,
// an invalid_schema() exception will be thrown.
data_holder_t get_field_value(
    gaia_id_t type_id,
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    field_position_t field_position);

// Set the scalar field value of a table record payload.
// The new value is provided in a data_holder_t structure.
//
// The binary_schema passed in will only be used if the type_cache
// does not already contain a field_cache entry for the type_id.
//
// If the binary_schema is needed but not provided,
// an invalid_schema() exception will be thrown.
//
// This function only works for scalar fields (integers and floating point numbers).
bool set_field_value(
    gaia_id_t type_id,
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    field_position_t field_position,
    const data_holder_t& value);

// Get the size of a field of array type.
//
// Callers should check the catalog to determine
// whether they access a scalar field or an array field,
// and then proceed accordingly to access the field content.
size_t get_field_array_size(
    gaia_id_t type_id,
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    field_position_t field_position);

// Get a specific element of a field of array type.
//
// An exception will be thrown if the index is out of bounds.
// Callers can first call get_table_field_array_size()
// to find the array's size.
data_holder_t get_field_array_element(
    gaia_id_t type_id,
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    field_position_t field_position,
    size_t array_index);

// Set a specific element of a scalar field of array type.
//
// An exception will be thrown if the index is out of bounds.
// Callers can first call get_table_field_array_size()
// to find the array's size.
//
// This function only works for scalar fields (integers and floating point numbers).
void set_field_array_element(
    gaia_id_t type_id,
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    field_position_t field_position,
    size_t array_index,
    const data_holder_t& value);

}
}
}
