/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Automatically generated by the Gaia Data Classes code generator.
// Do not modify.

#include <iterator>

#ifndef GAIA_GENERATED_catalog_H_
#define GAIA_GENERATED_catalog_H_

#include "gaia_object.hpp"
#include "catalog_generated.h"
#include "gaia_iterators.hpp"

using namespace std;
using namespace gaia::direct_access;

namespace gaia {
namespace catalog {

// The initial size of the flatbuffer builder buffer.
constexpr int c_flatbuffer_builder_size = 128;

// Constants contained in the gaia_rule object.
constexpr int c_parent_rules_gaia_ruleset = 0;
constexpr int c_next_rules_gaia_rule = 1;
constexpr int c_num_gaia_rule_ptrs = 2;

// Constants contained in the gaia_ruleset object.
constexpr int c_first_rules_gaia_rule = 0;
constexpr int c_num_gaia_ruleset_ptrs = 1;

// Constants contained in the gaia_field object.
constexpr int c_parent_fields_gaia_table = 0;
constexpr int c_next_fields_gaia_field = 1;
constexpr int c_parent_refs_gaia_table = 2;
constexpr int c_next_refs_gaia_field = 3;
constexpr int c_num_gaia_field_ptrs = 4;

// Constants contained in the gaia_table object.
constexpr int c_first_fields_gaia_field = 0;
constexpr int c_first_refs_gaia_field = 1;
constexpr int c_parent_gaia_table_gaia_database = 2;
constexpr int c_next_gaia_table_gaia_table = 3;
constexpr int c_num_gaia_table_ptrs = 4;

// Constants contained in the gaia_database object.
constexpr int c_first_gaia_table_gaia_table = 0;
constexpr int c_num_gaia_database_ptrs = 1;

struct gaia_rule_t;
struct gaia_ruleset_t;
struct gaia_field_t;
struct gaia_table_t;
struct gaia_database_t;

typedef gaia_writer_t<18446744073709551611llu,gaia_database_t,gaia_database,gaia_databaseT,c_num_gaia_database_ptrs> gaia_database_writer;
struct gaia_database_t : public gaia_object_t<18446744073709551611llu,gaia_database_t,gaia_database,gaia_databaseT,c_num_gaia_database_ptrs> {
    gaia_database_t() : gaia_object_t("gaia_database_t") {}
    const char* name() const {return GET_STR(name);}
    using gaia_object_t::insert_row;
    static gaia_id_t insert_row(const char* name) {
        flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
        b.Finish(Creategaia_databaseDirect(b, name));
        return gaia_object_t::insert_row(b);
    }
    static gaia_container_t<18446744073709551611llu, gaia_database_t>& list() {
        static gaia_container_t<18446744073709551611llu, gaia_database_t> list;
        return list;
    }
    reference_chain_container_t<gaia_database_t,gaia_table_t,c_parent_gaia_table_gaia_database,c_first_gaia_table_gaia_table,c_next_gaia_table_gaia_table> m_gaia_table_list;
    reference_chain_container_t<gaia_database_t,gaia_table_t,c_parent_gaia_table_gaia_database,c_first_gaia_table_gaia_table,c_next_gaia_table_gaia_table>& gaia_table_list() {
        return m_gaia_table_list;
    }
private:
    friend struct gaia_object_t<18446744073709551611llu, gaia_database_t, gaia_database, gaia_databaseT, c_num_gaia_database_ptrs>;
    gaia_database_t(gaia_id_t id) : gaia_object_t(id, "gaia_database_t") {
        m_gaia_table_list.set_outer(gaia_id());
    }
};

typedef gaia_writer_t<18446744073709551612llu,gaia_ruleset_t,gaia_ruleset,gaia_rulesetT,c_num_gaia_ruleset_ptrs> gaia_ruleset_writer;
struct gaia_ruleset_t : public gaia_object_t<18446744073709551612llu,gaia_ruleset_t,gaia_ruleset,gaia_rulesetT,c_num_gaia_ruleset_ptrs> {
    gaia_ruleset_t() : gaia_object_t("gaia_ruleset_t") {}
    const char* name() const {return GET_STR(name);}
    bool active_on_startup() const {return GET(active_on_startup);}
    const char* table_ids() const {return GET_STR(table_ids);}
    const char* source_location() const {return GET_STR(source_location);}
    const char* serial_stream() const {return GET_STR(serial_stream);}
    using gaia_object_t::insert_row;
    static gaia_id_t insert_row(const char* name, bool active_on_startup, const char* table_ids, const char* source_location, const char* serial_stream) {
        flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
        b.Finish(Creategaia_rulesetDirect(b, name, active_on_startup, table_ids, source_location, serial_stream));
        return gaia_object_t::insert_row(b);
    }
    static gaia_container_t<18446744073709551612llu, gaia_ruleset_t>& list() {
        static gaia_container_t<18446744073709551612llu, gaia_ruleset_t> list;
        return list;
    }
    reference_chain_container_t<gaia_ruleset_t,gaia_rule_t,c_parent_rules_gaia_ruleset,c_first_rules_gaia_rule,c_next_rules_gaia_rule> m_rules_gaia_rule_list;
    reference_chain_container_t<gaia_ruleset_t,gaia_rule_t,c_parent_rules_gaia_ruleset,c_first_rules_gaia_rule,c_next_rules_gaia_rule>& rules_gaia_rule_list() {
        return m_rules_gaia_rule_list;
    }
private:
    friend struct gaia_object_t<18446744073709551612llu, gaia_ruleset_t, gaia_ruleset, gaia_rulesetT, c_num_gaia_ruleset_ptrs>;
    gaia_ruleset_t(gaia_id_t id) : gaia_object_t(id, "gaia_ruleset_t") {
        m_rules_gaia_rule_list.set_outer(gaia_id());
    }
};

typedef gaia_writer_t<18446744073709551613llu,gaia_rule_t,gaia_rule,gaia_ruleT,c_num_gaia_rule_ptrs> gaia_rule_writer;
struct gaia_rule_t : public gaia_object_t<18446744073709551613llu,gaia_rule_t,gaia_rule,gaia_ruleT,c_num_gaia_rule_ptrs> {
    gaia_rule_t() : gaia_object_t("gaia_rule_t") {}
    const char* name() const {return GET_STR(name);}
    uint64_t ruleset_id() const {return GET(ruleset_id);}
    using gaia_object_t::insert_row;
    static gaia_id_t insert_row(const char* name, uint64_t ruleset_id) {
        flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
        b.Finish(Creategaia_ruleDirect(b, name, ruleset_id));
        return gaia_object_t::insert_row(b);
    }
    gaia_ruleset_t rules_gaia_ruleset() {
        return gaia_ruleset_t::get(this->references()[c_parent_rules_gaia_ruleset]);
    }
    static gaia_container_t<18446744073709551613llu, gaia_rule_t>& list() {
        static gaia_container_t<18446744073709551613llu, gaia_rule_t> list;
        return list;
    }
private:
    friend struct gaia_object_t<18446744073709551613llu, gaia_rule_t, gaia_rule, gaia_ruleT, c_num_gaia_rule_ptrs>;
    gaia_rule_t(gaia_id_t id) : gaia_object_t(id, "gaia_rule_t") {
    }
};

typedef gaia_writer_t<18446744073709551614llu,gaia_table_t,gaia_table,gaia_tableT,c_num_gaia_table_ptrs> gaia_table_writer;
struct gaia_table_t : public gaia_object_t<18446744073709551614llu,gaia_table_t,gaia_table,gaia_tableT,c_num_gaia_table_ptrs> {
    gaia_table_t() : gaia_object_t("gaia_table_t") {}
    const char* name() const {return GET_STR(name);}
    bool is_log() const {return GET(is_log);}
    uint8_t trim_action() const {return GET(trim_action);}
    uint64_t max_rows() const {return GET(max_rows);}
    uint64_t max_size() const {return GET(max_size);}
    uint64_t max_seconds() const {return GET(max_seconds);}
    const char* binary_schema() const {return GET_STR(binary_schema);}
    using gaia_object_t::insert_row;
    static gaia_id_t insert_row(const char* name, bool is_log, uint8_t trim_action, uint64_t max_rows, uint64_t max_size, uint64_t max_seconds, const char* binary_schema) {
        flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
        b.Finish(Creategaia_tableDirect(b, name, is_log, trim_action, max_rows, max_size, max_seconds, binary_schema));
        return gaia_object_t::insert_row(b);
    }
    gaia_database_t gaia_database() {
        return gaia_database_t::get(this->references()[c_parent_gaia_table_gaia_database]);
    }
    static gaia_container_t<18446744073709551614llu, gaia_table_t>& list() {
        static gaia_container_t<18446744073709551614llu, gaia_table_t> list;
        return list;
    }
    reference_chain_container_t<gaia_table_t,gaia_field_t,c_parent_fields_gaia_table,c_first_fields_gaia_field,c_next_fields_gaia_field> m_fields_gaia_field_list;
    reference_chain_container_t<gaia_table_t,gaia_field_t,c_parent_fields_gaia_table,c_first_fields_gaia_field,c_next_fields_gaia_field>& fields_gaia_field_list() {
        return m_fields_gaia_field_list;
    }
    reference_chain_container_t<gaia_table_t,gaia_field_t,c_parent_refs_gaia_table,c_first_refs_gaia_field,c_next_refs_gaia_field> m_refs_gaia_field_list;
    reference_chain_container_t<gaia_table_t,gaia_field_t,c_parent_refs_gaia_table,c_first_refs_gaia_field,c_next_refs_gaia_field>& refs_gaia_field_list() {
        return m_refs_gaia_field_list;
    }
private:
    friend struct gaia_object_t<18446744073709551614llu, gaia_table_t, gaia_table, gaia_tableT, c_num_gaia_table_ptrs>;
    gaia_table_t(gaia_id_t id) : gaia_object_t(id, "gaia_table_t") {
        m_fields_gaia_field_list.set_outer(gaia_id());
        m_refs_gaia_field_list.set_outer(gaia_id());
    }
};

typedef gaia_writer_t<18446744073709551615llu,gaia_field_t,gaia_field,gaia_fieldT,c_num_gaia_field_ptrs> gaia_field_writer;
struct gaia_field_t : public gaia_object_t<18446744073709551615llu,gaia_field_t,gaia_field,gaia_fieldT,c_num_gaia_field_ptrs> {
    gaia_field_t() : gaia_object_t("gaia_field_t") {}
    const char* name() const {return GET_STR(name);}
    uint64_t table_id() const {return GET(table_id);}
    uint8_t type() const {return GET(type);}
    uint64_t type_id() const {return GET(type_id);}
    uint16_t repeated_count() const {return GET(repeated_count);}
    uint16_t position() const {return GET(position);}
    bool required() const {return GET(required);}
    bool deprecated() const {return GET(deprecated);}
    bool active() const {return GET(active);}
    bool nullable() const {return GET(nullable);}
    bool has_default() const {return GET(has_default);}
    const char* default_value() const {return GET_STR(default_value);}
    using gaia_object_t::insert_row;
    static gaia_id_t insert_row(const char* name, uint64_t table_id, uint8_t type, uint64_t type_id, uint16_t repeated_count, uint16_t position, bool required, bool deprecated, bool active, bool nullable, bool has_default, const char* default_value) {
        flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
        b.Finish(Creategaia_fieldDirect(b, name, table_id, type, type_id, repeated_count, position, required, deprecated, active, nullable, has_default, default_value));
        return gaia_object_t::insert_row(b);
    }
    gaia_table_t fields_gaia_table() {
        return gaia_table_t::get(this->references()[c_parent_fields_gaia_table]);
    }
    gaia_table_t refs_gaia_table() {
        return gaia_table_t::get(this->references()[c_parent_refs_gaia_table]);
    }
    static gaia_container_t<18446744073709551615llu, gaia_field_t>& list() {
        static gaia_container_t<18446744073709551615llu, gaia_field_t> list;
        return list;
    }
private:
    friend struct gaia_object_t<18446744073709551615llu, gaia_field_t, gaia_field, gaia_fieldT, c_num_gaia_field_ptrs>;
    gaia_field_t(gaia_id_t id) : gaia_object_t(id, "gaia_field_t") {
    }
};

}  // namespace catalog
}  // namespace gaia

#endif  // GAIA_GENERATED_catalog_H_

