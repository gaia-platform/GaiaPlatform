/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Automatically generated by the Gaia Extended Data Classes code generator.
// Do not modify.

#ifndef GAIA_GENERATED_catalog_H_
#define GAIA_GENERATED_catalog_H_

#include <gaia/direct_access/edc_object.hpp>
#include <gaia/direct_access/edc_iterators.hpp>
#include "catalog_generated.h"

namespace gaia {
namespace catalog {

// The initial size of the flatbuffer builder buffer.
constexpr int c_flatbuffer_builder_size = 128;

// Constants contained in the gaia_index object.
constexpr uint32_t c_gaia_type_gaia_index = 4294967289u;
constexpr int c_gaia_index_parent_table = 0;
constexpr int c_gaia_index_next_table = 1;

// Constants contained in the gaia_rule object.
constexpr uint32_t c_gaia_type_gaia_rule = 4294967293u;
constexpr int c_gaia_rule_parent_ruleset = 0;
constexpr int c_gaia_rule_next_ruleset = 1;

// Constants contained in the gaia_ruleset object.
constexpr uint32_t c_gaia_type_gaia_ruleset = 4294967292u;
constexpr int c_gaia_ruleset_first_gaia_rules = 0;

// Constants contained in the gaia_relationship object.
constexpr uint32_t c_gaia_type_gaia_relationship = 4294967290u;
constexpr int c_gaia_relationship_parent_parent = 0;
constexpr int c_gaia_relationship_next_parent = 1;
constexpr int c_gaia_relationship_parent_child = 2;
constexpr int c_gaia_relationship_next_child = 3;

// Constants contained in the gaia_field object.
constexpr uint32_t c_gaia_type_gaia_field = 4294967295u;
constexpr int c_gaia_field_parent_table = 0;
constexpr int c_gaia_field_next_table = 1;

// Constants contained in the gaia_table object.
constexpr uint32_t c_gaia_type_gaia_table = 4294967294u;
constexpr int c_gaia_table_parent_database = 0;
constexpr int c_gaia_table_next_database = 1;
constexpr int c_gaia_table_first_gaia_fields = 2;
constexpr int c_gaia_table_first_outgoing_relationships = 3;
constexpr int c_gaia_table_first_incoming_relationships = 4;
constexpr int c_gaia_table_first_gaia_indexes = 5;

// Constants contained in the gaia_database object.
constexpr uint32_t c_gaia_type_gaia_database = 4294967291u;
constexpr int c_gaia_database_first_gaia_tables = 0;


class gaia_index_t;
class gaia_rule_t;
class gaia_ruleset_t;
class gaia_relationship_t;
class gaia_field_t;
class gaia_table_t;
class gaia_database_t;


typedef gaia::direct_access::edc_writer_t<c_gaia_type_gaia_index, gaia_index_t, internal::gaia_index, internal::gaia_indexT> gaia_index_writer;
class gaia_index_t : public gaia::direct_access::edc_object_t<c_gaia_type_gaia_index, gaia_index_t, internal::gaia_index, internal::gaia_indexT> {
    friend class edc_object_t<c_gaia_type_gaia_index, gaia_index_t, internal::gaia_index, internal::gaia_indexT>;
public:
    gaia_index_t() : edc_object_t() {}
    static const char* gaia_typename();
    static gaia::common::gaia_id_t insert_row(const char* name, bool unique, uint8_t type, const std::vector<uint64_t>& fields);
    static gaia::direct_access::edc_container_t<c_gaia_type_gaia_index, gaia_index_t> list();
    const char* name() const;
    bool unique() const;
    uint8_t type() const;
    gaia::direct_access::edc_vector_t<uint64_t> fields() const;
    gaia_table_t table() const;

