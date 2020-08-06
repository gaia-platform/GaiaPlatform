/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia_common.hpp"
#include "gaia_db.hpp"
#include "gaia_ptr.hpp"

// All Postgres headers and function declarations must have C linkage.
extern "C"
{

#include "postgres.h"

} // extern "C"

#include "flatbuffers_common_reader.h"
#include "flatbuffers_common_builder.h"

// Function pointer type that extracts a typed root object from a flatbuffer
// byte array.
typedef const void* (*root_object_deserializer_fn)(const void* buffer);

// Function pointer type that extracts an attribute from a typed flatbuffer
// object.
typedef Datum (*attribute_accessor_fn)(const void* root_object);

// Function pointer type that adds an attribute to a flatbuffer builder.
typedef void (*attribute_builder_fn)(flatbuffers_builder_t* builder, Datum value);

// Function pointer type that initializes a flatbuffer builder for a type.
typedef void (*builder_initializer_fn)(flatbuffers_builder_t* builder);

// Function pointer type that finalizes a flatbuffer builder for a type.
typedef void (*builder_finalizer_fn)(flatbuffers_builder_t* builder);

// Mapping of attribute names to accessor methods.
struct attribute_t
{
    const char* name;
    const attribute_accessor_fn accessor;
    const attribute_builder_fn builder;
};

// Mapping of relations to attribute accessor mappings
// also include initializer/finalizer methods and clone_as_root methods (for
// updates).
struct relation_attribute_mapping_t
{
    const char* relation;

    gaia::common::gaia_type_t gaia_type_id;

    root_object_deserializer_fn deserializer;

    builder_initializer_fn initializer;
    builder_finalizer_fn finalizer;

    const attribute_t* attributes;
    size_t attribute_count;
};

// flatbuffers type helpers.
// We need to use this instead of CStringGetTextDatum because it translates null
// pointers into zero-length strings.
Datum flatbuffers_string_to_text_datum(flatbuffers_string_t str);
