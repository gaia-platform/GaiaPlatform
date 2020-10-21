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
constexpr uint32_t c_gaia_type_gaia_rule = 4294967293u;
constexpr int c_parent_gaia_rule_gaia_ruleset = 0;
constexpr int c_next_gaia_rule_gaia_rule = 1;
constexpr int c_num_gaia_rule_ptrs = 2;

// Constants contained in the gaia_ruleset object.
constexpr uint32_t c_gaia_type_gaia_ruleset = 4294967292u;
constexpr int c_first_gaia_rule_gaia_rule = 0;
constexpr int c_num_gaia_ruleset_ptrs = 1;

// Constants contained in the gaia_field object.
constexpr uint32_t c_gaia_type_gaia_field = 4294967295u;
constexpr int c_parent_gaia_field_gaia_table = 0;
constexpr int c_next_gaia_field_gaia_field = 1;
constexpr int c_parent_ref_gaia_table = 2;
constexpr int c_next_ref_gaia_field = 3;
constexpr int c_num_gaia_field_ptrs = 4;

// Constants contained in the gaia_table object.
constexpr uint32_t c_gaia_type_gaia_table = 4294967294u;
constexpr int c_first_gaia_field_gaia_field = 0;
constexpr int c_first_ref_gaia_field = 1;
constexpr int c_parent_gaia_table_gaia_database = 2;
constexpr int c_next_gaia_table_gaia_table = 3;
constexpr int c_num_gaia_table_ptrs = 4;

// Constants contained in the gaia_database object.
constexpr uint32_t c_gaia_type_gaia_database = 4294967291u;
constexpr int c_first_gaia_table_gaia_table = 0;
constexpr int c_num_gaia_database_ptrs = 1;

struct gaia_rule_t;
struct gaia_ruleset_t;
struct gaia_field_t;
struct gaia_table_t;
struct gaia_database_t;

typedef gaia_writer_t<c_gaia_type_gaia_database, gaia_database_t, gaia_database, gaia_databaseT, c_num_gaia_database_ptrs> gaia_database_writer;
struct gaia_database_t : public gaia_object_t<c_gaia_type_gaia_database, gaia_database_t, gaia_database, gaia_databaseT, c_num_gaia_database_ptrs> {
    gaia_database_t() : gaia_object_t("gaia_database_t") {}
    const char* name() const {return GET_STR(name);}
    using gaia_object_t::insert_row;
    static gaia_id_t insert_row(const char* name) {
        flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
        b.Finish(Creategaia_databaseDirect(b, name));
        return gaia_object_t::insert_row(b);
    }
    static gaia_container_t<c_gaia_type_gaia_database, gaia_database_t>& list() {
        static gaia_container_t<c_gaia_type_gaia_database, gaia_database_t> list;
        return list;
    }
    reference_chain_container_t<gaia_database_t, gaia_table_t, c_parent_gaia_table_gaia_database, c_first_gaia_table_gaia_table, c_next_gaia_table_gaia_table> m_gaia_table_list;
    reference_chain_container_t<gaia_database_t, gaia_table_t, c_parent_gaia_table_gaia_database, c_first_gaia_table_gaia_table, c_next_gaia_table_gaia_table>& gaia_table_list() {
        return m_gaia_table_list;
    }
private:
    friend struct gaia_object_t<c_gaia_type_gaia_database, gaia_database_t, gaia_database, gaia_databaseT, c_num_gaia_database_ptrs>;
    gaia_database_t(gaia_id_t id) : gaia_object_t(id, "gaia_database_t") {
        m_gaia_table_list.set_outer(gaia_id());
    }
};

typedef gaia_writer_t<c_gaia_type_gaia_table, gaia_table_t, gaia_table, gaia_tableT, c_num_gaia_table_ptrs> gaia_table_writer;
struct gaia_table_t : public gaia_object_t<c_gaia_type_gaia_table, gaia_table_t, gaia_table, gaia_tableT, c_num_gaia_table_ptrs> {
    gaia_table_t() : gaia_object_t("gaia_table_t") {}
    const char* name() const {return GET_STR(name);}
    uint32_t type() const {return GET(type);}
    bool is_system() const {return GET(is_system);}
    const char* binary_schema() const {return GET_STR(binary_schema);}
    const char* serialization_template() const {return GET_STR(serialization_template);}
    using gaia_object_t::insert_row;
    static gaia_id_t insert_row(const char* name, const char* binary_schema, const char* serialization_template) {
        flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
        b.Finish(Creategaia_tableDirect(b, name, binary_schema, serialization_template));
        return gaia_object_t::insert_row(b);
    }
    gaia_database_t gaia_database() {
        return gaia_database_t::get(this->references()[c_parent_gaia_table_gaia_database]);
    }
    static gaia_container_t<c_gaia_type_gaia_table, gaia_table_t>& list() {
        static gaia_container_t<c_gaia_type_gaia_table, gaia_table_t> list;
        return list;
    }
    reference_chain_container_t<gaia_table_t, gaia_field_t, c_parent_gaia_field_gaia_table, c_first_gaia_field_gaia_field, c_next_gaia_field_gaia_field> m_gaia_field_list;
    reference_chain_container_t<gaia_table_t, gaia_field_t, c_parent_gaia_field_gaia_table, c_first_gaia_field_gaia_field, c_next_gaia_field_gaia_field>& gaia_field_list() {
        return m_gaia_field_list;
    }
    reference_chain_container_t<gaia_table_t, gaia_field_t, c_parent_ref_gaia_table, c_first_ref_gaia_field, c_next_ref_gaia_field> m_ref_gaia_field_list;
    reference_chain_container_t<gaia_table_t, gaia_field_t, c_parent_ref_gaia_table, c_first_ref_gaia_field, c_next_ref_gaia_field>& ref_gaia_field_list() {
        return m_ref_gaia_field_list;
    }
private:
    friend struct gaia_object_t<c_gaia_type_gaia_table, gaia_table_t, gaia_table, gaia_tableT, c_num_gaia_table_ptrs>;
    gaia_table_t(gaia_id_t id) : gaia_object_t(id, "gaia_table_t") {
        m_gaia_field_list.set_outer(gaia_id());
        m_ref_gaia_field_list.set_outer(gaia_id());
    }
};

