/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gaia_internal/db/type_metadata.hpp"

#include <optional>
#include <shared_mutex>

#include "gaia_internal/common/logger.hpp"
#include "gaia_internal/common/system_table_types.hpp"
#include "gaia_internal/db/catalog_core.hpp"
#include "gaia_internal/exceptions.hpp"

#include "type_id_mapping.hpp"

using namespace gaia::common;

namespace gaia
{
namespace db
{

/*
 * type_metadata_t
 */

std::optional<relationship_t> type_metadata_t::find_parent_relationship(reference_offset_t first_child_offset) const
{
    std::shared_lock lock(m_metadata_lock);

    const auto& parent_relationship = m_parent_relationships.find(first_child_offset);

    if (parent_relationship == m_parent_relationships.end())
    {
        return std::nullopt;
    }

    return *parent_relationship->second.get();
}

std::optional<relationship_t> type_metadata_t::find_child_relationship(reference_offset_t parent_offset) const
{
    std::shared_lock lock(m_metadata_lock);

    auto child_relationship = m_child_relationships.find(parent_offset);

    if (child_relationship == m_child_relationships.end())
    {
        return std::nullopt;
    }

    return *child_relationship->second.get();
}

void type_metadata_t::add_parent_relationship(const std::shared_ptr<relationship_t>& relationship)
{
    std::unique_lock lock(m_metadata_lock);

    m_parent_relationships.insert({relationship->first_child_offset, relationship});
    m_reference_count += 1;
}

void type_metadata_t::add_child_relationship(const std::shared_ptr<relationship_t>& relationship)
{
    std::unique_lock lock(m_metadata_lock);

    m_child_relationships.insert({relationship->parent_offset, relationship});

    // A child node contains 2 ref slots for every relationship.
    constexpr std::size_t c_num_ref_slots = 2;
    // A child node contains 3 ref slots for every value linked relationship.
    constexpr std::size_t c_value_linked_rel_num_ref_slots = 3;

    if (relationship->value_linked)
    {
        m_reference_count += c_value_linked_rel_num_ref_slots;
    }
    else
    {
        m_reference_count += c_num_ref_slots;
    }
}

gaia_type_t type_metadata_t::get_type() const
{
    return m_type;
}

reference_offset_t type_metadata_t::num_references() const
{
    std::shared_lock lock(m_metadata_lock);
    return m_reference_count;
}

bool type_metadata_t::is_initialized()
{
    return m_initialized;
}

void type_metadata_t::mark_as_initialized()
{
    m_initialized.store(true);
}

/*
 * type_registry_t
 */

void type_registry_t::init()
{
    // The catalog tables are created in ddl_executor for build time bootstrap.
    // The type registry is initialized before ddl_executor at runtime upon database startup.
    // The following initialization step is needed before we have a proper runtime bootstrap of the catalog.
    // TODO: runtime bootstrap of the catalog.
    auto gaia_database = static_cast<gaia_type_t::value_type>(catalog_table_type_t::gaia_database);
    auto gaia_table = static_cast<gaia_type_t::value_type>(catalog_table_type_t::gaia_table);
    auto gaia_field = static_cast<gaia_type_t::value_type>(catalog_table_type_t::gaia_field);
    auto gaia_relationship = static_cast<gaia_type_t::value_type>(catalog_table_type_t::gaia_relationship);
    auto gaia_rule = static_cast<gaia_type_t::value_type>(catalog_table_type_t::gaia_rule);
    auto gaia_ruleset = static_cast<gaia_type_t::value_type>(catalog_table_type_t::gaia_ruleset);
    auto gaia_index = static_cast<gaia_type_t::value_type>(catalog_table_type_t::gaia_index);
    auto gaia_application = static_cast<gaia_type_t::value_type>(catalog_table_type_t::gaia_application);
    auto app_database = static_cast<gaia_type_t::value_type>(catalog_table_type_t::app_database);
    auto app_ruleset = static_cast<gaia_type_t::value_type>(catalog_table_type_t::app_ruleset);
    auto ruleset_database = static_cast<gaia_type_t::value_type>(catalog_table_type_t::ruleset_database);
    auto rule_table = static_cast<gaia_type_t::value_type>(catalog_table_type_t::rule_table);
    auto rule_field = static_cast<gaia_type_t::value_type>(catalog_table_type_t::rule_field);
    auto rule_relationship = static_cast<gaia_type_t::value_type>(catalog_table_type_t::rule_relationship);

    auto gaia_database_gaia_table = std::make_shared<relationship_t>(relationship_t{
        .parent_type = gaia_database,
        .child_type = gaia_table,
        .first_child_offset = catalog_core_t::c_gaia_database_first_gaia_tables,
        .next_child_offset = catalog_core_t::c_gaia_table_next_database,
        .prev_child_offset = c_invalid_reference_offset,
        .parent_offset = catalog_core_t::c_gaia_table_parent_database,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto gaia_table_gaia_field = std::make_shared<relationship_t>(relationship_t{
        .parent_type = gaia_table,
        .child_type = gaia_field,
        .first_child_offset = catalog_core_t::c_gaia_table_first_gaia_fields,
        .next_child_offset = catalog_core_t::c_gaia_field_next_table,
        .prev_child_offset = c_invalid_reference_offset,
        .parent_offset = catalog_core_t::c_gaia_field_parent_table,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto gaia_table_gaia_relationship_parent = std::make_shared<relationship_t>(relationship_t{
        .parent_type = gaia_table,
        .child_type = gaia_relationship,
        .first_child_offset = catalog_core_t::c_gaia_table_first_outgoing_relationships,
        .next_child_offset = catalog_core_t::c_gaia_relationship_next_parent,
        .prev_child_offset = c_invalid_reference_offset,
        .parent_offset = catalog_core_t::c_gaia_relationship_parent_parent,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto gaia_table_gaia_relationship_child = std::make_shared<relationship_t>(relationship_t{
        .parent_type = gaia_table,
        .child_type = gaia_relationship,
        .first_child_offset = catalog_core_t::c_gaia_table_first_incoming_relationships,
        .next_child_offset = catalog_core_t::c_gaia_relationship_next_child,
        .prev_child_offset = c_invalid_reference_offset,
        .parent_offset = catalog_core_t::c_gaia_relationship_parent_child,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto gaia_ruleset_gaia_rule = std::make_shared<relationship_t>(relationship_t{
        .parent_type = gaia_ruleset,
        .child_type = gaia_rule,
        .first_child_offset = catalog_core_t::c_gaia_ruleset_first_gaia_rules,
        .next_child_offset = catalog_core_t::c_gaia_rule_next_ruleset,
        .prev_child_offset = c_invalid_reference_offset,
        .parent_offset = catalog_core_t::c_gaia_rule_parent_ruleset,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto gaia_table_gaia_index = std::make_shared<relationship_t>(relationship_t{
        .parent_type = gaia_table,
        .child_type = gaia_index,
        .first_child_offset = catalog_core_t::c_gaia_table_first_gaia_indexes,
        .next_child_offset = catalog_core_t::c_gaia_index_next_table,
        .prev_child_offset = c_invalid_reference_offset,
        .parent_offset = catalog_core_t::c_gaia_index_parent_table,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto gaia_rule_rule_relationship = std::make_shared<relationship_t>(relationship_t{
        .parent_type = gaia_rule,
        .child_type = rule_relationship,
        .first_child_offset = catalog_core_t::c_gaia_rule_first_rule_relationships,
        .next_child_offset = catalog_core_t::c_rule_relationship_next_rule,
        .parent_offset = catalog_core_t::c_rule_relationship_parent_rule,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto gaia_database_app_database = std::make_shared<relationship_t>(relationship_t{
        .parent_type = gaia_database,
        .child_type = app_database,
        .first_child_offset = catalog_core_t::c_gaia_database_first_app_databases,
        .next_child_offset = catalog_core_t::c_app_database_next_database,
        .prev_child_offset = c_invalid_reference_offset,
        .parent_offset = catalog_core_t::c_app_database_parent_database,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto gaia_database_ruleset_database = std::make_shared<relationship_t>(relationship_t{
        .parent_type = gaia_database,
        .child_type = ruleset_database,
        .first_child_offset = catalog_core_t::c_gaia_database_first_ruleset_databases,
        .next_child_offset = catalog_core_t::c_ruleset_database_next_database,
        .prev_child_offset = c_invalid_reference_offset,
        .parent_offset = catalog_core_t::c_ruleset_database_parent_database,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto gaia_application_app_database = std::make_shared<relationship_t>(relationship_t{
        .parent_type = gaia_application,
        .child_type = app_database,
        .first_child_offset = catalog_core_t::c_gaia_application_first_app_databases,
        .next_child_offset = catalog_core_t::c_app_database_next_application,
        .prev_child_offset = c_invalid_reference_offset,
        .parent_offset = catalog_core_t::c_app_database_parent_application,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto gaia_application_app_ruleset = std::make_shared<relationship_t>(relationship_t{
        .parent_type = gaia_application,
        .child_type = app_ruleset,
        .first_child_offset = catalog_core_t::c_gaia_application_first_app_rulesets,
        .next_child_offset = catalog_core_t::c_app_ruleset_next_application,
        .prev_child_offset = c_invalid_reference_offset,
        .parent_offset = catalog_core_t::c_app_ruleset_parent_application,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto gaia_ruleset_ruleset_database = std::make_shared<relationship_t>(relationship_t{
        .parent_type = gaia_ruleset,
        .child_type = ruleset_database,
        .first_child_offset = catalog_core_t::c_gaia_ruleset_first_ruleset_databases,
        .next_child_offset = catalog_core_t::c_ruleset_database_next_ruleset,
        .prev_child_offset = c_invalid_reference_offset,
        .parent_offset = catalog_core_t::c_ruleset_database_parent_ruleset,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto gaia_ruleset_app_ruleset = std::make_shared<relationship_t>(relationship_t{
        .parent_type = gaia_ruleset,
        .child_type = app_ruleset,
        .first_child_offset = catalog_core_t::c_gaia_ruleset_first_app_rulesets,
        .next_child_offset = catalog_core_t::c_app_ruleset_next_ruleset,
        .prev_child_offset = c_invalid_reference_offset,
        .parent_offset = catalog_core_t::c_app_ruleset_parent_ruleset,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto gaia_table_rule_table = std::make_shared<relationship_t>(relationship_t{
        .parent_type = gaia_table,
        .child_type = rule_table,
        .first_child_offset = catalog_core_t::c_gaia_table_first_rule_tables,
        .next_child_offset = catalog_core_t::c_rule_table_next_table,
        .prev_child_offset = c_invalid_reference_offset,
        .parent_offset = catalog_core_t::c_rule_table_parent_table,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto gaia_rule_rule_table = std::make_shared<relationship_t>(relationship_t{
        .parent_type = gaia_rule,
        .child_type = rule_table,
        .first_child_offset = catalog_core_t::c_gaia_rule_first_rule_tables,
        .next_child_offset = catalog_core_t::c_rule_table_next_rule,
        .prev_child_offset = c_invalid_reference_offset,
        .parent_offset = catalog_core_t::c_rule_table_parent_rule,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto gaia_rule_rule_field = std::make_shared<relationship_t>(relationship_t{
        .parent_type = gaia_rule,
        .child_type = rule_field,
        .first_child_offset = catalog_core_t::c_gaia_rule_first_rule_fields,
        .next_child_offset = catalog_core_t::c_rule_field_next_rule,
        .prev_child_offset = c_invalid_reference_offset,
        .parent_offset = catalog_core_t::c_rule_field_parent_rule,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto gaia_field_rule_field = std::make_shared<relationship_t>(relationship_t{
        .parent_type = gaia_field,
        .child_type = rule_field,
        .first_child_offset = catalog_core_t::c_gaia_field_first_rule_fields,
        .next_child_offset = catalog_core_t::c_rule_field_next_field,
        .prev_child_offset = c_invalid_reference_offset,
        .parent_offset = catalog_core_t::c_rule_field_parent_field,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto gaia_relationship_rule_relationship = std::make_shared<relationship_t>(relationship_t{
        .parent_type = gaia_relationship,
        .child_type = rule_relationship,
        .first_child_offset = catalog_core_t::c_gaia_relationship_first_rule_relationships,
        .next_child_offset = catalog_core_t::c_rule_relationship_next_relationship,
        .prev_child_offset = c_invalid_reference_offset,
        .parent_offset = catalog_core_t::c_rule_relationship_parent_relationship,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto& gaia_database_metadata = get_or_create_no_lock(gaia_database);
    gaia_database_metadata.add_parent_relationship(gaia_database_gaia_table);
    gaia_database_metadata.add_parent_relationship(gaia_database_app_database);
    gaia_database_metadata.add_parent_relationship(gaia_database_ruleset_database);
    gaia_database_metadata.mark_as_initialized();

    auto& gaia_table_metadata = get_or_create_no_lock(gaia_table);
    gaia_table_metadata.add_child_relationship(gaia_database_gaia_table);
    gaia_table_metadata.add_parent_relationship(gaia_table_gaia_field);
    gaia_table_metadata.add_parent_relationship(gaia_table_gaia_relationship_child);
    gaia_table_metadata.add_parent_relationship(gaia_table_gaia_relationship_parent);
    gaia_table_metadata.add_parent_relationship(gaia_table_gaia_index);
    gaia_table_metadata.add_parent_relationship(gaia_table_rule_table);
    gaia_table_metadata.mark_as_initialized();

    auto& gaia_field_metadata = get_or_create_no_lock(gaia_field);
    gaia_field_metadata.add_child_relationship(gaia_table_gaia_field);
    gaia_field_metadata.add_parent_relationship(gaia_field_rule_field);
    gaia_field_metadata.mark_as_initialized();

    auto& gaia_relationship_metadata = get_or_create_no_lock(gaia_relationship);
    gaia_relationship_metadata.add_child_relationship(gaia_table_gaia_relationship_parent);
    gaia_relationship_metadata.add_child_relationship(gaia_table_gaia_relationship_child);
    gaia_relationship_metadata.add_parent_relationship(gaia_relationship_rule_relationship);
    gaia_relationship_metadata.mark_as_initialized();

    auto& gaia_ruleset_metadata = get_or_create_no_lock(gaia_ruleset);
    gaia_ruleset_metadata.add_parent_relationship(gaia_ruleset_gaia_rule);
    gaia_ruleset_metadata.add_parent_relationship(gaia_ruleset_ruleset_database);
    gaia_ruleset_metadata.add_parent_relationship(gaia_ruleset_app_ruleset);
    gaia_ruleset_metadata.mark_as_initialized();

    auto& gaia_rule_metadata = get_or_create_no_lock(gaia_rule);
    gaia_rule_metadata.add_child_relationship(gaia_ruleset_gaia_rule);
    gaia_rule_metadata.add_parent_relationship(gaia_rule_rule_relationship);
    gaia_rule_metadata.add_parent_relationship(gaia_rule_rule_table);
    gaia_rule_metadata.add_parent_relationship(gaia_rule_rule_field);
    gaia_rule_metadata.mark_as_initialized();

    auto& gaia_index_metadata = get_or_create_no_lock(gaia_index);
    gaia_index_metadata.add_child_relationship(gaia_table_gaia_index);
    gaia_index_metadata.mark_as_initialized();

    auto& gaia_application_metadata = get_or_create_no_lock(gaia_application);
    gaia_application_metadata.add_parent_relationship(gaia_application_app_database);
    gaia_application_metadata.add_parent_relationship(gaia_application_app_ruleset);
    gaia_application_metadata.mark_as_initialized();

    auto& app_database_metadata = get_or_create_no_lock(app_database);
    app_database_metadata.add_child_relationship(gaia_database_app_database);
    app_database_metadata.add_child_relationship(gaia_application_app_database);
    app_database_metadata.mark_as_initialized();

    auto& app_ruleset_metadata = get_or_create_no_lock(app_ruleset);
    app_ruleset_metadata.add_child_relationship(gaia_application_app_ruleset);
    app_ruleset_metadata.add_child_relationship(gaia_ruleset_app_ruleset);
    app_ruleset_metadata.mark_as_initialized();

    auto& ruleset_database_metadata = get_or_create_no_lock(ruleset_database);
    ruleset_database_metadata.add_child_relationship(gaia_database_ruleset_database);
    ruleset_database_metadata.add_child_relationship(gaia_ruleset_ruleset_database);
    ruleset_database_metadata.mark_as_initialized();

    auto& rule_table_metadata = get_or_create_no_lock(rule_table);
    rule_table_metadata.add_child_relationship(gaia_table_rule_table);
    rule_table_metadata.add_child_relationship(gaia_rule_rule_table);
    rule_table_metadata.mark_as_initialized();

    auto& rule_field_metadata = get_or_create_no_lock(rule_field);
    rule_field_metadata.add_child_relationship(gaia_field_rule_field);
    rule_field_metadata.add_child_relationship(gaia_rule_rule_field);
    rule_field_metadata.mark_as_initialized();

    auto& rule_relationship_metadata = get_or_create_no_lock(rule_relationship);
    rule_relationship_metadata.add_child_relationship(gaia_rule_rule_relationship);
    rule_relationship_metadata.add_child_relationship(gaia_relationship_rule_relationship);
    rule_relationship_metadata.mark_as_initialized();
}

gaia_id_t type_registry_t::get_record_id(gaia_type_t type)
{
    return type_id_mapping_t::instance().get_record_id(type);
}

bool type_registry_t::exists(gaia_type_t type) const
{
    std::shared_lock lock(m_registry_lock);

    return m_metadata_registry.find(type) != m_metadata_registry.end();
}

const type_metadata_t& type_registry_t::get(gaia_type_t type)
{
    std::unique_lock lock(m_registry_lock);
    auto it = m_metadata_registry.find(type);

    if (it != m_metadata_registry.end() && it->second->is_initialized())
    {
        return *it->second;
    }

    return create(type);
}

void type_registry_t::clear()
{
    m_metadata_registry.clear();
    init();
}

type_metadata_t& type_registry_t::test_get_or_create(gaia_type_t type)
{
    auto& metadata = get_or_create_no_lock(type);
    metadata.mark_as_initialized();
    return metadata;
}

static std::shared_ptr<relationship_t> create_relationship(relationship_view_t relationship)
{
    auto parent_table_type = catalog_core_t::get_table(relationship.parent_table_id()).table_type();
    auto child_table_type = catalog_core_t::get_table(relationship.child_table_id()).table_type();
    return std::make_shared<relationship_t>(relationship_t{
        .parent_type = parent_table_type,
        .child_type = child_table_type,
        .first_child_offset = relationship.first_child_offset(),
        .next_child_offset = relationship.next_child_offset(),
        .prev_child_offset = relationship.prev_child_offset(),
        .parent_offset = relationship.parent_offset(),
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = relationship.is_value_linked()});
}

type_metadata_t& type_registry_t::create(gaia_type_t type)
{
    gaia_log::db().trace("Creating metadata for type: '{}'", type);

    gaia_id_t record_id = get_record_id(type);
    if (record_id == c_invalid_gaia_id)
    {
        throw invalid_object_type_internal(type);
    }
    auto& metadata = get_or_create_no_lock(type);

    for (auto relationship_view : catalog_core_t::list_relationship_to(record_id))
    {
        if (metadata.find_child_relationship(relationship_view.parent_offset()))
        {
            continue;
        }

        auto rel = create_relationship(relationship_view);

        gaia_log::db().trace(
            " relationship parent:'{}', child:'{}', first_child_offset:'{}', parent_offset:'{}', next_child_offset:'{}'",
            rel->parent_type, rel->child_type, rel->first_child_offset, rel->parent_offset, rel->next_child_offset);

        auto& parent_meta = get_or_create_no_lock(rel->parent_type);
        parent_meta.add_parent_relationship(rel);

        metadata.add_child_relationship(rel);
    }

    for (auto relationship_view : catalog_core_t::list_relationship_from(record_id))
    {
        if (metadata.find_parent_relationship(relationship_view.first_child_offset()))
        {
            continue;
        }

        auto rel = create_relationship(relationship_view);

        gaia_log::db().trace(
            " with relationship parent:'{}', child:'{}', first_child_offset:'{}', parent_offset:'{}', next_child_offset:'{}'",
            rel->parent_type, rel->child_type, rel->first_child_offset, rel->parent_offset, rel->next_child_offset);

        auto& child_meta = get_or_create_no_lock(rel->child_type);
        child_meta.add_child_relationship(rel);

        metadata.add_parent_relationship(rel);
    }

    metadata.mark_as_initialized();
    return metadata;
}

type_metadata_t& type_registry_t::get_or_create_no_lock(gaia_type_t type)
{
    auto it = m_metadata_registry.find(type);

    if (it != m_metadata_registry.end())
    {
        return *it->second;
    }

    auto it2 = m_metadata_registry.emplace(type, std::make_unique<type_metadata_t>(type));
    return *it2.first->second;
}

} // namespace db
} // namespace gaia
