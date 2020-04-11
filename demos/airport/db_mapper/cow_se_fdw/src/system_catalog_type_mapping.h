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
#include "flatbuffers_common_reader.h"
// flatcc generated code
#include "event_log_reader.h"

// type-specific extractors
static inline Datum event_log_get_gaia_id(const void *rootObject) {
    gaia_rules_event_log_table_t event = (gaia_rules_event_log_table_t) rootObject;
    int64_t gaia_id = gaia_rules_event_log_gaia_id(event);
    return Int64GetDatum(gaia_id);
}

static inline Datum event_log_get_gaia_type(const void *rootObject) {
    gaia_rules_event_log_table_t event = (gaia_rules_event_log_table_t) rootObject;
    uint64_t gaia_type = gaia_rules_event_log_gaia_type(event);
    return UInt64GetDatum(gaia_type);
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

static inline Datum event_log_get_context_id(const void *rootObject) {
    gaia_rules_event_log_table_t event = (gaia_rules_event_log_table_t) rootObject;
    uint64_t context_id = gaia_rules_event_log_context_id(event);
    return UInt64GetDatum(context_id);
}

static inline Datum event_log_get_rules_fired(const void *rootObject) {
    gaia_rules_event_log_table_t event = (gaia_rules_event_log_table_t) rootObject;
    bool rules_fired = gaia_rules_event_log_rules_fired(event);
    return BoolGetDatum(rules_fired);
}

static const AttributeWithAccessor EVENT_LOG_ATTRS[] = {
    { "gaia_id", event_log_get_gaia_id },
    { "gaia_type", event_log_get_gaia_type },
    { "event_type", event_log_get_event_type },
    { "event_mode", event_log_get_event_mode },
    { "event_source", event_log_get_event_source },
    { "timestamp", event_log_get_timestamp },
    { "context_id", event_log_get_context_id },
    { "rules_fired", event_log_get_rules_fired },
};

RelationAttributeMapping EVENT_LOG_MAPPING = {
    "event_log",
    system_catalog_types::kEventLogType,
    false,
    (RootObjectDeserializer) gaia_rules_event_log_as_root,
    EVENT_LOG_ATTRS,
    ARRAY_SIZE(EVENT_LOG_ATTRS),
};

const char *EVENT_LOG_DDL_STMT_FMT =
    "create foreign table event_log( "
    "gaia_id bigint, "
    "gaia_type bigint, event_type int, event_mode smallint, "
    "event_source text, timestamp bigint, context_id bigint, "
    "rules_fired boolean) "
    "server %s;";

} // extern "C"
