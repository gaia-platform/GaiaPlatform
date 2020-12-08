/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <cstdint>

#include "flatbuffers/reflection.h"

#include "gaia/exception.hpp"
#include "data_holder.hpp"
#include "type_cache.hpp"

namespace gaia
{
namespace db
{
namespace payload_types
{

class invalid_schema : public gaia::common::gaia_exception
{
public:
    invalid_schema();
};

class missing_root_type : public gaia::common::gaia_exception
{
public:
    missing_root_type();
};

class invalid_serialized_data : public gaia::common::gaia_exception
{
public:
    invalid_serialized_data();
};

class invalid_field_position : public gaia::common::gaia_exception
{
public:
    invalid_field_position(gaia::common::field_position_t position);
};

class unhandled_field_type : public gaia::common::gaia_exception
{
public:
    unhandled_field_type(size_t field_type);
};

class cannot_set_null_string_value : public gaia::common::gaia_exception
{
public:
    cannot_set_null_string_value();
};

class cannot_update_null_string_value : public gaia::common::gaia_exception
{
public:
    cannot_update_null_string_value();
};

///////////////////////////////////////////////////////////////////////////////
// GENERAL FIELD ACCESS API NOTES
//
// These functions provide read/write access to table record field values.
//
// The data values are packed in a data_holder_t structure.
//
// The caller is responsible for handling concurrency issues
// and for protecting the memory passed in to the API
// during its execution.
//
// The access of this API to the type_cache is protected by the type_cache API.
//
// The binary_schema passed in will only be used if the type_cache
// does not already contain a type_information entry for the type_id.
// Exception: APIs the set a string field value always need the binary schema.
//
// If the binary_schema is needed, but was not provided,
// an invalid_schema() exception will be thrown.
///////////////////////////////////////////////////////////////////////////////

// Parse the binary schema and insert its Field definitions
// into the provided type_information.
//
// Note that binary schemas that we get passed right now are temporary copies,
// so they need to be copied into the type information as well.
void initialize_type_information_from_binary_schema(
    type_information_t* type_information,
    const uint8_t* binary_schema,
    size_t binary_schema_size);

// Verify that the serialized data matches the schema.
bool verify_data_schema(
    const uint8_t* serialized_data,
    size_t serialized_data_size,
    const uint8_t* binary_schema);

// Get the field value of a table record payload.
data_holder_t get_field_value(
    gaia::common::gaia_type_t type_id,
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    gaia::common::field_position_t field_position);

// Set the scalar field value of a table record payload.
//
// This function only works for scalar fields (integers and floating point numbers).
bool set_field_value(
    gaia::common::gaia_type_t type_id,
    uint8_t* serialized_data,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    gaia::common::field_position_t field_position,
    const data_holder_t& value);

// Set the string field value of a table record payload.
//
// This function only works for string fields.
void set_field_value(
    gaia::common::gaia_type_t type_id,
    std::vector<uint8_t>& serialized_data,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    gaia::common::field_position_t field_position,
    const data_holder_t& value);

// Alternative method for setting string fields,
// when the serialization is not already available in a vector.
std::vector<uint8_t> set_field_value(
    gaia::common::gaia_type_t type_id,
    const uint8_t* serialized_data,
    size_t serialized_data_size,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    gaia::common::field_position_t field_position,
    const data_holder_t& value);

// Get the size of a field of array type.
size_t get_field_array_size(
    gaia::common::gaia_type_t type_id,
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    gaia::common::field_position_t field_position);

// Set the size of a field of array type.
// If the array is expanded, new entries will be set to 0.
void set_field_array_size(
    gaia::common::gaia_type_t type_id,
    std::vector<uint8_t>& serialized_data,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    gaia::common::field_position_t field_position,
    size_t new_size);

// Set the size of a field of array type.
// If the array is expanded, new entries will be set to 0.
std::vector<uint8_t> set_field_array_size(
    gaia::common::gaia_type_t type_id,
    const uint8_t* serialized_data,
    size_t serialized_data_size,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    gaia::common::field_position_t field_position,
    size_t new_size);

// Get a specific element of a field of array type.
//
// An exception will be thrown if the index is out of bounds.
data_holder_t get_field_array_element(
    gaia::common::gaia_type_t type_id,
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    gaia::common::field_position_t field_position,
    size_t array_index);

// Set a specific element of a scalar field of array type.
//
// An exception will be thrown if the index is out of bounds.
//
// This function only works for scalar fields (integers and floating point numbers).
void set_field_array_element(
    gaia::common::gaia_type_t type_id,
    uint8_t* serialized_data,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    gaia::common::field_position_t field_position,
    size_t array_index,
    const data_holder_t& value);

// Set a specific element of a string field of array type.
//
// An exception will be thrown if the index is out of bounds.
//
// This function only works for string fields.
void set_field_array_element(
    gaia::common::gaia_type_t type_id,
    std::vector<uint8_t>& serialized_data,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    gaia::common::field_position_t field_position,
    size_t array_index,
    const data_holder_t& value);

// Set a specific element of a string field of array type.
//
// An exception will be thrown if the index is out of bounds.
//
// This function only works for string fields.
std::vector<uint8_t> set_field_array_element(
    gaia::common::gaia_type_t type_id,
    const uint8_t* serialized_data,
    size_t serialized_data_size,
    const uint8_t* binary_schema,
    size_t binary_schema_size,
    gaia::common::field_position_t field_position,
    size_t array_index,
    const data_holder_t& value);

} // namespace payload_types
} // namespace db
} // namespace gaia
