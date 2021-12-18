/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

// Automatically generated by the Gaia Direct Access Classes code generator.
// Do not modify.

#include "gaia_catalog.h"

namespace gaia {
namespace catalog {

//
// Implementation of class rule_relationship_t.
//

const char* rule_relationship_t::gaia_typename() {
    static const char* gaia_typename = "rule_relationship_t";
    return gaia_typename;
}
gaia::common::gaia_id_t rule_relationship_t::insert_row(uint8_t type) {
    flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
    b.Finish(internal::Createrule_relationship(b, type));
    return dac_object_t::insert_row(b);
}
gaia::direct_access::dac_container_t<c_gaia_type_rule_relationship, rule_relationship_t> rule_relationship_t::list() {
    return gaia::direct_access::dac_container_t<c_gaia_type_rule_relationship, rule_relationship_t>();
}
uint8_t rule_relationship_t::type() const {
    return GET(type);
}
gaia_relationship_t rule_relationship_t::relationship() const {
    gaia::common::gaia_id_t id = this->references()[c_rule_relationship_parent_relationship];
    return (id == gaia::common::c_invalid_gaia_id) ? gaia_relationship_t() : gaia_relationship_t::get(id);
}
gaia_rule_t rule_relationship_t::rule() const {
    gaia::common::gaia_id_t id = this->references()[c_rule_relationship_parent_rule];
    return (id == gaia::common::c_invalid_gaia_id) ? gaia_rule_t() : gaia_rule_t::get(id);
}
//
// Implementation of class rule_field_t.
//

const char* rule_field_t::gaia_typename() {
    static const char* gaia_typename = "rule_field_t";
    return gaia_typename;
}
gaia::common::gaia_id_t rule_field_t::insert_row(uint8_t type, bool active) {
    flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
    b.Finish(internal::Createrule_field(b, type, active));
    return dac_object_t::insert_row(b);
}
gaia::direct_access::dac_container_t<c_gaia_type_rule_field, rule_field_t> rule_field_t::list() {
    return gaia::direct_access::dac_container_t<c_gaia_type_rule_field, rule_field_t>();
}
uint8_t rule_field_t::type() const {
    return GET(type);
}
bool rule_field_t::active() const {
    return GET(active);
}
gaia_field_t rule_field_t::field() const {
    gaia::common::gaia_id_t id = this->references()[c_rule_field_parent_field];
    return (id == gaia::common::c_invalid_gaia_id) ? gaia_field_t() : gaia_field_t::get(id);
}
gaia_rule_t rule_field_t::rule() const {
    gaia::common::gaia_id_t id = this->references()[c_rule_field_parent_rule];
    return (id == gaia::common::c_invalid_gaia_id) ? gaia_rule_t() : gaia_rule_t::get(id);
}
//
// Implementation of class rule_table_t.
//

const char* rule_table_t::gaia_typename() {
    static const char* gaia_typename = "rule_table_t";
    return gaia_typename;
}
gaia::common::gaia_id_t rule_table_t::insert_row(uint8_t type, bool anchor) {
    flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
    b.Finish(internal::Createrule_table(b, type, anchor));
    return dac_object_t::insert_row(b);
}
gaia::direct_access::dac_container_t<c_gaia_type_rule_table, rule_table_t> rule_table_t::list() {
    return gaia::direct_access::dac_container_t<c_gaia_type_rule_table, rule_table_t>();
}
uint8_t rule_table_t::type() const {
    return GET(type);
}
bool rule_table_t::anchor() const {
    return GET(anchor);
}
gaia_table_t rule_table_t::table() const {
    gaia::common::gaia_id_t id = this->references()[c_rule_table_parent_table];
    return (id == gaia::common::c_invalid_gaia_id) ? gaia_table_t() : gaia_table_t::get(id);
}
gaia_rule_t rule_table_t::rule() const {
    gaia::common::gaia_id_t id = this->references()[c_rule_table_parent_rule];
    return (id == gaia::common::c_invalid_gaia_id) ? gaia_rule_t() : gaia_rule_t::get(id);
}
//
// Implementation of class ruleset_database_t.
//

const char* ruleset_database_t::gaia_typename() {
    static const char* gaia_typename = "ruleset_database_t";
    return gaia_typename;
}
gaia::common::gaia_id_t ruleset_database_t::insert_row() {
    flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
    b.Finish(internal::Createruleset_database(b));
    return dac_object_t::insert_row(b);
}
gaia::direct_access::dac_container_t<c_gaia_type_ruleset_database, ruleset_database_t> ruleset_database_t::list() {
    return gaia::direct_access::dac_container_t<c_gaia_type_ruleset_database, ruleset_database_t>();
}
gaia_ruleset_t ruleset_database_t::ruleset() const {
    gaia::common::gaia_id_t id = this->references()[c_ruleset_database_parent_ruleset];
    return (id == gaia::common::c_invalid_gaia_id) ? gaia_ruleset_t() : gaia_ruleset_t::get(id);
}
gaia_database_t ruleset_database_t::database() const {
    gaia::common::gaia_id_t id = this->references()[c_ruleset_database_parent_database];
    return (id == gaia::common::c_invalid_gaia_id) ? gaia_database_t() : gaia_database_t::get(id);
}
//
// Implementation of class app_ruleset_t.
//

const char* app_ruleset_t::gaia_typename() {
    static const char* gaia_typename = "app_ruleset_t";
    return gaia_typename;
}
gaia::common::gaia_id_t app_ruleset_t::insert_row(bool active_on_startup) {
    flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
    b.Finish(internal::Createapp_ruleset(b, active_on_startup));
    return dac_object_t::insert_row(b);
}
gaia::direct_access::dac_container_t<c_gaia_type_app_ruleset, app_ruleset_t> app_ruleset_t::list() {
    return gaia::direct_access::dac_container_t<c_gaia_type_app_ruleset, app_ruleset_t>();
}
bool app_ruleset_t::active_on_startup() const {
    return GET(active_on_startup);
}
gaia_ruleset_t app_ruleset_t::ruleset() const {
    gaia::common::gaia_id_t id = this->references()[c_app_ruleset_parent_ruleset];
    return (id == gaia::common::c_invalid_gaia_id) ? gaia_ruleset_t() : gaia_ruleset_t::get(id);
}
gaia_application_t app_ruleset_t::application() const {
    gaia::common::gaia_id_t id = this->references()[c_app_ruleset_parent_application];
    return (id == gaia::common::c_invalid_gaia_id) ? gaia_application_t() : gaia_application_t::get(id);
}
//
// Implementation of class app_database_t.
//

const char* app_database_t::gaia_typename() {
    static const char* gaia_typename = "app_database_t";
    return gaia_typename;
}
gaia::common::gaia_id_t app_database_t::insert_row() {
    flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
    b.Finish(internal::Createapp_database(b));
    return dac_object_t::insert_row(b);
}
gaia::direct_access::dac_container_t<c_gaia_type_app_database, app_database_t> app_database_t::list() {
    return gaia::direct_access::dac_container_t<c_gaia_type_app_database, app_database_t>();
}
gaia_database_t app_database_t::database() const {
    gaia::common::gaia_id_t id = this->references()[c_app_database_parent_database];
    return (id == gaia::common::c_invalid_gaia_id) ? gaia_database_t() : gaia_database_t::get(id);
}
gaia_application_t app_database_t::application() const {
    gaia::common::gaia_id_t id = this->references()[c_app_database_parent_application];
    return (id == gaia::common::c_invalid_gaia_id) ? gaia_application_t() : gaia_application_t::get(id);
}
//
// Implementation of class gaia_application_t.
//

const char* gaia_application_t::gaia_typename() {
    static const char* gaia_typename = "gaia_application_t";
    return gaia_typename;
}
gaia::common::gaia_id_t gaia_application_t::insert_row(const char* name) {
    flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
    b.Finish(internal::Creategaia_applicationDirect(b, name));
    return dac_object_t::insert_row(b);
}
gaia::direct_access::dac_container_t<c_gaia_type_gaia_application, gaia_application_t> gaia_application_t::list() {
    return gaia::direct_access::dac_container_t<c_gaia_type_gaia_application, gaia_application_t>();
}
const char* gaia_application_t::name() const {
    return GET_STR(name);
}
gaia_application_t::app_rulesets_list_t gaia_application_t::app_rulesets() const {
    return gaia_application_t::app_rulesets_list_t(gaia_id(), c_gaia_application_first_app_rulesets, c_app_ruleset_next_application);
}
gaia_application_t::app_databases_list_t gaia_application_t::app_databases() const {
    return gaia_application_t::app_databases_list_t(gaia_id(), c_gaia_application_first_app_databases, c_app_database_next_application);
}
//
// Implementation of class gaia_index_t.
//

const char* gaia_index_t::gaia_typename() {
    static const char* gaia_typename = "gaia_index_t";
    return gaia_typename;
}
gaia::common::gaia_id_t gaia_index_t::insert_row(const char* name, bool unique, uint8_t type, const std::vector<uint64_t>& fields) {
    flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
    b.Finish(internal::Creategaia_indexDirect(b, name, unique, type, &fields));
    return dac_object_t::insert_row(b);
}
gaia::direct_access::dac_container_t<c_gaia_type_gaia_index, gaia_index_t> gaia_index_t::list() {
    return gaia::direct_access::dac_container_t<c_gaia_type_gaia_index, gaia_index_t>();
}
const char* gaia_index_t::name() const {
    return GET_STR(name);
}
bool gaia_index_t::unique() const {
    return GET(unique);
}
uint8_t gaia_index_t::type() const {
    return GET(type);
}
gaia::direct_access::dac_vector_t<uint64_t> gaia_index_t::fields() const {
    return GET_ARRAY(fields);
}
gaia_table_t gaia_index_t::table() const {
    gaia::common::gaia_id_t id = this->references()[c_gaia_index_parent_table];
    return (id == gaia::common::c_invalid_gaia_id) ? gaia_table_t() : gaia_table_t::get(id);
}
//
// Implementation of class gaia_rule_t.
//

const char* gaia_rule_t::gaia_typename() {
    static const char* gaia_typename = "gaia_rule_t";
    return gaia_typename;
}
gaia::common::gaia_id_t gaia_rule_t::insert_row(const char* name) {
    flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
    b.Finish(internal::Creategaia_ruleDirect(b, name));
    return dac_object_t::insert_row(b);
}
gaia::direct_access::dac_container_t<c_gaia_type_gaia_rule, gaia_rule_t> gaia_rule_t::list() {
    return gaia::direct_access::dac_container_t<c_gaia_type_gaia_rule, gaia_rule_t>();
}
const char* gaia_rule_t::name() const {
    return GET_STR(name);
}
gaia_ruleset_t gaia_rule_t::ruleset() const {
    gaia::common::gaia_id_t id = this->references()[c_gaia_rule_parent_ruleset];
    return (id == gaia::common::c_invalid_gaia_id) ? gaia_ruleset_t() : gaia_ruleset_t::get(id);
}
gaia_rule_t::rule_relationships_list_t gaia_rule_t::rule_relationships() const {
    return gaia_rule_t::rule_relationships_list_t(gaia_id(), c_gaia_rule_first_rule_relationships, c_rule_relationship_next_rule);
}
gaia_rule_t::rule_fields_list_t gaia_rule_t::rule_fields() const {
    return gaia_rule_t::rule_fields_list_t(gaia_id(), c_gaia_rule_first_rule_fields, c_rule_field_next_rule);
}
gaia_rule_t::rule_tables_list_t gaia_rule_t::rule_tables() const {
    return gaia_rule_t::rule_tables_list_t(gaia_id(), c_gaia_rule_first_rule_tables, c_rule_table_next_rule);
}
//
// Implementation of class gaia_ruleset_t.
//

const char* gaia_ruleset_t::gaia_typename() {
    static const char* gaia_typename = "gaia_ruleset_t";
    return gaia_typename;
}
gaia::common::gaia_id_t gaia_ruleset_t::insert_row(const char* name, const char* serial_stream) {
    flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
    b.Finish(internal::Creategaia_rulesetDirect(b, name, serial_stream));
    return dac_object_t::insert_row(b);
}
gaia::direct_access::dac_container_t<c_gaia_type_gaia_ruleset, gaia_ruleset_t> gaia_ruleset_t::list() {
    return gaia::direct_access::dac_container_t<c_gaia_type_gaia_ruleset, gaia_ruleset_t>();
}
const char* gaia_ruleset_t::name() const {
    return GET_STR(name);
}
const char* gaia_ruleset_t::serial_stream() const {
    return GET_STR(serial_stream);
}
gaia_ruleset_t::ruleset_databases_list_t gaia_ruleset_t::ruleset_databases() const {
    return gaia_ruleset_t::ruleset_databases_list_t(gaia_id(), c_gaia_ruleset_first_ruleset_databases, c_ruleset_database_next_ruleset);
}
gaia_ruleset_t::app_rulesets_list_t gaia_ruleset_t::app_rulesets() const {
    return gaia_ruleset_t::app_rulesets_list_t(gaia_id(), c_gaia_ruleset_first_app_rulesets, c_app_ruleset_next_ruleset);
}
gaia_ruleset_t::gaia_rules_list_t gaia_ruleset_t::gaia_rules() const {
    return gaia_ruleset_t::gaia_rules_list_t(gaia_id(), c_gaia_ruleset_first_gaia_rules, c_gaia_rule_next_ruleset);
}
//
// Implementation of class gaia_relationship_t.
//

const char* gaia_relationship_t::gaia_typename() {
    static const char* gaia_typename = "gaia_relationship_t";
    return gaia_typename;
}
gaia::common::gaia_id_t gaia_relationship_t::insert_row(const char* name, const char* to_parent_link_name, const char* to_child_link_name, uint8_t cardinality, bool parent_required, bool deprecated, uint16_t first_child_offset, uint16_t next_child_offset, uint16_t prev_child_offset, uint16_t parent_offset, const std::vector<uint16_t>& parent_field_positions, const std::vector<uint16_t>& child_field_positions, const char* hash) {
    flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
    b.Finish(internal::Creategaia_relationshipDirect(b, name, to_parent_link_name, to_child_link_name, cardinality, parent_required, deprecated, first_child_offset, next_child_offset, prev_child_offset, parent_offset, &parent_field_positions, &child_field_positions, hash));
    return dac_object_t::insert_row(b);
}
gaia::direct_access::dac_container_t<c_gaia_type_gaia_relationship, gaia_relationship_t> gaia_relationship_t::list() {
    return gaia::direct_access::dac_container_t<c_gaia_type_gaia_relationship, gaia_relationship_t>();
}
const char* gaia_relationship_t::name() const {
    return GET_STR(name);
}
const char* gaia_relationship_t::to_parent_link_name() const {
    return GET_STR(to_parent_link_name);
}
const char* gaia_relationship_t::to_child_link_name() const {
    return GET_STR(to_child_link_name);
}
uint8_t gaia_relationship_t::cardinality() const {
    return GET(cardinality);
}
bool gaia_relationship_t::parent_required() const {
    return GET(parent_required);
}
bool gaia_relationship_t::deprecated() const {
    return GET(deprecated);
}
uint16_t gaia_relationship_t::first_child_offset() const {
    return GET(first_child_offset);
}
uint16_t gaia_relationship_t::next_child_offset() const {
    return GET(next_child_offset);
}
uint16_t gaia_relationship_t::prev_child_offset() const {
    return GET(prev_child_offset);
}
uint16_t gaia_relationship_t::parent_offset() const {
    return GET(parent_offset);
}
gaia::direct_access::dac_vector_t<uint16_t> gaia_relationship_t::parent_field_positions() const {
    return GET_ARRAY(parent_field_positions);
}
gaia::direct_access::dac_vector_t<uint16_t> gaia_relationship_t::child_field_positions() const {
    return GET_ARRAY(child_field_positions);
}
const char* gaia_relationship_t::hash() const {
    return GET_STR(hash);
}
gaia_table_t gaia_relationship_t::child() const {
    gaia::common::gaia_id_t id = this->references()[c_gaia_relationship_parent_child];
    return (id == gaia::common::c_invalid_gaia_id) ? gaia_table_t() : gaia_table_t::get(id);
}
gaia_table_t gaia_relationship_t::parent() const {
    gaia::common::gaia_id_t id = this->references()[c_gaia_relationship_parent_parent];
    return (id == gaia::common::c_invalid_gaia_id) ? gaia_table_t() : gaia_table_t::get(id);
}
gaia_relationship_t::rule_relationships_list_t gaia_relationship_t::rule_relationships() const {
    return gaia_relationship_t::rule_relationships_list_t(gaia_id(), c_gaia_relationship_first_rule_relationships, c_rule_relationship_next_relationship);
}
//
// Implementation of class gaia_field_t.
//

const char* gaia_field_t::gaia_typename() {
    static const char* gaia_typename = "gaia_field_t";
    return gaia_typename;
}
gaia::common::gaia_id_t gaia_field_t::insert_row(const char* name, uint8_t type, uint16_t repeated_count, uint16_t position, bool deprecated, bool active, bool unique, const char* hash) {
    flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
    b.Finish(internal::Creategaia_fieldDirect(b, name, type, repeated_count, position, deprecated, active, unique, hash));
    return dac_object_t::insert_row(b);
}
gaia::direct_access::dac_container_t<c_gaia_type_gaia_field, gaia_field_t> gaia_field_t::list() {
    return gaia::direct_access::dac_container_t<c_gaia_type_gaia_field, gaia_field_t>();
}
const char* gaia_field_t::name() const {
    return GET_STR(name);
}
uint8_t gaia_field_t::type() const {
    return GET(type);
}
uint16_t gaia_field_t::repeated_count() const {
    return GET(repeated_count);
}
uint16_t gaia_field_t::position() const {
    return GET(position);
}
bool gaia_field_t::deprecated() const {
    return GET(deprecated);
}
bool gaia_field_t::active() const {
    return GET(active);
}
bool gaia_field_t::unique() const {
    return GET(unique);
}
const char* gaia_field_t::hash() const {
    return GET_STR(hash);
}
gaia_table_t gaia_field_t::table() const {
    gaia::common::gaia_id_t id = this->references()[c_gaia_field_parent_table];
    return (id == gaia::common::c_invalid_gaia_id) ? gaia_table_t() : gaia_table_t::get(id);
}
gaia_field_t::rule_fields_list_t gaia_field_t::rule_fields() const {
    return gaia_field_t::rule_fields_list_t(gaia_id(), c_gaia_field_first_rule_fields, c_rule_field_next_field);
}
//
// Implementation of class gaia_table_t.
//

const char* gaia_table_t::gaia_typename() {
    static const char* gaia_typename = "gaia_table_t";
    return gaia_typename;
}
gaia::common::gaia_id_t gaia_table_t::insert_row(const char* name, uint32_t type, bool is_system, const std::vector<uint8_t>& binary_schema, const std::vector<uint8_t>& serialization_template, const char* hash) {
    flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
    b.Finish(internal::Creategaia_tableDirect(b, name, type, is_system, &binary_schema, &serialization_template, hash));
    return dac_object_t::insert_row(b);
}
gaia::direct_access::dac_container_t<c_gaia_type_gaia_table, gaia_table_t> gaia_table_t::list() {
    return gaia::direct_access::dac_container_t<c_gaia_type_gaia_table, gaia_table_t>();
}
const char* gaia_table_t::name() const {
    return GET_STR(name);
}
uint32_t gaia_table_t::type() const {
    return GET(type);
}
bool gaia_table_t::is_system() const {
    return GET(is_system);
}
gaia::direct_access::dac_vector_t<uint8_t> gaia_table_t::binary_schema() const {
    return GET_ARRAY(binary_schema);
}
gaia::direct_access::dac_vector_t<uint8_t> gaia_table_t::serialization_template() const {
    return GET_ARRAY(serialization_template);
}
const char* gaia_table_t::hash() const {
    return GET_STR(hash);
}
gaia_database_t gaia_table_t::database() const {
    gaia::common::gaia_id_t id = this->references()[c_gaia_table_parent_database];
    return (id == gaia::common::c_invalid_gaia_id) ? gaia_database_t() : gaia_database_t::get(id);
}
gaia_table_t::rule_tables_list_t gaia_table_t::rule_tables() const {
    return gaia_table_t::rule_tables_list_t(gaia_id(), c_gaia_table_first_rule_tables, c_rule_table_next_table);
}
gaia_table_t::gaia_indexes_list_t gaia_table_t::gaia_indexes() const {
    return gaia_table_t::gaia_indexes_list_t(gaia_id(), c_gaia_table_first_gaia_indexes, c_gaia_index_next_table);
}
gaia_table_t::incoming_relationships_list_t gaia_table_t::incoming_relationships() const {
    return gaia_table_t::incoming_relationships_list_t(gaia_id(), c_gaia_table_first_incoming_relationships, c_gaia_relationship_next_child);
}
gaia_table_t::outgoing_relationships_list_t gaia_table_t::outgoing_relationships() const {
    return gaia_table_t::outgoing_relationships_list_t(gaia_id(), c_gaia_table_first_outgoing_relationships, c_gaia_relationship_next_parent);
}
gaia_table_t::gaia_fields_list_t gaia_table_t::gaia_fields() const {
    return gaia_table_t::gaia_fields_list_t(gaia_id(), c_gaia_table_first_gaia_fields, c_gaia_field_next_table);
}
//
// Implementation of class gaia_database_t.
//

const char* gaia_database_t::gaia_typename() {
    static const char* gaia_typename = "gaia_database_t";
    return gaia_typename;
}
gaia::common::gaia_id_t gaia_database_t::insert_row(const char* name, const char* hash) {
    flatbuffers::FlatBufferBuilder b(c_flatbuffer_builder_size);
    b.Finish(internal::Creategaia_databaseDirect(b, name, hash));
    return dac_object_t::insert_row(b);
}
gaia::direct_access::dac_container_t<c_gaia_type_gaia_database, gaia_database_t> gaia_database_t::list() {
    return gaia::direct_access::dac_container_t<c_gaia_type_gaia_database, gaia_database_t>();
}
const char* gaia_database_t::name() const {
    return GET_STR(name);
}
const char* gaia_database_t::hash() const {
    return GET_STR(hash);
}
gaia_database_t::ruleset_databases_list_t gaia_database_t::ruleset_databases() const {
    return gaia_database_t::ruleset_databases_list_t(gaia_id(), c_gaia_database_first_ruleset_databases, c_ruleset_database_next_database);
}
gaia_database_t::app_databases_list_t gaia_database_t::app_databases() const {
    return gaia_database_t::app_databases_list_t(gaia_id(), c_gaia_database_first_app_databases, c_app_database_next_database);
}
gaia_database_t::gaia_tables_list_t gaia_database_t::gaia_tables() const {
    return gaia_database_t::gaia_tables_list_t(gaia_id(), c_gaia_database_first_gaia_tables, c_gaia_table_next_database);
}
}  // namespace catalog
}  // namespace gaia


