/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <algorithm>
#include <string>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/hash.hpp"
#include "gaia_internal/common/retail_assert.hpp"

#include <gaia_spdlog/fmt/fmt.h>

using namespace std;
using namespace gaia::common::hash;
using namespace gaia::db;

namespace gaia
{
namespace catalog
{

// Calculate and store a hash for this index. Include it in it's table.
static void add_index_hashes(multi_segment_hash& table_hash, gaia_index_t& index)
{
    multi_segment_hash hashes;
    hashes.hash_add(index.name());
    hashes.hash_add(index.unique());
    hashes.hash_add(index.type());
    for (size_t field_position = 0; field_position < index.fields().size(); ++field_position)
    {
        hashes.hash_add(field_position);
    }
    hashes.hash_calc();
    table_hash.hash_include(hashes.hash());
    auto index_w = index.writer();
    index_w.hash = hashes.to_string();
    index_w.update_row();
}

// Calculate and store a hash for this field. Include it in it's table.
static void add_field_hashes(multi_segment_hash& table_hash, gaia_field_t& field)
{
    multi_segment_hash hashes;
    hashes.hash_add(field.name());
    hashes.hash_add(field.type());
    hashes.hash_add(field.repeated_count());
    hashes.hash_add(field.position());
    // active? Currently 'active' is unused.
    hashes.hash_add(field.unique());
    hashes.hash_add(field.optional());
    hashes.hash_calc();
    table_hash.hash_include(hashes.hash());
    auto field_w = field.writer();
    field_w.hash = hashes.to_string();
    field_w.update_row();
}

// Calculate and store a hash for this relationship. Include it in it's table.
static void add_relationship_hashes(multi_segment_hash& table_hash, gaia_relationship_t& relationship)
{
    if (relationship.deprecated())
    {
        return;
    }
    // Calculate the hash.
    multi_segment_hash hashes;
    hashes.hash_add(relationship.name());
    hashes.hash_add(relationship.to_parent_link_name());
    hashes.hash_add(relationship.to_child_link_name());
    hashes.hash_add(relationship.cardinality());
    hashes.hash_add(relationship.parent_required());
    for (
        size_t parent_position = 0;
        parent_position < relationship.parent_field_positions().size();
        ++parent_position)
    {
        hashes.hash_add(parent_position);
    }
    for (
        size_t child_position = 0;
        child_position < relationship.child_field_positions().size();
        ++child_position)
    {
        hashes.hash_add(child_position);
    }
    auto relationship_w = relationship.writer();
    hashes.hash_calc();
    table_hash.hash_include(hashes.hash());
    relationship_w.hash = hashes.to_string();
    relationship_w.update_row();
}

// For every catalog row that defines this database, calculate and store its hash.
void add_catalog_hashes(const std::string db_name)
{
    begin_transaction();

    // The database hash is composed of the hash for the database name, followed by the hashes
    // of all database tables.
    for (auto& db : gaia_database_t::list().where(gaia_database_expr::name == db_name))
    {
        multi_segment_hash db_hash;
        db_hash.hash_add(db.name());
        auto db_w = db.writer();

        // The table hash is composed of the hashes for the table's name and type, followed
        // by the hashes of all indexes, fields and relationships that are connected to this table.
        for (auto& table : db.gaia_tables())
        {
            multi_segment_hash table_hash;
            table_hash.hash_add(table.name());
            table_hash.hash_add(table.type());

            for (auto& index : table.gaia_indexes())
            {
                add_index_hashes(table_hash, index);
            }

            for (auto& field : table.gaia_fields())
            {
                add_field_hashes(table_hash, field);
            }

            for (auto& parent_relationship : table.outgoing_relationships())
            {
                add_relationship_hashes(table_hash, parent_relationship);
            }

            for (auto& child_relationship : table.incoming_relationships())
            {
                add_relationship_hashes(table_hash, child_relationship);
            }

            // Include this table's hash in the database's composite, and
            // store the hash string in this table.
            table_hash.hash_calc();
            db_hash.hash_include(table_hash.hash());
            auto table_w = table.writer();
            table_w.hash = table_hash.to_string();
            table_w.update_row();
        }

        // Finally, store the hash string of the database.
        db_hash.hash_calc();
        db_w.hash = db_hash.to_string();
        db_w.update_row();
    }

    commit_transaction();
}

// Apply this algorithm on all non-system databases.
void add_catalog_hashes()
{
    vector<string> db_name_list;

    // Multiple loops to avoid extra transactions.
    begin_transaction();
    for (auto& db : gaia_database_t::list())
    {
        if (strcmp(db.name(), "catalog") && strcmp(db.name(), "event_log"))
        {
            db_name_list.emplace_back(db.name());
        }
    }
    commit_transaction();

    for (const auto& dbname : db_name_list)
    {
        add_catalog_hashes(dbname);
    }
}

} // namespace catalog
} // namespace gaia
