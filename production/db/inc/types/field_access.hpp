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

void initialize_field_cache_from_binary_schema(
    field_cache_t* field_cache,
    uint8_t* binary_schema);

type_holder_t
get_table_field_value(
    uint64_t type_id,
    uint8_t* serialized_data,
    uint8_t* binary_schema,
    uint16_t field_position)
{
    type_holder_t result;

    // First, we parse the serialized data to get its root object.
    const flatbuffers::Table* root_table = flatbuffers::GetAnyRoot(serialized_data);
    if (root_table == nullptr)
    {
        throw invalid_serialized_data();
    }

    // Get hold of the type cache and lookup the field cache for our type.
    type_cache_t* type_cache = type_cache_t::get_type_cache();
    const field_cache_t* field_cache = type_cache->get_field_cache(type_id);

    // If we got hold of the field cache, we need to release access once we're don using it.
    auto_release_cache_read_access auto_release_cache_access(field_cache != nullptr);

    // If data is not available for our type, we load it locally from the binary schema provided to us.
    field_cache_t local_field_cache;
    if (field_cache == nullptr)
    {
        initialize_field_cache_from_binary_schema(&local_field_cache, binary_schema);
        field_cache = &local_field_cache;
    }

    // Lookup field information in the field cache.
    const reflection::Field* field = field_cache->get_field(field_position);
    if (field == nullptr)
    {
        throw invalid_field_position(field_position);
    }

    // Read field value according to its type.
    result.type = field->type()->base_type();
    if (field->type()->base_type() == reflection::String)
    {
        const flatbuffers::String* field_value = flatbuffers::GetFieldS(*root_table, *field);
        if (field_value == nullptr)
        {
            throw invalid_serialized_data();
        }

        result.string_value = field_value->c_str();
    }
    else if (flatbuffers::IsInteger(field->type()->base_type()))
    {
        result.integer_value = flatbuffers::GetAnyFieldI(*root_table, *field);
    }
    else if (flatbuffers::IsFloat(field->type()->base_type()))
    {
        result.float_value = flatbuffers::GetAnyFieldF(*root_table, *field);
    }
    else
    {
        throw unhandled_field_type(field->type()->base_type());
    }

    return result;
}

}
}
}