    template<class unused_t>
    struct expr_ {
        static gaia::direct_access::expression_t<gaia_index_t, gaia::common::gaia_id_t> gaia_id;
        static gaia::direct_access::expression_t<gaia_index_t, const char*> name;
        static gaia::direct_access::expression_t<gaia_index_t, bool> unique;
        static gaia::direct_access::expression_t<gaia_index_t, uint8_t> type;
        static gaia::direct_access::expression_t<gaia_index_t, gaia::direct_access::edc_vector_t<uint64_t>> fields;
        static gaia::direct_access::expression_t<gaia_index_t, gaia_table_t> table;
    };
    using expr = expr_<void>;
private:
    explicit gaia_index_t(gaia::common::gaia_id_t id) : edc_object_t(id) {}
};

namespace gaia_index_expr {
    static auto& gaia_id = gaia_index_t::expr::gaia_id;
    static auto& name = gaia_index_t::expr::name;
    static auto& unique = gaia_index_t::expr::unique;
    static auto& type = gaia_index_t::expr::type;
    static auto& fields = gaia_index_t::expr::fields;
    static auto& table = gaia_index_t::expr::table;
};

template<class unused_t> gaia::direct_access::expression_t<gaia_index_t, gaia::common::gaia_id_t> gaia_index_t::expr_<unused_t>::gaia_id{&gaia_index_t::gaia_id};
template<class unused_t> gaia::direct_access::expression_t<gaia_index_t, const char*> gaia_index_t::expr_<unused_t>::name{&gaia_index_t::name};
template<class unused_t> gaia::direct_access::expression_t<gaia_index_t, bool> gaia_index_t::expr_<unused_t>::unique{&gaia_index_t::unique};
template<class unused_t> gaia::direct_access::expression_t<gaia_index_t, uint8_t> gaia_index_t::expr_<unused_t>::type{&gaia_index_t::type};
template<class unused_t> gaia::direct_access::expression_t<gaia_index_t, gaia::direct_access::edc_vector_t<uint64_t>> gaia_index_t::expr_<unused_t>::fields{&gaia_index_t::fields};
template<class unused_t> gaia::direct_access::expression_t<gaia_index_t, gaia_table_t> gaia_index_t::expr_<unused_t>::table{&gaia_index_t::table};


typedef gaia::direct_access::edc_writer_t<c_gaia_type_gaia_rule, gaia_rule_t, internal::gaia_rule, internal::gaia_ruleT> gaia_rule_writer;
class gaia_rule_t : public gaia::direct_access::edc_object_t<c_gaia_type_gaia_rule, gaia_rule_t, internal::gaia_rule, internal::gaia_ruleT> {
    friend class edc_object_t<c_gaia_type_gaia_rule, gaia_rule_t, internal::gaia_rule, internal::gaia_ruleT>;
public:
    gaia_rule_t() : edc_object_t() {}
    static const char* gaia_typename();
    static gaia::common::gaia_id_t insert_row(const char* name);
    static gaia::direct_access::edc_container_t<c_gaia_type_gaia_rule, gaia_rule_t> list();
    const char* name() const;
    gaia_ruleset_t ruleset() const;

