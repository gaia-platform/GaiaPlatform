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

    if (relationship->value_linked)
    {
        m_has_value_linked_relationship = true;
    }
}

void type_metadata_t::add_child_relationship(const std::shared_ptr<relationship_t>& relationship)
{
    std::unique_lock lock(m_metadata_lock);

    m_child_relationships.insert({relationship->parent_offset, relationship});
    m_reference_count += 3;

    if (relationship->value_linked)
    {
        m_has_value_linked_relationship = true;
    }
}

gaia_type_t type_metadata_t::get_type() const
{
    return m_type;
}

reference_offset_t type_metadata_t::references_count() const
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

bool type_metadata_t::has_value_linked_relationship() const
{
    return m_has_value_linked_relationship;
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
    auto database = static_cast<gaia_type_t::value_type>(catalog_core_table_type_t::gaia_database);
    auto table = static_cast<gaia_type_t::value_type>(catalog_core_table_type_t::gaia_table);
    auto field = static_cast<gaia_type_t::value_type>(catalog_core_table_type_t::gaia_field);
    auto relationship = static_cast<gaia_type_t::value_type>(catalog_core_table_type_t::gaia_relationship);
    auto index = static_cast<gaia_type_t::value_type>(catalog_core_table_type_t::gaia_index);

    auto db_table_relationship = std::make_shared<relationship_t>(relationship_t{
        .parent_type = database,
        .child_type = table,
        .first_child_offset = catalog_core::c_gaia_database_first_gaia_table_offset,
        .next_child_offset = catalog_core::c_gaia_table_next_gaia_table_offset,
        .prev_child_offset = catalog_core::c_gaia_table_prev_gaia_table_offset,
        .parent_offset = catalog_core::c_gaia_table_parent_gaia_database_offset,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto table_field_relationship = std::make_shared<relationship_t>(relationship_t{
        .parent_type = table,
        .child_type = field,
        .first_child_offset = catalog_core::c_gaia_table_first_gaia_field_offset,
        .next_child_offset = catalog_core::c_gaia_field_next_gaia_field_offset,
        .prev_child_offset = catalog_core::c_gaia_field_prev_gaia_field_offset,
        .parent_offset = catalog_core::c_gaia_field_parent_gaia_table_offset,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto relationship_parent_table_relationship = std::make_shared<relationship_t>(relationship_t{
        .parent_type = table,
        .child_type = relationship,
        .first_child_offset = catalog_core::c_gaia_table_first_parent_gaia_relationship_offset,
        .next_child_offset = catalog_core::c_gaia_relationship_next_parent_gaia_relationship_offset,
        .prev_child_offset = catalog_core::c_gaia_relationship_prev_parent_gaia_relationship_offset,
        .parent_offset = catalog_core::c_gaia_relationship_parent_parent_gaia_table_offset,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto relationship_child_table_relationship = std::make_shared<relationship_t>(relationship_t{
        .parent_type = table,
        .child_type = relationship,
        .first_child_offset = catalog_core::c_gaia_table_first_child_gaia_relationship_offset,
        .next_child_offset = catalog_core::c_gaia_relationship_next_child_gaia_relationship_offset,
        .prev_child_offset = catalog_core::c_gaia_relationship_prev_child_gaia_relationship_offset,
        .parent_offset = catalog_core::c_gaia_relationship_parent_child_gaia_table_offset,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto table_index_relationship = std::make_shared<relationship_t>(relationship_t{
        .parent_type = table,
        .child_type = index,
        .first_child_offset = catalog_core::c_gaia_table_first_gaia_index_offset,
        .next_child_offset = catalog_core::c_gaia_index_next_gaia_index_offset,
        .prev_child_offset = catalog_core::c_gaia_index_prev_gaia_index_offset,
        .parent_offset = catalog_core::c_gaia_index_parent_gaia_table_offset,
        .cardinality = cardinality_t::many,
        .parent_required = false,
        .value_linked = false});

    auto& db_metadata = get_or_create_no_lock(database);
    db_metadata.add_parent_relationship(db_table_relationship);
    db_metadata.mark_as_initialized();

    auto& table_metadata = get_or_create_no_lock(table);
    table_metadata.add_child_relationship(db_table_relationship);
    table_metadata.add_parent_relationship(table_field_relationship);
    table_metadata.add_parent_relationship(relationship_child_table_relationship);
    table_metadata.add_parent_relationship(relationship_parent_table_relationship);
    table_metadata.add_parent_relationship(table_index_relationship);
    table_metadata.mark_as_initialized();

    auto& field_metadata = get_or_create_no_lock(field);
    field_metadata.add_child_relationship(table_field_relationship);
    field_metadata.mark_as_initialized();

    auto& relationship_metadata = get_or_create_no_lock(relationship);
    relationship_metadata.add_child_relationship(relationship_parent_table_relationship);
    relationship_metadata.add_child_relationship(relationship_child_table_relationship);
    relationship_metadata.mark_as_initialized();

    auto& index_metadata = get_or_create_no_lock(index);
    index_metadata.add_child_relationship(table_index_relationship);
    index_metadata.mark_as_initialized();
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

static std::shared_ptr<relationship_t> create_relationship(catalog_core::relationship_view_t relationship)
{
    auto parent_table_type = catalog_core::get_table(relationship.parent_table_id()).table_type();
    auto child_table_type = catalog_core::get_table(relationship.child_table_id()).table_type();
    return std::make_shared<relationship_t>(relationship_t{
        .parent_type = parent_table_type,
        .child_type = child_table_type,
        .first_child_offset = relationship.first_child_offset(),
        .next_child_offset = relationship.next_child_offset(),
        .prev_child_offset = relationship.prev_child_offset(),
        .parent_offset = relationship.parent_offset(),
        .cardinality = relationship.cardinality(),
        .parent_required = false,
        .value_linked = relationship.is_value_linked()});
}

type_metadata_t& type_registry_t::create(gaia_type_t type)
{
    gaia_log::db().trace("Creating metadata for type: '{}'", type);

    gaia_id_t record_id = get_record_id(type);
    if (!record_id.is_valid())
    {
        throw invalid_object_type_internal(type);
    }
    auto& metadata = get_or_create_no_lock(type);

    for (auto relationship_view : catalog_core::list_relationship_to(record_id))
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

    for (auto relationship_view : catalog_core::list_relationship_from(record_id))
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
