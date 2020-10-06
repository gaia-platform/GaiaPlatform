/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "system_catalog_type_mapping.hpp"

#include "system_table_types.hpp"

static const attribute_t c_event_log_attributes[] = {
    {"event_type", event_log_get_event_type, event_log_add_event_type},
    {"type_id", event_log_get_type_id, event_log_add_type_id},
    {"record_id", event_log_get_record_id, event_log_add_record_id},
    {"column_id", event_log_get_column_id, event_log_add_column_id},
    {"timestamp", event_log_get_timestamp, event_log_add_timestamp},
    {"rules_invoked", event_log_get_rules_invoked, event_log_add_rules_invoked},
};

const relation_attribute_mapping_t c_event_log_mapping = {
    "event_log",
    static_cast<gaia_type_t>(system_table_type_t::event_log),
    (root_object_deserializer_fn)gaia_rules_event_log_as_root,
    (builder_initializer_fn)gaia_rules_event_log_start_as_root,
    (builder_finalizer_fn)gaia_rules_event_log_end_as_root,
    c_event_log_attributes,
    std::size(c_event_log_attributes),
};

const char *c_event_log_ddl_stmt_fmt =
    "create foreign table event_log( "
    "event_type int, type_id bigint, record_id bigint, "
    "column_id smallint, timestamp bigint, "
    "rules_invoked boolean) "
    "server %s;";