    template<class unused_t>
    struct expr_ {
        static gaia::direct_access::expression_t<gaia_rule_t, gaia::common::gaia_id_t> gaia_id;
        static gaia::direct_access::expression_t<gaia_rule_t, const char*> name;
        static gaia::direct_access::expression_t<gaia_rule_t, gaia_ruleset_t> ruleset;
    };
    using expr = expr_<void>;
private:
    explicit gaia_rule_t(gaia::common::gaia_id_t id) : edc_object_t(id) {}
};

namespace gaia_rule_expr {
    static auto& gaia_id = gaia_rule_t::expr::gaia_id;
    static auto& name = gaia_rule_t::expr::name;
    static auto& ruleset = gaia_rule_t::expr::ruleset;
};

template<class unused_t> gaia::direct_access::expression_t<gaia_rule_t, gaia::common::gaia_id_t> gaia_rule_t::expr_<unused_t>::gaia_id{&gaia_rule_t::gaia_id};
template<class unused_t> gaia::direct_access::expression_t<gaia_rule_t, const char*> gaia_rule_t::expr_<unused_t>::name{&gaia_rule_t::name};
template<class unused_t> gaia::direct_access::expression_t<gaia_rule_t, gaia_ruleset_t> gaia_rule_t::expr_<unused_t>::ruleset{&gaia_rule_t::ruleset};


typedef gaia::direct_access::edc_writer_t<c_gaia_type_gaia_ruleset, gaia_ruleset_t, internal::gaia_ruleset, internal::gaia_rulesetT> gaia_ruleset_writer;
class gaia_ruleset_t : public gaia::direct_access::edc_object_t<c_gaia_type_gaia_ruleset, gaia_ruleset_t, internal::gaia_ruleset, internal::gaia_rulesetT> {
    friend class edc_object_t<c_gaia_type_gaia_ruleset, gaia_ruleset_t, internal::gaia_ruleset, internal::gaia_rulesetT>;
public:
    typedef gaia::direct_access::reference_chain_container_t<gaia_rule_t> gaia_rules_list_t;
    gaia_ruleset_t() : edc_object_t() {}
    static const char* gaia_typename();
    static gaia::common::gaia_id_t insert_row(const char* name, bool active_on_startup, const std::vector<uint64_t>& table_ids, const char* source_location, const char* serial_stream);
    static gaia::direct_access::edc_container_t<c_gaia_type_gaia_ruleset, gaia_ruleset_t> list();
    const char* name() const;
    bool active_on_startup() const;
    gaia::direct_access::edc_vector_t<uint64_t> table_ids() const;
    const char* source_location() const;
    const char* serial_stream() const;
    gaia_rules_list_t gaia_rules() const;

    template<class unused_t>
    struct expr_ {
        static gaia::direct_access::expression_t<gaia_ruleset_t, gaia::common::gaia_id_t> gaia_id;
        static gaia::direct_access::expression_t<gaia_ruleset_t, const char*> name;
        static gaia::direct_access::expression_t<gaia_ruleset_t, bool> active_on_startup;
        static gaia::direct_access::expression_t<gaia_ruleset_t, gaia::direct_access::edc_vector_t<uint64_t>> table_ids;
        static gaia::direct_access::expression_t<gaia_ruleset_t, const char*> source_location;
        static gaia::direct_access::expression_t<gaia_ruleset_t, const char*> serial_stream;
        static gaia::direct_access::expression_t<gaia_ruleset_t, gaia_ruleset_t::gaia_rules_list_t> gaia_rules;
    };
    using expr = expr_<void>;
private:
    explicit gaia_ruleset_t(gaia::common::gaia_id_t id) : edc_object_t(id) {}
};

namespace gaia_ruleset_expr {
    static auto& gaia_id = gaia_ruleset_t::expr::gaia_id;
    static auto& name = gaia_ruleset_t::expr::name;
    static auto& active_on_startup = gaia_ruleset_t::expr::active_on_startup;
    static auto& table_ids = gaia_ruleset_t::expr::table_ids;
    static auto& source_location = gaia_ruleset_t::expr::source_location;
    static auto& serial_stream = gaia_ruleset_t::expr::serial_stream;
    static auto& gaia_rules = gaia_ruleset_t::expr::gaia_rules;
};

template<class unused_t> gaia::direct_access::expression_t<gaia_ruleset_t, gaia::common::gaia_id_t> gaia_ruleset_t::expr_<unused_t>::gaia_id{&gaia_ruleset_t::gaia_id};
template<class unused_t> gaia::direct_access::expression_t<gaia_ruleset_t, const char*> gaia_ruleset_t::expr_<unused_t>::name{&gaia_ruleset_t::name};
template<class unused_t> gaia::direct_access::expression_t<gaia_ruleset_t, bool> gaia_ruleset_t::expr_<unused_t>::active_on_startup{&gaia_ruleset_t::active_on_startup};
template<class unused_t> gaia::direct_access::expression_t<gaia_ruleset_t, gaia::direct_access::edc_vector_t<uint64_t>> gaia_ruleset_t::expr_<unused_t>::table_ids{&gaia_ruleset_t::table_ids};
template<class unused_t> gaia::direct_access::expression_t<gaia_ruleset_t, const char*> gaia_ruleset_t::expr_<unused_t>::source_location{&gaia_ruleset_t::source_location};
template<class unused_t> gaia::direct_access::expression_t<gaia_ruleset_t, const char*> gaia_ruleset_t::expr_<unused_t>::serial_stream{&gaia_ruleset_t::serial_stream};
template<class unused_t> gaia::direct_access::expression_t<gaia_ruleset_t, gaia_ruleset_t::gaia_rules_list_t> gaia_ruleset_t::expr_<unused_t>::gaia_rules{&gaia_ruleset_t::gaia_rules};


typedef gaia::direct_access::edc_writer_t<c_gaia_type_gaia_relationship, gaia_relationship_t, internal::gaia_relationship, internal::gaia_relationshipT> gaia_relationship_writer;
class gaia_relationship_t : public gaia::direct_access::edc_object_t<c_gaia_type_gaia_relationship, gaia_relationship_t, internal::gaia_relationship, internal::gaia_relationshipT> {
    friend class edc_object_t<c_gaia_type_gaia_relationship, gaia_relationship_t, internal::gaia_relationship, internal::gaia_relationshipT>;
public:
    gaia_relationship_t() : edc_object_t() {}
    static const char* gaia_typename();
    static gaia::common::gaia_id_t insert_row(const char* name, const char* to_parent_link_name, const char* to_child_link_name, uint8_t cardinality, bool parent_required, bool deprecated, uint16_t first_child_offset, uint16_t next_child_offset, uint16_t parent_offset);
    static gaia::direct_access::edc_container_t<c_gaia_type_gaia_relationship, gaia_relationship_t> list();
    const char* name() const;
    const char* to_parent_link_name() const;
    const char* to_child_link_name() const;
    uint8_t cardinality() const;
    bool parent_required() const;
    bool deprecated() const;
    uint16_t first_child_offset() const;
    uint16_t next_child_offset() const;
    uint16_t parent_offset() const;
    gaia_table_t child() const;
    gaia_table_t parent() const;

