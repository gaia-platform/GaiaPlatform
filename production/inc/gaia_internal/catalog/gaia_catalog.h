/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Automatically generated by the Gaia Data Classes code generator.
// Do not modify.

#include <iterator>

#ifndef GAIA_GENERATED_catalog_H_
#define GAIA_GENERATED_catalog_H_

#include "gaia/direct_access/edc_object.hpp"
#include "catalog_generated.h"
#include "gaia/direct_access/edc_iterators.hpp"

namespace gaia {
namespace catalog {

// The initial size of the flatbuffer builder buffer.
constexpr int c_flatbuffer_builder_size = 128;

// Constants contained in the gaia_rule object.
constexpr uint32_t c_gaia_type_gaia_rule = 4294967293u;
constexpr int c_parent_gaia_ruleset_gaia_rule = 0;
constexpr int c_next_gaia_ruleset_gaia_rule = 1;
constexpr int c_num_gaia_rule_ptrs = 2;

// Constants contained in the gaia_ruleset object.
constexpr uint32_t c_gaia_type_gaia_ruleset = 4294967292u;
constexpr int c_first_gaia_ruleset_gaia_rule = 0;
constexpr int c_num_gaia_ruleset_ptrs = 1;

// Constants contained in the gaia_relationship object.
constexpr uint32_t c_gaia_type_gaia_relationship = 4294967290u;
constexpr int c_parent_parent_gaia_relationship = 0;
constexpr int c_next_parent_gaia_relationship = 1;
constexpr int c_parent_child_gaia_relationship = 2;
constexpr int c_next_child_gaia_relationship = 3;
constexpr int c_num_gaia_relationship_ptrs = 4;

// Constants contained in the gaia_field object.
constexpr uint32_t c_gaia_type_gaia_field = 4294967295u;
constexpr int c_parent_gaia_table_gaia_field = 0;
constexpr int c_next_gaia_table_gaia_field = 1;
constexpr int c_num_gaia_field_ptrs = 2;

// Constants contained in the gaia_table object.
constexpr uint32_t c_gaia_type_gaia_table = 4294967294u;
constexpr int c_parent_gaia_database_gaia_table = 0;
constexpr int c_next_gaia_database_gaia_table = 1;
constexpr int c_first_gaia_table_gaia_field = 2;
constexpr int c_first_parent_gaia_relationship = 3;
constexpr int c_first_child_gaia_relationship = 4;
constexpr int c_num_gaia_table_ptrs = 5;

// Constants contained in the gaia_database object.
constexpr uint32_t c_gaia_type_gaia_database = 4294967291u;
constexpr int c_first_gaia_database_gaia_table = 0;
constexpr int c_num_gaia_database_ptrs = 1;

struct gaia_rule_t;
struct gaia_ruleset_t;
struct gaia_relationship_t;
struct gaia_field_t;
struct gaia_table_t;
struct gaia_database_t;

typedef gaia::direct_access::gaia_writer_t<c_gaia_type_gaia_database, gaia_database_t, gaia_database, gaia_databaseT, c_num_gaia_database_ptrs> gaia_database_writer;
struct gaia_database_t : public gaia::direct_access::gaia_object_t<c_gaia_type_gaia_database, gaia_database_t, gaia_database, gaia_databaseT, c_num_gaia_database_ptrs> {
    typedef gaia::direct_access::reference_chain_container_t<gaia_database_t, gaia_table_t, c_parent_gaia_database_gaia_table, c_first_gaia_database_gaia_table, c_next_gaia_database_gaia_table> gaia_table_list_t;
    gaia_database_t() : gaia_object_t("gaia_database_t") {}
    const char* name() const {return GET_STR(name);}
    using gaia_object_t::insert_row;
    static gaia::common::gaia_id_t insert_row(const char* name) {
        flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
        b.Finish(Creategaia_databaseDirect(b, name));
        return gaia_object_t::insert_row(b);
    }
    static gaia::direct_access::gaia_container_t<c_gaia_type_gaia_database, gaia_database_t>& list() {
        static gaia::direct_access::gaia_container_t<c_gaia_type_gaia_database, gaia_database_t> list;
        return list;
    }
    gaia_table_list_t gaia_table_list() {
        return gaia_table_list_t(gaia_id());
    }
private:
    friend struct gaia_object_t<c_gaia_type_gaia_database, gaia_database_t, gaia_database, gaia_databaseT, c_num_gaia_database_ptrs>;
    explicit gaia_database_t(gaia::common::gaia_id_t id) : gaia_object_t(id, "gaia_database_t") {}
};

typedef gaia::direct_access::gaia_writer_t<c_gaia_type_gaia_table, gaia_table_t, gaia_table, gaia_tableT, c_num_gaia_table_ptrs> gaia_table_writer;
struct gaia_table_t : public gaia::direct_access::gaia_object_t<c_gaia_type_gaia_table, gaia_table_t, gaia_table, gaia_tableT, c_num_gaia_table_ptrs> {
    typedef gaia::direct_access::reference_chain_container_t<gaia_table_t, gaia_field_t, c_parent_gaia_table_gaia_field, c_first_gaia_table_gaia_field, c_next_gaia_table_gaia_field> gaia_field_list_t;
    typedef gaia::direct_access::reference_chain_container_t<gaia_table_t, gaia_relationship_t, c_parent_parent_gaia_relationship, c_first_parent_gaia_relationship, c_next_parent_gaia_relationship> parent_gaia_relationship_list_t;
    typedef gaia::direct_access::reference_chain_container_t<gaia_table_t, gaia_relationship_t, c_parent_child_gaia_relationship, c_first_child_gaia_relationship, c_next_child_gaia_relationship> child_gaia_relationship_list_t;
    gaia_table_t() : gaia_object_t("gaia_table_t") {}
    const char* name() const {return GET_STR(name);}
    uint32_t type() const {return GET(type);}
    bool is_system() const {return GET(is_system);}
    const char* binary_schema() const {return GET_STR(binary_schema);}
    const char* serialization_template() const {return GET_STR(serialization_template);}
    using gaia_object_t::insert_row;
    static gaia::common::gaia_id_t insert_row(const char* name, uint32_t type, bool is_system, const char* binary_schema, const char* serialization_template) {
        flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
        b.Finish(Creategaia_tableDirect(b, name, type, is_system, binary_schema, serialization_template));
        return gaia_object_t::insert_row(b);
    }
    gaia_database_t gaia_database() {
        return gaia_database_t::get(this->references()[c_parent_gaia_database_gaia_table]);
    }
    static gaia::direct_access::gaia_container_t<c_gaia_type_gaia_table, gaia_table_t>& list() {
        static gaia::direct_access::gaia_container_t<c_gaia_type_gaia_table, gaia_table_t> list;
        return list;
    }
    gaia_field_list_t gaia_field_list() {
        return gaia_field_list_t(gaia_id());
    }
    parent_gaia_relationship_list_t parent_gaia_relationship_list() {
        return parent_gaia_relationship_list_t(gaia_id());
    }
    child_gaia_relationship_list_t child_gaia_relationship_list() {
        return child_gaia_relationship_list_t(gaia_id());
    }
private:
    friend struct gaia_object_t<c_gaia_type_gaia_table, gaia_table_t, gaia_table, gaia_tableT, c_num_gaia_table_ptrs>;
    explicit gaia_table_t(gaia::common::gaia_id_t id) : gaia_object_t(id, "gaia_table_t") {}
};

typedef gaia::direct_access::gaia_writer_t<c_gaia_type_gaia_field, gaia_field_t, gaia_field, gaia_fieldT, c_num_gaia_field_ptrs> gaia_field_writer;
struct gaia_field_t : public gaia::direct_access::gaia_object_t<c_gaia_type_gaia_field, gaia_field_t, gaia_field, gaia_fieldT, c_num_gaia_field_ptrs> {
    gaia_field_t() : gaia_object_t("gaia_field_t") {}
    const char* name() const {return GET_STR(name);}
    uint8_t type() const {return GET(type);}
    uint16_t repeated_count() const {return GET(repeated_count);}
    uint16_t position() const {return GET(position);}
    bool deprecated() const {return GET(deprecated);}
    bool active() const {return GET(active);}
    using gaia_object_t::insert_row;
    static gaia::common::gaia_id_t insert_row(const char* name, uint8_t type, uint16_t repeated_count, uint16_t position, bool deprecated, bool active) {
        flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
        b.Finish(Creategaia_fieldDirect(b, name, type, repeated_count, position, deprecated, active));
        return gaia_object_t::insert_row(b);
    }
    gaia_table_t gaia_table() {
        return gaia_table_t::get(this->references()[c_parent_gaia_table_gaia_field]);
    }
    static gaia::direct_access::gaia_container_t<c_gaia_type_gaia_field, gaia_field_t>& list() {
        static gaia::direct_access::gaia_container_t<c_gaia_type_gaia_field, gaia_field_t> list;
        return list;
    }
private:
    friend struct gaia_object_t<c_gaia_type_gaia_field, gaia_field_t, gaia_field, gaia_fieldT, c_num_gaia_field_ptrs>;
    explicit gaia_field_t(gaia::common::gaia_id_t id) : gaia_object_t(id, "gaia_field_t") {}
};

typedef gaia::direct_access::gaia_writer_t<c_gaia_type_gaia_relationship, gaia_relationship_t, gaia_relationship, gaia_relationshipT, c_num_gaia_relationship_ptrs> gaia_relationship_writer;
struct gaia_relationship_t : public gaia::direct_access::gaia_object_t<c_gaia_type_gaia_relationship, gaia_relationship_t, gaia_relationship, gaia_relationshipT, c_num_gaia_relationship_ptrs> {
    gaia_relationship_t() : gaia_object_t("gaia_relationship_t") {}
    const char* name() const {return GET_STR(name);}
    uint8_t cardinality() const {return GET(cardinality);}
    bool parent_required() const {return GET(parent_required);}
    bool deprecated() const {return GET(deprecated);}
    uint16_t first_child_offset() const {return GET(first_child_offset);}
    uint16_t next_child_offset() const {return GET(next_child_offset);}
    uint16_t parent_offset() const {return GET(parent_offset);}
    using gaia_object_t::insert_row;
    static gaia::common::gaia_id_t insert_row(const char* name, uint8_t cardinality, bool parent_required, bool deprecated, uint16_t first_child_offset, uint16_t next_child_offset, uint16_t parent_offset) {
        flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
        b.Finish(Creategaia_relationshipDirect(b, name, cardinality, parent_required, deprecated, first_child_offset, next_child_offset, parent_offset));
        return gaia_object_t::insert_row(b);
    }
    gaia_table_t parent_gaia_table() {
        return gaia_table_t::get(this->references()[c_parent_parent_gaia_relationship]);
    }
    gaia_table_t child_gaia_table() {
        return gaia_table_t::get(this->references()[c_parent_child_gaia_relationship]);
    }
    static gaia::direct_access::gaia_container_t<c_gaia_type_gaia_relationship, gaia_relationship_t>& list() {
        static gaia::direct_access::gaia_container_t<c_gaia_type_gaia_relationship, gaia_relationship_t> list;
        return list;
    }
private:
    friend struct gaia_object_t<c_gaia_type_gaia_relationship, gaia_relationship_t, gaia_relationship, gaia_relationshipT, c_num_gaia_relationship_ptrs>;
    explicit gaia_relationship_t(gaia::common::gaia_id_t id) : gaia_object_t(id, "gaia_relationship_t") {}
};

typedef gaia::direct_access::gaia_writer_t<c_gaia_type_gaia_ruleset, gaia_ruleset_t, gaia_ruleset, gaia_rulesetT, c_num_gaia_ruleset_ptrs> gaia_ruleset_writer;
struct gaia_ruleset_t : public gaia::direct_access::gaia_object_t<c_gaia_type_gaia_ruleset, gaia_ruleset_t, gaia_ruleset, gaia_rulesetT, c_num_gaia_ruleset_ptrs> {
    typedef gaia::direct_access::reference_chain_container_t<gaia_ruleset_t, gaia_rule_t, c_parent_gaia_ruleset_gaia_rule, c_first_gaia_ruleset_gaia_rule, c_next_gaia_ruleset_gaia_rule> gaia_rule_list_t;
    gaia_ruleset_t() : gaia_object_t("gaia_ruleset_t") {}
    const char* name() const {return GET_STR(name);}
    bool active_on_startup() const {return GET(active_on_startup);}
    const char* table_ids() const {return GET_STR(table_ids);}
    const char* source_location() const {return GET_STR(source_location);}
    const char* serial_stream() const {return GET_STR(serial_stream);}
    using gaia_object_t::insert_row;
    static gaia::common::gaia_id_t insert_row(const char* name, bool active_on_startup, const char* table_ids, const char* source_location, const char* serial_stream) {
        flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
        b.Finish(Creategaia_rulesetDirect(b, name, active_on_startup, table_ids, source_location, serial_stream));
        return gaia_object_t::insert_row(b);
    }
    static gaia::direct_access::gaia_container_t<c_gaia_type_gaia_ruleset, gaia_ruleset_t>& list() {
        static gaia::direct_access::gaia_container_t<c_gaia_type_gaia_ruleset, gaia_ruleset_t> list;
        return list;
    }
    gaia_rule_list_t gaia_rule_list() {
        return gaia_rule_list_t(gaia_id());
    }
private:
    friend struct gaia_object_t<c_gaia_type_gaia_ruleset, gaia_ruleset_t, gaia_ruleset, gaia_rulesetT, c_num_gaia_ruleset_ptrs>;
    explicit gaia_ruleset_t(gaia::common::gaia_id_t id) : gaia_object_t(id, "gaia_ruleset_t") {}
};

typedef gaia::direct_access::gaia_writer_t<c_gaia_type_gaia_rule, gaia_rule_t, gaia_rule, gaia_ruleT, c_num_gaia_rule_ptrs> gaia_rule_writer;
struct gaia_rule_t : public gaia::direct_access::gaia_object_t<c_gaia_type_gaia_rule, gaia_rule_t, gaia_rule, gaia_ruleT, c_num_gaia_rule_ptrs> {
    gaia_rule_t() : gaia_object_t("gaia_rule_t") {}
    const char* name() const {return GET_STR(name);}
    using gaia_object_t::insert_row;
    static gaia::common::gaia_id_t insert_row(const char* name) {
        flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
        b.Finish(Creategaia_ruleDirect(b, name));
        return gaia_object_t::insert_row(b);
    }
    gaia_ruleset_t gaia_ruleset() {
        return gaia_ruleset_t::get(this->references()[c_parent_gaia_ruleset_gaia_rule]);
    }
    static gaia::direct_access::gaia_container_t<c_gaia_type_gaia_rule, gaia_rule_t>& list() {
        static gaia::direct_access::gaia_container_t<c_gaia_type_gaia_rule, gaia_rule_t> list;
        return list;
    }
private:
    friend struct gaia_object_t<c_gaia_type_gaia_rule, gaia_rule_t, gaia_rule, gaia_ruleT, c_num_gaia_rule_ptrs>;
    explicit gaia_rule_t(gaia::common::gaia_id_t id) : gaia_object_t(id, "gaia_rule_t") {}
};

}  // namespace catalog
}  // namespace gaia

#endif  // GAIA_GENERATED_catalog_H_

