/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "gaia_common.hpp"
#include "gaia_db.hpp"
#include "gaia_ptr.hpp"

// All definitions in this file and included files should have C linkage.
extern "C" {

#include "postgres.h"

}  // extern "C"

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

typedef void (*option_handler_fn)(const char *name, const char *value, Oid context);

// Describes the valid options for objects that use this wrapper.
typedef struct {
    const char *name;

    // Oid of catalog in which option may appear.
    Oid context;

    option_handler_fn handler;
} gaia_fdw_option_t;

// The scan state is set up in gaia_begin_foreign_scan and stashed away in
// node->fdw_private and fetched in gaia_iterate_foreign_scan.
typedef struct {
    root_object_deserializer_fn deserializer;

    // flatbuffer accessor functions indexed by attrnum.
    attribute_accessor_fn *indexed_accessors;

    // The COW-SE smart ptr we are currently iterating over.
    gaia::db::gaia_ptr current_node;
} gaia_fdw_scan_state_t;

// The modify state is for maintaining state of modify operations.
//
// It is set up in gaiaBeginForeignModify and stashed in
// rinfo->ri_FdwState and subsequently used in gaiaExecForeignInsert,
// gaiaExecForeignUpdate, gaiaExecForeignDelete and
// gaiaEndForeignModify.
typedef struct {
    builder_initializer_fn initializer;
    builder_finalizer_fn finalizer;

    // flatbuffer attribute builder functions indexed by attrnum.
    attribute_builder_fn *indexed_builders;

    // flatbuffers builder for INSERT and UPDATE.
    flatcc_builder_t builder;

    // 0-based index of gaia_id attribute in tuple descriptor.
    int pk_attr_idx;

    gaia_type_t gaia_type_id;

    // The COW-SE smart ptr that is the target of our update.
    gaia::db::gaia_ptr target_node;
} gaia_fdw_modify_state_t;

// Valid options for gaia_fdw.
extern const gaia_fdw_option_t valid_options[];

// flatbuffers type helpers.
// We need to use this instead of CStringGetTextDatum because it translates null
// pointers into zero-length strings.
Datum flatbuffers_string_to_text_datum(flatbuffers_string_t str);

// Check if the provided option is one of the valid options.
// context is the Oid of the catalog holding the object the option is for.
// If handler is registered for this option, invoke it.
bool is_valid_option(const char *option, const char *value, Oid context);