    template<class unused_t>
    struct expr_ {
        static gaia::direct_access::expression_t<gaia_relationship_t, gaia::common::gaia_id_t> gaia_id;
        static gaia::direct_access::expression_t<gaia_relationship_t, const char*> name;
        static gaia::direct_access::expression_t<gaia_relationship_t, const char*> to_parent_link_name;
        static gaia::direct_access::expression_t<gaia_relationship_t, const char*> to_child_link_name;
        static gaia::direct_access::expression_t<gaia_relationship_t, uint8_t> cardinality;
        static gaia::direct_access::expression_t<gaia_relationship_t, bool> parent_required;
        static gaia::direct_access::expression_t<gaia_relationship_t, bool> deprecated;
        static gaia::direct_access::expression_t<gaia_relationship_t, uint16_t> first_child_offset;
        static gaia::direct_access::expression_t<gaia_relationship_t, uint16_t> next_child_offset;
        static gaia::direct_access::expression_t<gaia_relationship_t, uint16_t> parent_offset;
        static gaia::direct_access::expression_t<gaia_relationship_t, gaia_table_t> child;
        static gaia::direct_access::expression_t<gaia_relationship_t, gaia_table_t> parent;
    };
    using expr = expr_<void>;
private:
    explicit gaia_relationship_t(gaia::common::gaia_id_t id) : edc_object_t(id) {}
};

namespace gaia_relationship_expr {
    static auto& gaia_id = gaia_relationship_t::expr::gaia_id;
    static auto& name = gaia_relationship_t::expr::name;
    static auto& to_parent_link_name = gaia_relationship_t::expr::to_parent_link_name;
    static auto& to_child_link_name = gaia_relationship_t::expr::to_child_link_name;
    static auto& cardinality = gaia_relationship_t::expr::cardinality;
    static auto& parent_required = gaia_relationship_t::expr::parent_required;
    static auto& deprecated = gaia_relationship_t::expr::deprecated;
    static auto& first_child_offset = gaia_relationship_t::expr::first_child_offset;
    static auto& next_child_offset = gaia_relationship_t::expr::next_child_offset;
    static auto& parent_offset = gaia_relationship_t::expr::parent_offset;
    static auto& child = gaia_relationship_t::expr::child;
    static auto& parent = gaia_relationship_t::expr::parent;
};

template<class unused_t> gaia::direct_access::expression_t<gaia_relationship_t, gaia::common::gaia_id_t> gaia_relationship_t::expr_<unused_t>::gaia_id{&gaia_relationship_t::gaia_id};
template<class unused_t> gaia::direct_access::expression_t<gaia_relationship_t, const char*> gaia_relationship_t::expr_<unused_t>::name{&gaia_relationship_t::name};
template<class unused_t> gaia::direct_access::expression_t<gaia_relationship_t, const char*> gaia_relationship_t::expr_<unused_t>::to_parent_link_name{&gaia_relationship_t::to_parent_link_name};
template<class unused_t> gaia::direct_access::expression_t<gaia_relationship_t, const char*> gaia_relationship_t::expr_<unused_t>::to_child_link_name{&gaia_relationship_t::to_child_link_name};
template<class unused_t> gaia::direct_access::expression_t<gaia_relationship_t, uint8_t> gaia_relationship_t::expr_<unused_t>::cardinality{&gaia_relationship_t::cardinality};
template<class unused_t> gaia::direct_access::expression_t<gaia_relationship_t, bool> gaia_relationship_t::expr_<unused_t>::parent_required{&gaia_relationship_t::parent_required};
template<class unused_t> gaia::direct_access::expression_t<gaia_relationship_t, bool> gaia_relationship_t::expr_<unused_t>::deprecated{&gaia_relationship_t::deprecated};
template<class unused_t> gaia::direct_access::expression_t<gaia_relationship_t, uint16_t> gaia_relationship_t::expr_<unused_t>::first_child_offset{&gaia_relationship_t::first_child_offset};
template<class unused_t> gaia::direct_access::expression_t<gaia_relationship_t, uint16_t> gaia_relationship_t::expr_<unused_t>::next_child_offset{&gaia_relationship_t::next_child_offset};
template<class unused_t> gaia::direct_access::expression_t<gaia_relationship_t, uint16_t> gaia_relationship_t::expr_<unused_t>::parent_offset{&gaia_relationship_t::parent_offset};
template<class unused_t> gaia::direct_access::expression_t<gaia_relationship_t, gaia_table_t> gaia_relationship_t::expr_<unused_t>::child{&gaia_relationship_t::child};
template<class unused_t> gaia::direct_access::expression_t<gaia_relationship_t, gaia_table_t> gaia_relationship_t::expr_<unused_t>::parent{&gaia_relationship_t::parent};


typedef gaia::direct_access::edc_writer_t<c_gaia_type_gaia_field, gaia_field_t, internal::gaia_field, internal::gaia_fieldT> gaia_field_writer;
class gaia_field_t : public gaia::direct_access::edc_object_t<c_gaia_type_gaia_field, gaia_field_t, internal::gaia_field, internal::gaia_fieldT> {
    friend class edc_object_t<c_gaia_type_gaia_field, gaia_field_t, internal::gaia_field, internal::gaia_fieldT>;
public:
    gaia_field_t() : edc_object_t() {}
    static const char* gaia_typename();
    static gaia::common::gaia_id_t insert_row(const char* name, uint8_t type, uint16_t repeated_count, uint16_t position, bool deprecated, bool active);
    static gaia::direct_access::edc_container_t<c_gaia_type_gaia_field, gaia_field_t> list();
    const char* name() const;
    uint8_t type() const;
    uint16_t repeated_count() const;
    uint16_t position() const;
    bool deprecated() const;
    bool active() const;
    gaia_table_t table() const;

