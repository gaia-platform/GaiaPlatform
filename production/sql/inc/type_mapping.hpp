/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <functional>

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
typedef std::function<const void*(const void*)> root_object_deserializer_fn;

// Function pointer type that extracts an attribute from a typed flatbuffer
// object.
typedef std::function<Datum(const void*)> attribute_accessor_fn;

// Function pointer type that adds an attribute to a flatbuffer builder.
typedef std::function<void(flatbuffers_builder_t*, Datum)> attribute_builder_fn;

// Function pointer type that initializes a flatbuffer builder for a type.
typedef std::function<void(flatbuffers_builder_t*)> builder_initializer_fn;

// Function pointer type that finalizes a flatbuffer builder for a type.
typedef std::function<void(flatbuffers_builder_t*)> builder_finalizer_fn;

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

    gaia::common::gaia_type_t gaia_container_id;

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
