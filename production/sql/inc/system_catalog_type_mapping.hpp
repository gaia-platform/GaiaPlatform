/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "array_size.hpp"
#include "type_mapping.hpp"
#include "system_catalog_types.hpp"

// all definitions in this file and included files should have C linkage
extern "C" {

#include "postgres.h"
#include "utils/builtins.h"

#include "flatbuffers_common_reader.h"
// flatcc generated code
#include "event_log_reader.h"
#include "event_log_builder.h"

// type-specific extractors
static inline Datum event_log_get_event_type(const void *rootObject) {
    gaia_rules_event_log_table_t event =
        (gaia_rules_event_log_table_t)rootObject;
    uint32_t event_type = gaia_rules_event_log_event_type(event);
    return UInt32GetDatum(event_type);
}

static inline Datum event_log_get_type_id(const void *rootObject) {
    gaia_rules_event_log_table_t event =
        (gaia_rules_event_log_table_t)rootObject;
    uint64_t type_id = gaia_rules_event_log_type_id(event);
    return UInt64GetDatum(type_id);
}

static inline Datum event_log_get_record_id(const void *rootObject) {
    gaia_rules_event_log_table_t event =
        (gaia_rules_event_log_table_t)rootObject;
    uint64_t record_id = gaia_rules_event_log_record_id(event);
    return UInt64GetDatum(record_id);
}

static inline Datum event_log_get_column_id(const void *rootObject) {
    gaia_rules_event_log_table_t event =
        (gaia_rules_event_log_table_t)rootObject;
    uint16_t column_id = gaia_rules_event_log_column_id(event);
    return UInt16GetDatum(column_id);
}

static inline Datum event_log_get_timestamp(const void *rootObject) {
    gaia_rules_event_log_table_t event =
        (gaia_rules_event_log_table_t)rootObject;
    uint64_t timestamp = gaia_rules_event_log_timestamp(event);
    return UInt64GetDatum(timestamp);
}

static inline Datum event_log_get_rules_invoked(const void *rootObject) {
    gaia_rules_event_log_table_t event =
        (gaia_rules_event_log_table_t)rootObject;
    bool rules_invoked = gaia_rules_event_log_rules_invoked(event);
    return BoolGetDatum(rules_invoked);
}

// type-specific builders
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

static const Attribute EVENT_LOG_ATTRS[] = {
    {"event_type", event_log_get_event_type, event_log_add_event_type},
    {"type_id", event_log_get_type_id, event_log_add_type_id},
    {"record_id", event_log_get_record_id, event_log_add_record_id},
    {"column_id", event_log_get_column_id, event_log_add_column_id},
    {"timestamp", event_log_get_timestamp, event_log_add_timestamp},
    {"rules_invoked", event_log_get_rules_invoked, event_log_add_rules_invoked},
};

RelationAttributeMapping EVENT_LOG_MAPPING = {
    "event_log",
    system_catalog_types::kEventLogType,
    (RootObjectDeserializer)gaia_rules_event_log_as_root,
    (BuilderInitializer)gaia_rules_event_log_start_as_root,
    (BuilderFinalizer)gaia_rules_event_log_end_as_root,
    EVENT_LOG_ATTRS,
    array_size(EVENT_LOG_ATTRS),
};

const char *EVENT_LOG_DDL_STMT_FMT =
    "create foreign table event_log( "
    "event_type int, type_id bigint, record_id bigint, "
    "column_id smallint, timestamp bigint, "
    "rules_invoked boolean) "
    "server %s;";

} // extern "C"