    template<class unused_t>
    struct expr_ {
        static gaia::direct_access::expression_t<gaia_field_t, gaia::common::gaia_id_t> gaia_id;
        static gaia::direct_access::expression_t<gaia_field_t, const char*> name;
        static gaia::direct_access::expression_t<gaia_field_t, uint8_t> type;
        static gaia::direct_access::expression_t<gaia_field_t, uint16_t> repeated_count;
        static gaia::direct_access::expression_t<gaia_field_t, uint16_t> position;
        static gaia::direct_access::expression_t<gaia_field_t, bool> deprecated;
        static gaia::direct_access::expression_t<gaia_field_t, bool> active;
        static gaia::direct_access::expression_t<gaia_field_t, gaia_table_t> table;
    };
    using expr = expr_<void>;
private:
    explicit gaia_field_t(gaia::common::gaia_id_t id) : edc_object_t(id) {}
};

namespace gaia_field_expr {
    static auto& gaia_id = gaia_field_t::expr::gaia_id;
    static auto& name = gaia_field_t::expr::name;
    static auto& type = gaia_field_t::expr::type;
    static auto& repeated_count = gaia_field_t::expr::repeated_count;
    static auto& position = gaia_field_t::expr::position;
    static auto& deprecated = gaia_field_t::expr::deprecated;
    static auto& active = gaia_field_t::expr::active;
    static auto& table = gaia_field_t::expr::table;
};

template<class unused_t> gaia::direct_access::expression_t<gaia_field_t, gaia::common::gaia_id_t> gaia_field_t::expr_<unused_t>::gaia_id{&gaia_field_t::gaia_id};
template<class unused_t> gaia::direct_access::expression_t<gaia_field_t, const char*> gaia_field_t::expr_<unused_t>::name{&gaia_field_t::name};
template<class unused_t> gaia::direct_access::expression_t<gaia_field_t, uint8_t> gaia_field_t::expr_<unused_t>::type{&gaia_field_t::type};
template<class unused_t> gaia::direct_access::expression_t<gaia_field_t, uint16_t> gaia_field_t::expr_<unused_t>::repeated_count{&gaia_field_t::repeated_count};
template<class unused_t> gaia::direct_access::expression_t<gaia_field_t, uint16_t> gaia_field_t::expr_<unused_t>::position{&gaia_field_t::position};
template<class unused_t> gaia::direct_access::expression_t<gaia_field_t, bool> gaia_field_t::expr_<unused_t>::deprecated{&gaia_field_t::deprecated};
template<class unused_t> gaia::direct_access::expression_t<gaia_field_t, bool> gaia_field_t::expr_<unused_t>::active{&gaia_field_t::active};
template<class unused_t> gaia::direct_access::expression_t<gaia_field_t, gaia_table_t> gaia_field_t::expr_<unused_t>::table{&gaia_field_t::table};


typedef gaia::direct_access::edc_writer_t<c_gaia_type_gaia_table, gaia_table_t, internal::gaia_table, internal::gaia_tableT> gaia_table_writer;
class gaia_table_t : public gaia::direct_access::edc_object_t<c_gaia_type_gaia_table, gaia_table_t, internal::gaia_table, internal::gaia_tableT> {
    friend class edc_object_t<c_gaia_type_gaia_table, gaia_table_t, internal::gaia_table, internal::gaia_tableT>;
public:
    typedef gaia::direct_access::reference_chain_container_t<gaia_index_t> gaia_indexes_list_t;
    typedef gaia::direct_access::reference_chain_container_t<gaia_relationship_t> incoming_relationships_list_t;
    typedef gaia::direct_access::reference_chain_container_t<gaia_relationship_t> outgoing_relationships_list_t;
    typedef gaia::direct_access::reference_chain_container_t<gaia_field_t> gaia_fields_list_t;
    gaia_table_t() : edc_object_t() {}
    static const char* gaia_typename();
    static gaia::common::gaia_id_t insert_row(const char* name, uint32_t type, bool is_system, const std::vector<uint8_t>& binary_schema, const std::vector<uint8_t>& serialization_template);
    static gaia::direct_access::edc_container_t<c_gaia_type_gaia_table, gaia_table_t> list();
    const char* name() const;
    uint32_t type() const;
    bool is_system() const;
    gaia::direct_access::edc_vector_t<uint8_t> binary_schema() const;
    gaia::direct_access::edc_vector_t<uint8_t> serialization_template() const;
    gaia_database_t database() const;
    gaia_indexes_list_t gaia_indexes() const;
    incoming_relationships_list_t incoming_relationships() const;
    outgoing_relationships_list_t outgoing_relationships() const;
    gaia_fields_list_t gaia_fields() const;

