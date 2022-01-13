/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string>
#include <vector>

#include <flatbuffers/flatbuffers.h>

#include "gaia/common.hpp"
#include "gaia/db/db.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/common/generator_iterator.hpp"
#include "gaia_internal/common/system_table_types.hpp"
#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/db_types.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"

namespace gaia
{
namespace db
{

struct catalog_db_object_view_t
{
    explicit catalog_db_object_view_t(const db_object_t* obj_ptr)
        : m_obj_ptr{obj_ptr}
    {
    }

    [[nodiscard]] common::gaia_id_t id() const
    {
        return m_obj_ptr->id;
    }

    [[nodiscard]] common::gaia_type_t type() const
    {
        return m_obj_ptr->type;
    }

protected:
    const db_object_t* m_obj_ptr;
};

struct field_view_t : catalog_db_object_view_t
{
    using catalog_db_object_view_t::catalog_db_object_view_t;
    [[nodiscard]] const char* name() const;
    [[nodiscard]] common::data_type_t data_type() const;
    [[nodiscard]] common::field_position_t position() const;
};

using buffer = const flatbuffers::Vector<uint8_t>;

struct table_view_t : catalog_db_object_view_t
{
    using catalog_db_object_view_t::catalog_db_object_view_t;
    [[nodiscard]] const char* name() const;
    [[nodiscard]] common::gaia_type_t table_type() const;
    [[nodiscard]] buffer* binary_schema() const;
    [[nodiscard]] buffer* serialization_template() const;
};

struct relationship_view_t : catalog_db_object_view_t
{
    static constexpr common::reference_offset_t c_parent_gaia_table_ref_offset = 0;
    static constexpr common::reference_offset_t c_child_gaia_table_ref_offset = 2;

    using catalog_db_object_view_t::catalog_db_object_view_t;
    [[nodiscard]] const char* name() const;
    [[nodiscard]] const char* to_parent_link_name() const;
    [[nodiscard]] const char* to_child_link_name() const;
    [[nodiscard]] common::gaia_id_t parent_table_id() const;
    [[nodiscard]] common::gaia_id_t child_table_id() const;
    [[nodiscard]] common::reference_offset_t first_child_offset() const;
    [[nodiscard]] common::reference_offset_t next_child_offset() const;
    [[nodiscard]] common::reference_offset_t prev_child_offset() const;
    [[nodiscard]] common::reference_offset_t parent_offset() const;
    [[nodiscard]] const flatbuffers::Vector<uint16_t>* parent_field_positions() const;
    [[nodiscard]] const flatbuffers::Vector<uint16_t>* child_field_positions() const;
    [[nodiscard]] bool is_value_linked() const;
};

struct index_view_t : catalog_db_object_view_t
{
    static constexpr common::reference_offset_t c_parent_table_ref_offset = 0;

    using catalog_db_object_view_t::catalog_db_object_view_t;
    [[nodiscard]] const char* name() const;
    [[nodiscard]] bool unique() const;
    [[nodiscard]] catalog::index_type_t type() const;
    [[nodiscard]] const flatbuffers::Vector<common::gaia_id_t>* fields() const;
    [[nodiscard]] common::gaia_id_t table_id() const;
};

using field_list_t = common::iterators::generator_range_t<field_view_t>;
using table_list_t = common::iterators::generator_range_t<table_view_t>;
using relationship_list_t = common::iterators::generator_range_t<relationship_view_t>;
using index_list_t = common::iterators::generator_range_t<index_view_t>;

struct catalog_core_t
{
    // Constants for reference slots in catalog records.
    // They need to be updated when the corresponding catalog table definition change.
    //
    // The gaia_field reference slots:
    //    parent gaia_table
    //    next gaia_field
    //    first rule_field
    static constexpr common::reference_offset_t c_gaia_field_parent_table = 0;
    static constexpr common::reference_offset_t c_gaia_field_next_table = 1;
    static constexpr common::reference_offset_t c_gaia_field_first_rule_fields = 2;
    //
    // The gaia_table reference slots:
    //    parent gaia_database
    //    next gaia_table
    //    first gaia_field
    //    first gaia_relationship (outgoing)
    //    first gaia_relationship (incoming)
    //    first gaia_index
    //    first rule_table
    static constexpr common::reference_offset_t c_gaia_table_parent_database = 0;
    static constexpr common::reference_offset_t c_gaia_table_next_database = 1;
    static constexpr common::reference_offset_t c_gaia_table_first_gaia_fields = 2;
    static constexpr common::reference_offset_t c_gaia_table_first_outgoing_relationships = 3;
    static constexpr common::reference_offset_t c_gaia_table_first_incoming_relationships = 4;
    static constexpr common::reference_offset_t c_gaia_table_first_gaia_indexes = 5;
    static constexpr common::reference_offset_t c_gaia_table_first_rule_tables = 6;
    //
    // The gaia_relationship reference slots:
    //    parent gaia_table (outgoing)
    //    next gaia_relationship (outgoing)
    //    parent gaia_relationship (incoming)
    //    next gaia_relationship (incoming)
    //    first rule_relationship
    static constexpr common::reference_offset_t c_gaia_relationship_parent_parent = 0;
    static constexpr common::reference_offset_t c_gaia_relationship_next_parent = 1;
    static constexpr common::reference_offset_t c_gaia_relationship_parent_child = 2;
    static constexpr common::reference_offset_t c_gaia_relationship_next_child = 3;
    static constexpr common::reference_offset_t c_gaia_relationship_first_rule_relationships = 4;
    //
    // The gaia_database reference slots:
    //    first gaia_table
    //    first app_database
    //    first ruleset_database
    static constexpr common::reference_offset_t c_gaia_database_first_gaia_tables = 0;
    static constexpr common::reference_offset_t c_gaia_database_first_app_databases = 1;
    static constexpr common::reference_offset_t c_gaia_database_first_ruleset_databases = 2;
    //
    // The gaia_ruleset reference slots:
    //    first gaia_rule
    //    first app_ruleset
    //    first ruleset_database
    static constexpr common::reference_offset_t c_gaia_ruleset_first_gaia_rules = 0;
    static constexpr common::reference_offset_t c_gaia_ruleset_first_app_rulesets = 1;
    static constexpr common::reference_offset_t c_gaia_ruleset_first_ruleset_databases = 2;
    //
    // The gaia_rule reference slots:
    //    parent gaia_ruleset
    //    next gaia_rule
    //    first rule_table
    //    first rule_field
    //    first rule_relationship
    static constexpr common::reference_offset_t c_gaia_rule_parent_ruleset = 0;
    static constexpr common::reference_offset_t c_gaia_rule_next_ruleset = 1;
    static constexpr common::reference_offset_t c_gaia_rule_first_rule_tables = 2;
    static constexpr common::reference_offset_t c_gaia_rule_first_rule_fields = 3;
    static constexpr common::reference_offset_t c_gaia_rule_first_rule_relationships = 4;
    //
    // The gaia_index reference slots:
    //    parent gaia_table
    //    next gaia_index
    static constexpr common::reference_offset_t c_gaia_index_parent_table = 0;
    static constexpr common::reference_offset_t c_gaia_index_next_table = 1;

