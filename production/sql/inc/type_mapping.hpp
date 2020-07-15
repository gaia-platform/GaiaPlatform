/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia_common.hpp"

// all definitions in this file and included files should have C linkage
extern "C" {

#include "postgres.h"
#include "flatbuffers_common_reader.h"
#include "flatbuffers_common_builder.h"

// function pointer type that extracts a typed root object from a flatbuffer
// byte array
typedef const void *(*RootObjectDeserializer)(const void *buffer);
// function pointer type that extracts an attribute from a typed flatbuffer
// object
typedef Datum (*AttributeAccessor)(const void *rootObject);
// function pointer type that adds an attribute to a flatbuffer builder
typedef void (*AttributeBuilder)(flatbuffers_builder_t *builder, Datum value);
// function pointer type that initializes a flatbuffer builder for a type
typedef void (*BuilderInitializer)(flatbuffers_builder_t *builder);
// function pointer type that finalizes a flatbuffer builder for a type
typedef void (*BuilderFinalizer)(flatbuffers_builder_t *builder);

// mapping of attribute names to accessor methods
typedef struct {
    const char *name;
    const AttributeAccessor accessor;
    const AttributeBuilder builder;
} Attribute;

// mapping of relations to attribute accessor mappings
// also include initializer/finalizer methods and clone_as_root methods (for
// updates)
typedef struct {
    const char *relation;
    gaia::common::gaia_type_t gaia_type_id;
    RootObjectDeserializer deserializer;
    BuilderInitializer initializer;
    BuilderFinalizer finalizer;
    const Attribute *attributes;
    size_t attribute_count;
} RelationAttributeMapping;

// flatbuffers type helpers
// we need to use this instead of CStringGetTextDatum because it translates null
// pointers into zero-length strings
static Datum flatbuffers_string_to_text_datum(flatbuffers_string_t str) {
    size_t str_len = flatbuffers_string_len(str);
    size_t text_len = str_len + VARHDRSZ;
    text *t = (text *)palloc(text_len);
    SET_VARSIZE(t, text_len);
    memcpy(VARDATA(t), str, str_len);
    return CStringGetDatum(t);
}

} // extern "C"