    template<class unused_t>
    struct expr_ {
        static gaia::direct_access::expression_t<gaia_table_t, gaia::common::gaia_id_t> gaia_id;
        static gaia::direct_access::expression_t<gaia_table_t, const char*> name;
        static gaia::direct_access::expression_t<gaia_table_t, uint32_t> type;
        static gaia::direct_access::expression_t<gaia_table_t, bool> is_system;
        static gaia::direct_access::expression_t<gaia_table_t, gaia::direct_access::edc_vector_t<uint8_t>> binary_schema;
        static gaia::direct_access::expression_t<gaia_table_t, gaia::direct_access::edc_vector_t<uint8_t>> serialization_template;
        static gaia::direct_access::expression_t<gaia_table_t, gaia_database_t> database;
        static gaia::direct_access::expression_t<gaia_table_t, gaia_table_t::gaia_indexes_list_t> gaia_indexes;
        static gaia::direct_access::expression_t<gaia_table_t, gaia_table_t::incoming_relationships_list_t> incoming_relationships;
        static gaia::direct_access::expression_t<gaia_table_t, gaia_table_t::outgoing_relationships_list_t> outgoing_relationships;
        static gaia::direct_access::expression_t<gaia_table_t, gaia_table_t::gaia_fields_list_t> gaia_fields;
    };
    using expr = expr_<void>;
private:
    explicit gaia_table_t(gaia::common::gaia_id_t id) : edc_object_t(id) {}
};

namespace gaia_table_expr {
    static auto& gaia_id = gaia_table_t::expr::gaia_id;
    static auto& name = gaia_table_t::expr::name;
    static auto& type = gaia_table_t::expr::type;
    static auto& is_system = gaia_table_t::expr::is_system;
    static auto& binary_schema = gaia_table_t::expr::binary_schema;
    static auto& serialization_template = gaia_table_t::expr::serialization_template;
    static auto& database = gaia_table_t::expr::database;
    static auto& gaia_indexes = gaia_table_t::expr::gaia_indexes;
    static auto& incoming_relationships = gaia_table_t::expr::incoming_relationships;
    static auto& outgoing_relationships = gaia_table_t::expr::outgoing_relationships;
    static auto& gaia_fields = gaia_table_t::expr::gaia_fields;
};

template<class unused_t> gaia::direct_access::expression_t<gaia_table_t, gaia::common::gaia_id_t> gaia_table_t::expr_<unused_t>::gaia_id{&gaia_table_t::gaia_id};
template<class unused_t> gaia::direct_access::expression_t<gaia_table_t, const char*> gaia_table_t::expr_<unused_t>::name{&gaia_table_t::name};
template<class unused_t> gaia::direct_access::expression_t<gaia_table_t, uint32_t> gaia_table_t::expr_<unused_t>::type{&gaia_table_t::type};
template<class unused_t> gaia::direct_access::expression_t<gaia_table_t, bool> gaia_table_t::expr_<unused_t>::is_system{&gaia_table_t::is_system};
template<class unused_t> gaia::direct_access::expression_t<gaia_table_t, gaia::direct_access::edc_vector_t<uint8_t>> gaia_table_t::expr_<unused_t>::binary_schema{&gaia_table_t::binary_schema};
template<class unused_t> gaia::direct_access::expression_t<gaia_table_t, gaia::direct_access::edc_vector_t<uint8_t>> gaia_table_t::expr_<unused_t>::serialization_template{&gaia_table_t::serialization_template};
template<class unused_t> gaia::direct_access::expression_t<gaia_table_t, gaia_database_t> gaia_table_t::expr_<unused_t>::database{&gaia_table_t::database};
template<class unused_t> gaia::direct_access::expression_t<gaia_table_t, gaia_table_t::gaia_indexes_list_t> gaia_table_t::expr_<unused_t>::gaia_indexes{&gaia_table_t::gaia_indexes};
template<class unused_t> gaia::direct_access::expression_t<gaia_table_t, gaia_table_t::incoming_relationships_list_t> gaia_table_t::expr_<unused_t>::incoming_relationships{&gaia_table_t::incoming_relationships};
template<class unused_t> gaia::direct_access::expression_t<gaia_table_t, gaia_table_t::outgoing_relationships_list_t> gaia_table_t::expr_<unused_t>::outgoing_relationships{&gaia_table_t::outgoing_relationships};
template<class unused_t> gaia::direct_access::expression_t<gaia_table_t, gaia_table_t::gaia_fields_list_t> gaia_table_t::expr_<unused_t>::gaia_fields{&gaia_table_t::gaia_fields};


typedef gaia::direct_access::edc_writer_t<c_gaia_type_gaia_database, gaia_database_t, internal::gaia_database, internal::gaia_databaseT> gaia_database_writer;
class gaia_database_t : public gaia::direct_access::edc_object_t<c_gaia_type_gaia_database, gaia_database_t, internal::gaia_database, internal::gaia_databaseT> {
    friend class edc_object_t<c_gaia_type_gaia_database, gaia_database_t, internal::gaia_database, internal::gaia_databaseT>;
public:
    typedef gaia::direct_access::reference_chain_container_t<gaia_table_t> gaia_tables_list_t;
    gaia_database_t() : edc_object_t() {}
    static const char* gaia_typename();
    static gaia::common::gaia_id_t insert_row(const char* name);
    static gaia::direct_access::edc_container_t<c_gaia_type_gaia_database, gaia_database_t> list();
    const char* name() const;
    gaia_tables_list_t gaia_tables() const;

