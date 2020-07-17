/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia_common.hpp"

// All definitions in this file and included files should have C linkage.
extern "C" {

#include "postgres.h"
#include "flatbuffers_common_reader.h"
#include "flatbuffers_common_builder.h"

// Function pointer type that extracts a typed root object from a flatbuffer
// byte array.
typedef const void *(*root_object_deserializer_fn)(const void *buffer);

// Function pointer type that extracts an attribute from a typed flatbuffer
// object.
typedef Datum (*attribute_accessor_fn)(const void *rootObject);

// Function pointer type that adds an attribute to a flatbuffer builder.
typedef void (*attribute_builder_fn)(flatbuffers_builder_t *builder, Datum value);

// Function pointer type that initializes a flatbuffer builder for a type.
typedef void (*builder_initializer_fn)(flatbuffers_builder_t *builder);

// Function pointer type that finalizes a flatbuffer builder for a type.
typedef void (*builder_finalizer_fn)(flatbuffers_builder_t *builder);

// Mapping of attribute names to accessor methods.
typedef struct {
    const char *name;
    const attribute_accessor_fn accessor;
    const attribute_builder_fn builder;
} attribute_t;

// Mapping of relations to attribute accessor mappings
// also include initializer/finalizer methods and clone_as_root methods (for
// updates).
typedef struct {
    const char *relation;
    gaia::common::gaia_type_t gaia_type_id;
    root_object_deserializer_fn deserializer;
    builder_initializer_fn initializer;
    builder_finalizer_fn finalizer;
    const attribute_t *attributes;
    size_t attribute_count;
} relation_attribute_mapping_t;

// flatbuffers type helpers.
// We need to use this instead of CStringGetTextDatum because it translates null
// pointers into zero-length strings.
static Datum flatbuffers_string_to_text_datum(flatbuffers_string_t str) {
    size_t str_len = flatbuffers_string_len(str);
    size_t text_len = str_len + VARHDRSZ;
    text *t = (text *)palloc(text_len);
    SET_VARSIZE(t, text_len);
    memcpy(VARDATA(t), str, str_len);
    return CStringGetDatum(t);
}

}  // extern "C"
