/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "type_mapping.h"
#include "system_catalog_types.h"
#include "helpers.h"

// all definitions in this file and included files should have C linkage
extern "C" {

#include "postgres.h"
#include "utils/builtins.h"

#include "flatbuffers_common_reader.h"
// flatcc generated code
#include "event_log_reader.h"
#include "event_log_builder.h"

// type-specific extractors
static inline Datum event_log_get_gaia_type_id(const void *rootObject) {
    gaia_rules_event_log_table_t event = (gaia_rules_event_log_table_t) rootObject;
    uint64_t gaia_type_id = gaia_rules_event_log_gaia_type_id(event);
    return UInt64GetDatum(gaia_type_id);
}

static inline Datum event_log_get_event_type(const void *rootObject) {
    gaia_rules_event_log_table_t event = (gaia_rules_event_log_table_t) rootObject;
    uint32_t event_type = gaia_rules_event_log_event_type(event);
    return UInt32GetDatum(event_type);
}

static inline Datum event_log_get_event_mode(const void *rootObject) {
    gaia_rules_event_log_table_t event = (gaia_rules_event_log_table_t) rootObject;
    uint8_t event_mode = gaia_rules_event_log_event_mode(event);
    return UInt8GetDatum(event_mode);
}

static inline Datum event_log_get_event_source(const void *rootObject) {
    gaia_rules_event_log_table_t event = (gaia_rules_event_log_table_t) rootObject;
    flatbuffers_string_t event_source = gaia_rules_event_log_event_source(event);
    return flatbuffers_string_to_text_datum(event_source);
}

static inline Datum event_log_get_timestamp(const void *rootObject) {
    gaia_rules_event_log_table_t event = (gaia_rules_event_log_table_t) rootObject;
    uint64_t timestamp = gaia_rules_event_log_timestamp(event);
    return UInt64GetDatum(timestamp);
}

static inline Datum event_log_get_gaia_id(const void *rootObject) {
    gaia_rules_event_log_table_t event = (gaia_rules_event_log_table_t) rootObject;
    uint64_t gaia_id = gaia_rules_event_log_gaia_id(event);
    return UInt64GetDatum(gaia_id);
}

static inline Datum event_log_get_rules_fired(const void *rootObject) {
    gaia_rules_event_log_table_t event = (gaia_rules_event_log_table_t) rootObject;
    bool rules_fired = gaia_rules_event_log_rules_fired(event);
    return BoolGetDatum(rules_fired);
}

// type-specific builders
static inline void event_log_add_gaia_type_id(flatbuffers_builder_t* builder, Datum value) {
    uint64_t gaia_type_id = DatumGetUInt64(value);
    gaia_rules_event_log_gaia_type_id_add(builder, gaia_type_id);
}

static inline void event_log_add_event_type(flatbuffers_builder_t* builder, Datum value) {
    uint32_t event_type = DatumGetUInt32(value);
    gaia_rules_event_log_event_type_add(builder, event_type);
}

static inline void event_log_add_event_mode(flatbuffers_builder_t* builder, Datum value) {
    uint8_t event_mode = DatumGetUInt8(value);
    gaia_rules_event_log_event_mode_add(builder, event_mode);
}

static inline void event_log_add_event_source(flatbuffers_builder_t* builder, Datum value) {
    const char *event_source = TextDatumGetCString(value);
    gaia_rules_event_log_event_source_create_str(builder, event_source);
}

static inline void event_log_add_timestamp(flatbuffers_builder_t* builder, Datum value) {
    uint64_t timestamp = DatumGetUInt64(value);
    gaia_rules_event_log_timestamp_add(builder, timestamp);
}

static inline void event_log_add_gaia_id(flatbuffers_builder_t* builder, Datum value) {
    uint64_t gaia_id = DatumGetUInt64(value);
    gaia_rules_event_log_gaia_id_add(builder, gaia_id);
}

static inline void event_log_add_rules_fired(flatbuffers_builder_t* builder, Datum value) {
    bool rules_fired = DatumGetBool(value);
    gaia_rules_event_log_rules_fired_add(builder, rules_fired);
}

static const Attribute EVENT_LOG_ATTRS[] = {
    { "gaia_type_id", event_log_get_gaia_type_id, event_log_add_gaia_type_id },
    { "event_type", event_log_get_event_type, event_log_add_event_type },
    { "event_mode", event_log_get_event_mode, event_log_add_event_mode },
    { "event_source", event_log_get_event_source, event_log_add_event_source },
    { "timestamp", event_log_get_timestamp, event_log_add_timestamp },
    { "gaia_id", event_log_get_gaia_id, event_log_add_gaia_id },
    { "rules_fired", event_log_get_rules_fired, event_log_add_rules_fired },
};

RelationAttributeMapping EVENT_LOG_MAPPING = {
    "event_log",
    system_catalog_types::kEventLogType,
    false,
    (RootObjectDeserializer) gaia_rules_event_log_as_root,
    (BuilderInitializer) gaia_rules_event_log_start_as_root,
    (BuilderFinalizer) gaia_rules_event_log_end_as_root,
    EVENT_LOG_ATTRS,
    ARRAY_SIZE(EVENT_LOG_ATTRS),
};

const char *EVENT_LOG_DDL_STMT_FMT =
    "create foreign table event_log( "
    "gaia_type_id bigint, event_type int, event_mode smallint, "
    "event_source text, timestamp bigint, gaia_id bigint, "
    "rules_fired boolean) "
    "server %s;";

} // extern "C"
