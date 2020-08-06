/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "type_mapping.hpp"

// All Postgres headers and function declarations must have C linkage.
extern "C" {

#include "postgres.h"
#include "utils/builtins.h"

} // extern "C"

// flatcc generated code.
#include "event_log_reader.h"
#include "event_log_builder.h"

// Type-specific extractors.
inline Datum event_log_get_event_type(const void *root_object) {
    gaia_rules_event_log_table_t event =
        (gaia_rules_event_log_table_t)root_object;
    uint32_t event_type = gaia_rules_event_log_event_type(event);
    return UInt32GetDatum(event_type);
}

inline Datum event_log_get_type_id(const void *root_object) {
    gaia_rules_event_log_table_t event =
        (gaia_rules_event_log_table_t)root_object;
    uint64_t type_id = gaia_rules_event_log_type_id(event);
    return UInt64GetDatum(type_id);
}

inline Datum event_log_get_record_id(const void *root_object) {
    gaia_rules_event_log_table_t event =
        (gaia_rules_event_log_table_t)root_object;
    uint64_t record_id = gaia_rules_event_log_record_id(event);
    return UInt64GetDatum(record_id);
}

inline Datum event_log_get_column_id(const void *root_object) {
    gaia_rules_event_log_table_t event =
        (gaia_rules_event_log_table_t)root_object;
    uint16_t column_id = gaia_rules_event_log_column_id(event);
    return UInt16GetDatum(column_id);
}

inline Datum event_log_get_timestamp(const void *root_object) {
    gaia_rules_event_log_table_t event =
        (gaia_rules_event_log_table_t)root_object;
    uint64_t timestamp = gaia_rules_event_log_timestamp(event);
    return UInt64GetDatum(timestamp);
}

inline Datum event_log_get_rules_invoked(const void *root_object) {
    gaia_rules_event_log_table_t event =
        (gaia_rules_event_log_table_t)root_object;
    bool rules_invoked = gaia_rules_event_log_rules_invoked(event);
    return BoolGetDatum(rules_invoked);
}

// Type-specific builders.
inline void event_log_add_event_type(flatbuffers_builder_t *builder,
    Datum value) {
    uint32_t event_type = DatumGetUInt32(value);
    gaia_rules_event_log_event_type_add(builder, event_type);
}

inline void event_log_add_type_id(flatbuffers_builder_t *builder,
    Datum value) {
    uint64_t type_id = DatumGetUInt64(value);
    gaia_rules_event_log_type_id_add(builder, type_id);
}

inline void event_log_add_record_id(flatbuffers_builder_t *builder,
    Datum value) {
    uint64_t record_id = DatumGetUInt64(value);
    gaia_rules_event_log_record_id_add(builder, record_id);
}

inline void event_log_add_column_id(flatbuffers_builder_t *builder,
    Datum value) {
    uint16_t column_id = DatumGetUInt16(value);
    gaia_rules_event_log_column_id_add(builder, column_id);
}

inline void event_log_add_timestamp(flatbuffers_builder_t *builder,
    Datum value) {
    uint64_t timestamp = DatumGetUInt64(value);
    gaia_rules_event_log_timestamp_add(builder, timestamp);
}

inline void event_log_add_rules_invoked(flatbuffers_builder_t *builder,
    Datum value) {
    bool rules_invoked = DatumGetBool(value);
    gaia_rules_event_log_rules_invoked_add(builder, rules_invoked);
}

extern const relation_attribute_mapping_t c_event_log_mapping;

extern const char *c_event_log_ddl_stmt_fmt;