typedef gaia_writer_t<c_gaia_type_gaia_field, gaia_field_t, gaia_field, gaia_fieldT, c_num_gaia_field_ptrs> gaia_field_writer;
struct gaia_field_t : public gaia_object_t<c_gaia_type_gaia_field, gaia_field_t, gaia_field, gaia_fieldT, c_num_gaia_field_ptrs> {
    gaia_field_t() : gaia_object_t("gaia_field_t") {}
    const char* name() const {return GET_STR(name);}
    uint8_t type() const {return GET(type);}
    uint16_t repeated_count() const {return GET(repeated_count);}
    uint16_t position() const {return GET(position);}
    bool deprecated() const {return GET(deprecated);}
    bool active() const {return GET(active);}
    using gaia_object_t::insert_row;
    static gaia_id_t insert_row(const char* name, uint8_t type, uint16_t repeated_count, uint16_t position, bool deprecated, bool active) {
        flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
        b.Finish(Creategaia_fieldDirect(b, name, type, repeated_count, position, deprecated, active));
        return gaia_object_t::insert_row(b);
    }
    gaia_table_t gaia_table() {
        return gaia_table_t::get(this->references()[c_parent_gaia_field_gaia_table]);
    }
    gaia_table_t ref_gaia_table() {
        return gaia_table_t::get(this->references()[c_parent_ref_gaia_table]);
    }
    static gaia_container_t<c_gaia_type_gaia_field, gaia_field_t>& list() {
        static gaia_container_t<c_gaia_type_gaia_field, gaia_field_t> list;
        return list;
    }
private:
    friend struct gaia_object_t<c_gaia_type_gaia_field, gaia_field_t, gaia_field, gaia_fieldT, c_num_gaia_field_ptrs>;
    gaia_field_t(gaia_id_t id) : gaia_object_t(id, "gaia_field_t") {
    }
};

typedef gaia_writer_t<c_gaia_type_gaia_ruleset, gaia_ruleset_t, gaia_ruleset, gaia_rulesetT, c_num_gaia_ruleset_ptrs> gaia_ruleset_writer;
struct gaia_ruleset_t : public gaia_object_t<c_gaia_type_gaia_ruleset, gaia_ruleset_t, gaia_ruleset, gaia_rulesetT, c_num_gaia_ruleset_ptrs> {
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
    static gaia_container_t<c_gaia_type_gaia_ruleset, gaia_ruleset_t>& list() {
        static gaia_container_t<c_gaia_type_gaia_ruleset, gaia_ruleset_t> list;
        return list;
    }
    reference_chain_container_t<gaia_ruleset_t, gaia_rule_t, c_parent_gaia_rule_gaia_ruleset, c_first_gaia_rule_gaia_rule, c_next_gaia_rule_gaia_rule> m_gaia_rule_list;
    reference_chain_container_t<gaia_ruleset_t, gaia_rule_t, c_parent_gaia_rule_gaia_ruleset, c_first_gaia_rule_gaia_rule, c_next_gaia_rule_gaia_rule>& gaia_rule_list() {
        return m_gaia_rule_list;
    }
private:
    friend struct gaia_object_t<c_gaia_type_gaia_ruleset, gaia_ruleset_t, gaia_ruleset, gaia_rulesetT, c_num_gaia_ruleset_ptrs>;
    gaia_ruleset_t(gaia_id_t id) : gaia_object_t(id, "gaia_ruleset_t") {
        m_gaia_rule_list.set_outer(gaia_id());
    }
};

typedef gaia_writer_t<c_gaia_type_gaia_rule, gaia_rule_t, gaia_rule, gaia_ruleT, c_num_gaia_rule_ptrs> gaia_rule_writer;
struct gaia_rule_t : public gaia_object_t<c_gaia_type_gaia_rule, gaia_rule_t, gaia_rule, gaia_ruleT, c_num_gaia_rule_ptrs> {
    gaia_rule_t() : gaia_object_t("gaia_rule_t") {}
    const char* name() const {return GET_STR(name);}
    using gaia_object_t::insert_row;
    static gaia_id_t insert_row(const char* name) {
        flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
        b.Finish(Creategaia_ruleDirect(b, name));
        return gaia_object_t::insert_row(b);
    }
    gaia_ruleset_t gaia_ruleset() {
        return gaia_ruleset_t::get(this->references()[c_parent_gaia_rule_gaia_ruleset]);
    }
    static gaia_container_t<c_gaia_type_gaia_rule, gaia_rule_t>& list() {
        static gaia_container_t<c_gaia_type_gaia_rule, gaia_rule_t> list;
        return list;
    }
private:
    friend struct gaia_object_t<c_gaia_type_gaia_rule, gaia_rule_t, gaia_rule, gaia_ruleT, c_num_gaia_rule_ptrs>;
    gaia_rule_t(gaia_id_t id) : gaia_object_t(id, "gaia_rule_t") {
    }
};

}  // namespace catalog
}  // namespace gaia

#endif  // GAIA_GENERATED_catalog_H_
