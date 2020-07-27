/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "array_size.hpp"
#include "type_mapping.hpp"
#include "system_catalog_types.hpp"

// All Postgres headers and function declarations must have C linkage.
extern "C" {

#include "postgres.h"
#include "utils/builtins.h"

}  // extern "C"

#include "flatbuffers_common_reader.h"

// flatcc generated code.
#include "event_log_reader.h"
#include "event_log_builder.h"

// Type-specific extractors.
static inline Datum event_log_get_event_type(const void *root_object) {
    gaia_rules_event_log_table_t event =
        (gaia_rules_event_log_table_t)root_object;
    uint32_t event_type = gaia_rules_event_log_event_type(event);
    return UInt32GetDatum(event_type);
}

static inline Datum event_log_get_type_id(const void *root_object) {
    gaia_rules_event_log_table_t event =
        (gaia_rules_event_log_table_t)root_object;
    uint64_t type_id = gaia_rules_event_log_type_id(event);
    return UInt64GetDatum(type_id);
}

static inline Datum event_log_get_record_id(const void *root_object) {
    gaia_rules_event_log_table_t event =
        (gaia_rules_event_log_table_t)root_object;
    uint64_t record_id = gaia_rules_event_log_record_id(event);
    return UInt64GetDatum(record_id);
}

static inline Datum event_log_get_column_id(const void *root_object) {
    gaia_rules_event_log_table_t event =
        (gaia_rules_event_log_table_t)root_object;
    uint16_t column_id = gaia_rules_event_log_column_id(event);
    return UInt16GetDatum(column_id);
}

static inline Datum event_log_get_timestamp(const void *root_object) {
    gaia_rules_event_log_table_t event =
        (gaia_rules_event_log_table_t)root_object;
    uint64_t timestamp = gaia_rules_event_log_timestamp(event);
    return UInt64GetDatum(timestamp);
}

static inline Datum event_log_get_rules_invoked(const void *root_object) {
    gaia_rules_event_log_table_t event =
        (gaia_rules_event_log_table_t)root_object;
    bool rules_invoked = gaia_rules_event_log_rules_invoked(event);
    return BoolGetDatum(rules_invoked);
}

// Type-specific builders.
static inline void event_log_add_event_type(flatbuffers_builder_t *builder,
    Datum value) {
    uint32_t event_type = DatumGetUInt32(value);
    gaia_rules_event_log_event_type_add(builder, event_type);
}

static inline void event_log_add_type_id(flatbuffers_builder_t *builder,
    Datum value) {
    uint64_t type_id = DatumGetUInt64(value);
    gaia_rules_event_log_type_id_add(builder, type_id);
}

static inline void event_log_add_record_id(flatbuffers_builder_t *builder,
    Datum value) {
    uint64_t record_id = DatumGetUInt64(value);
    gaia_rules_event_log_record_id_add(builder, record_id);
}

static inline void event_log_add_column_id(flatbuffers_builder_t *builder,
    Datum value) {
    uint16_t column_id = DatumGetUInt16(value);
    gaia_rules_event_log_column_id_add(builder, column_id);
}

static inline void event_log_add_timestamp(flatbuffers_builder_t *builder,
    Datum value) {
    uint64_t timestamp = DatumGetUInt64(value);
    gaia_rules_event_log_timestamp_add(builder, timestamp);
}

static inline void event_log_add_rules_invoked(flatbuffers_builder_t *builder,
    Datum value) {
    bool rules_invoked = DatumGetBool(value);
    gaia_rules_event_log_rules_invoked_add(builder, rules_invoked);
}

static const attribute_t c_event_log_attributes[] = {
    {"event_type", event_log_get_event_type, event_log_add_event_type},
    {"type_id", event_log_get_type_id, event_log_add_type_id},
    {"record_id", event_log_get_record_id, event_log_add_record_id},
    {"column_id", event_log_get_column_id, event_log_add_column_id},
    {"timestamp", event_log_get_timestamp, event_log_add_timestamp},
    {"rules_invoked", event_log_get_rules_invoked, event_log_add_rules_invoked},
};

relation_attribute_mapping_t c_event_log_mapping = {
    "event_log",
    system_catalog_types::c_event_log_type,
    (root_object_deserializer_fn)gaia_rules_event_log_as_root,
    (builder_initializer_fn)gaia_rules_event_log_start_as_root,
    (builder_finalizer_fn)gaia_rules_event_log_end_as_root,
    c_event_log_attributes,
    array_size(c_event_log_attributes),
};

const char *c_event_log_ddl_stmt_fmt =
    "create foreign table event_log( "
    "event_type int, type_id bigint, record_id bigint, "
    "column_id smallint, timestamp bigint, "
    "rules_invoked boolean) "
    "server %s;";