    template<class unused_t>
    struct expr_ {
        static gaia::direct_access::expression_t<gaia_database_t, gaia::common::gaia_id_t> gaia_id;
        static gaia::direct_access::expression_t<gaia_database_t, const char*> name;
        static gaia::direct_access::expression_t<gaia_database_t, gaia_database_t::gaia_tables_list_t> gaia_tables;
    };
    using expr = expr_<void>;
private:
    explicit gaia_database_t(gaia::common::gaia_id_t id) : edc_object_t(id) {}
};

namespace gaia_database_expr {
    static auto& gaia_id = gaia_database_t::expr::gaia_id;
    static auto& name = gaia_database_t::expr::name;
    static auto& gaia_tables = gaia_database_t::expr::gaia_tables;
};

template<class unused_t> gaia::direct_access::expression_t<gaia_database_t, gaia::common::gaia_id_t> gaia_database_t::expr_<unused_t>::gaia_id{&gaia_database_t::gaia_id};
template<class unused_t> gaia::direct_access::expression_t<gaia_database_t, const char*> gaia_database_t::expr_<unused_t>::name{&gaia_database_t::name};
template<class unused_t> gaia::direct_access::expression_t<gaia_database_t, gaia_database_t::gaia_tables_list_t> gaia_database_t::expr_<unused_t>::gaia_tables{&gaia_database_t::gaia_tables};


}  // namespace catalog
}  // namespace gaia

#endif  // GAIA_GENERATED_catalog_H_


