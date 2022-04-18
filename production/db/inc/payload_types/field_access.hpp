/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <cstdint>

#include <flatbuffers/reflection.h>

#include "gaia/common.hpp"
#include "gaia/exception.hpp"

#include "data_holder.hpp"

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
    explicit invalid_field_position(gaia::common::field_position_t position);
};

class unhandled_field_type : public gaia::common::gaia_exception
{
public:
    explicit unhandled_field_type(size_t field_type);
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

// Verify that the serialized data matches the schema.
bool verify_data_schema(
    const uint8_t* serialized_data,
    size_t serialized_data_size,
    const uint8_t* binary_schema);

// Return true if the field values are equal and false if they are different.
//
// This function handles numeric-type fields and numeric-type-array fields.
bool are_field_values_equal(
    const uint8_t* first_serialized_data,
    const uint8_t* second_serialized_data,
    const uint8_t* binary_schema,
    gaia::common::field_position_t field_position);

// Get the value of a field.
//
// This function handles both numeric-type fields and string-type fields.
data_holder_t get_field_value(
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    gaia::common::field_position_t field_position);

// Set the value of a numeric-type field.
bool set_field_value(
    uint8_t* serialized_data,
    const uint8_t* binary_schema,
    gaia::common::field_position_t field_position,
    const data_holder_t& value);

// Set the value of a string-type field.
void set_field_value(
    std::vector<uint8_t>& serialized_data,
    const uint8_t* binary_schema,
    gaia::common::field_position_t field_position,
    const data_holder_t& value);

// Set the value of a string-type field.
std::vector<uint8_t> set_field_value(
    const uint8_t* serialized_data,
    size_t serialized_data_size,
    const uint8_t* binary_schema,
    gaia::common::field_position_t field_position,
    const data_holder_t& value);

// Get the size of an array-type field.
// This will return std::numeric_limits<size_t>::max() if the array field is null.
size_t get_field_array_size(
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    gaia::common::field_position_t field_position);

// Set the size of an array-type field.
//
// If the array is expanded, new entries will be set to 0.
void set_field_array_size(
    std::vector<uint8_t>& serialized_data,
    const uint8_t* binary_schema,
    gaia::common::field_position_t field_position,
    size_t new_size);

// Set the size of an array-type field.
//
// If the array is expanded, new entries will be set to 0.
std::vector<uint8_t> set_field_array_size(
    const uint8_t* serialized_data,
    size_t serialized_data_size,
    const uint8_t* binary_schema,
    gaia::common::field_position_t field_position,
    size_t new_size);

// Get a specific element of an array-type field.
//
// This function handles both numeric-type elements and string-type elements.
//
// An exception will be thrown if the index is out of bounds.
data_holder_t get_field_array_element(
    const uint8_t* serialized_data,
    const uint8_t* binary_schema,
    gaia::common::field_position_t field_position,
    size_t array_index);

// Set a specific numeric-type element of an array-type field .
//
// An exception will be thrown if the index is out of bounds.
void set_field_array_element(
    uint8_t* serialized_data,
    const uint8_t* binary_schema,
    gaia::common::field_position_t field_position,
    size_t array_index,
    const data_holder_t& value);

// Set a specific string-type element of an array-type field.
//
// An exception will be thrown if the index is out of bounds.
void set_field_array_element(
    std::vector<uint8_t>& serialized_data,
    const uint8_t* binary_schema,
    gaia::common::field_position_t field_position,
    size_t array_index,
    const data_holder_t& value);

// Set a specific string-type element of an array-type field.
//
// An exception will be thrown if the index is out of bounds.
std::vector<uint8_t> set_field_array_element(
    const uint8_t* serialized_data,
    size_t serialized_data_size,
    const uint8_t* binary_schema,
    gaia::common::field_position_t field_position,
    size_t array_index,
    const data_holder_t& value);

} // namespace payload_types
} // namespace db
} // namespace gaia