    // The rule_relationship reference slots:
    //    parent gaia_rule
    //    next rule_relationship
    //    parent gaia_relationship
    //    next rule_relationship
    static constexpr common::reference_offset_t c_rule_relationship_parent_rule = 0;
    static constexpr common::reference_offset_t c_rule_relationship_next_rule = 1;
    static constexpr common::reference_offset_t c_rule_relationship_parent_relationship = 2;
    static constexpr common::reference_offset_t c_rule_relationship_next_relationship = 3;

    // The rule_field reference slots:
    //    parent gaia_rule
    //    next rule_field
    //    parent gaia_field
    //    next rule_field
    static constexpr common::reference_offset_t c_rule_field_parent_rule = 0;
    static constexpr common::reference_offset_t c_rule_field_next_rule = 1;
    static constexpr common::reference_offset_t c_rule_field_parent_field = 2;
    static constexpr common::reference_offset_t c_rule_field_next_field = 3;

    // The rule_table reference slots:
    //    parent gaia_rule
    //    next rule_table
    //    parent gaia_table
    //    next rule_table
    static constexpr common::reference_offset_t c_rule_table_parent_rule = 0;
    static constexpr common::reference_offset_t c_rule_table_next_rule = 1;
    static constexpr common::reference_offset_t c_rule_table_parent_table = 2;
    static constexpr common::reference_offset_t c_rule_table_next_table = 3;

    // The ruleset_database reference slots:
    //    parent gaia_database
    //    next ruleset_database
    //    parent gaia_ruleset
    //    next ruleset_database
    static constexpr common::reference_offset_t c_ruleset_database_parent_database = 0;
    static constexpr common::reference_offset_t c_ruleset_database_next_database = 1;
    static constexpr common::reference_offset_t c_ruleset_database_parent_ruleset = 2;
    static constexpr common::reference_offset_t c_ruleset_database_next_ruleset = 3;

    // The app_ruleset reference slots:
    //    parent gaia_application
    //    next app_ruleset
    //    parent gaia_ruleset
    //    next app_ruleset
    static constexpr common::reference_offset_t c_app_ruleset_parent_application = 0;
    static constexpr common::reference_offset_t c_app_ruleset_next_application = 1;
    static constexpr common::reference_offset_t c_app_ruleset_parent_ruleset = 2;
    static constexpr common::reference_offset_t c_app_ruleset_next_ruleset = 3;

    // The app_database reference slots:
    //    parent gaia_application
    //    next app_database
    //    parent gaia_database
    //    next app_database
    static constexpr common::reference_offset_t c_app_database_parent_application = 0;
    static constexpr common::reference_offset_t c_app_database_next_application = 1;
    static constexpr common::reference_offset_t c_app_database_parent_database = 2;
    static constexpr common::reference_offset_t c_app_database_next_database = 3;

    // The gaia_application reference slots:
    //    first app_database
    //    first app_ruleset
    static constexpr common::reference_offset_t c_gaia_application_first_app_databases = 0;
    static constexpr common::reference_offset_t c_gaia_application_first_app_rulesets = 1;

    [[nodiscard]] static inline const db_object_t* get_db_object_ptr(common::gaia_id_t);

    static table_view_t get_table(common::gaia_id_t table_id);
    static table_list_t list_tables();
    static field_list_t list_fields(common::gaia_id_t table_id);

    // List all the relationship(s) originating from the given table.
    static relationship_list_t list_relationship_from(common::gaia_id_t table_id);
    // List all the relationship(s) pointing to the given table.
    static relationship_list_t list_relationship_to(common::gaia_id_t table_id);

    static index_list_t list_indexes(common::gaia_id_t table_id);

    // TODO: Decide if the method belongs to catalog_core or index headers.
    // Helper method to find an index for a given table and field. Return
    // invalid_gaia_id if cannot find index (for the field).
    static common::gaia_id_t find_index(common::gaia_id_t table_id, common::field_position_t field_position);
};

class table_generator_t : public common::iterators::generator_t<table_view_t>
{
public:
    explicit table_generator_t(common::iterators::generator_iterator_t<gaia_ptr_t>&& iterator);

    std::optional<table_view_t> operator()() final;

private:
    common::iterators::generator_iterator_t<gaia_ptr_t> m_gaia_ptr_iterator;
};

} // namespace db
} // namespace gaia
