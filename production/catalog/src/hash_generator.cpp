/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <algorithm>
#include <string>
#include <vector>

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
namespace
{

// Calculate and store a hash for this index. Store it in the gaia_index row.
void add_hash(multi_segment_hash& parent_hash, gaia_index_t index)
{
    multi_segment_hash index_hash;
    index_hash.hash_add(index.name());
    index_hash.hash_add(index.unique());
    index_hash.hash_add(index.type());
    for (size_t field_position = 0; field_position < index.fields().size(); ++field_position)
    {
        index_hash.hash_add(index.fields()[field_position]);
    }
    index_hash.hash_calc();
    parent_hash.hash_include(index_hash.hash());
    auto index_w = index.writer();
    index_w.hash = index_hash.to_string();
    index_w.update_row();
}

// Calculate and store a hash for this field. Store it in the gaia_field row.
void add_hash(multi_segment_hash& parent_hash, gaia_field_t field)
{
    multi_segment_hash field_hash;
    field_hash.hash_add(field.name());
    field_hash.hash_add(field.type());
    field_hash.hash_add(field.repeated_count());
    field_hash.hash_add(field.position());
    // active? Currently 'active' is unused.
    field_hash.hash_add(field.unique());
    field_hash.hash_add(field.optional());
    field_hash.hash_calc();
    parent_hash.hash_include(field_hash.hash());
    auto field_w = field.writer();
    field_w.hash = field_hash.to_string();
    field_w.update_row();
}

// Calculate and store a hash for this relationship. Store it in the gaia_relationship row.
void add_hash(multi_segment_hash& parent_hash, gaia_relationship_t relationship)
{
    if (relationship.deprecated())
    {
        return;
    }
    // Calculate the hash.
    multi_segment_hash relationship_hash;
    relationship_hash.hash_add(relationship.name());
    relationship_hash.hash_add(relationship.to_parent_link_name());
    relationship_hash.hash_add(relationship.to_child_link_name());
    relationship_hash.hash_add(relationship.cardinality());
    relationship_hash.hash_add(relationship.parent_required());
    for (
        size_t parent_position = 0;
        parent_position < relationship.parent_field_positions().size();
        ++parent_position)
    {
        relationship_hash.hash_add(relationship.parent_field_positions()[parent_position]);
    }
    for (
        size_t child_position = 0;
        child_position < relationship.child_field_positions().size();
        ++child_position)
    {
        relationship_hash.hash_add(relationship.parent_field_positions()[child_position]);
    }
    auto relationship_w = relationship.writer();
    relationship_hash.hash_calc();
    parent_hash.hash_include(relationship_hash.hash());
    relationship_w.hash = relationship_hash.to_string();
    relationship_w.update_row();
}


// Calculate and store a hash for this table. Store it in the gaia_table row.
void add_hash(multi_segment_hash& parent_hash, gaia_table_t table)
{
    // Calculate the hash.
    multi_segment_hash table_hash;
    table_hash.hash_add(table.name());
    table_hash.hash_add(table.type());

    for (auto& field : table.gaia_fields())
    {
        add_hash(table_hash, field);
    }

    auto table_w = table.writer();
    table_hash.hash_calc();
    parent_hash.hash_include(table_hash.hash());
    table_w.hash = table_hash.to_string();
    table_w.update_row();
}


bool sort_by_name(
    const pair<string, common::gaia_id_t>& a,
    const pair<string, common::gaia_id_t>& b)
{
    return (a.first < b.first);
}
} // namespace

// For every catalog row that defines this database, calculate and store its hash.
void add_hash(const std::string db_name)
{
    // The database hash is composed of the hash for the database name, followed by the hashes
    // of all database tables.
    const auto& db_list = gaia_database_t::list().where(gaia_database_expr::name == db_name);
    if (db_list.begin() != db_list.end())
    {
        multi_segment_hash db_hash;
        auto db = *db_list.begin();
        db_hash.hash_add(db.name());
        auto db_w = db.writer();

        // Prepare to collect and sort the tables by the table name. This ensures that the
        // same hash for the database will be generated even if the table definitions have been
        // re-arranged in the DDL.
        vector<pair<string, common::gaia_id_t>> table_vector;
        // Prepare to collect and sort the indices by the table name. This ensures that the
        // same hash for the database will be generated even if the index definitions have been
        // re-arranged in the DDL.
        vector<pair<string, common::gaia_id_t>> index_vector;
        // Prepare to collect and sort the relations by the table name. This ensures that the
        // same hash for the database will be generated even if the relation definitions have been
        // re-arranged in the DDL.
        vector<pair<string, common::gaia_id_t>> relationship_vector;

        // The table hash is composed of the hashes for the table's name and type, followed
        // by the hashes of all indexes, fields and relationships that are connected to this table.
        for (const auto& table : db.gaia_tables())
        {
            pair<string, common::gaia_id_t> table_pair;
            table_pair.first = table.name();
            table_pair.second = table.gaia_id();
            table_vector.push_back(table_pair);
        }

        for (const auto& index : gaia_index_t::list())
        {
            const gaia_table_t& table = index.table();
            const gaia_database_t& database = table.database();
            if (db.gaia_id() == database.gaia_id())
            {
                pair<string, common::gaia_id_t> index_pair;
                index_pair.first = index.name();
                index_pair.second = index.gaia_id();
                index_vector.push_back(index_pair);
            }
        }

        for (const auto& relationship : gaia_relationship_t::list())
        {
            const gaia_table_t& table = relationship.child();
            const gaia_database_t& database = table.database();
            if (db.gaia_id() == database.gaia_id())
            {
                pair<string, common::gaia_id_t> relationship_pair;
                relationship_pair.first = relationship.name();
                relationship_pair.second = relationship.gaia_id();
                relationship_vector.push_back(relationship_pair);
            }
        }

        // Sort the data.
        sort(table_vector.begin(), table_vector.end(), sort_by_name);
        sort(index_vector.begin(), index_vector.end(), sort_by_name);
        sort(relationship_vector.begin(), relationship_vector.end(), sort_by_name);

        // Calculate the hash.
        for (const auto& table : table_vector)
        {
            add_hash(db_hash, gaia_table_t::get(table.second));
        }
        for (const auto& index : index_vector)
        {
            add_hash(db_hash, gaia_index_t::get(index.second));
        }
        for (const auto& relationship : relationship_vector)
        {
            add_hash(db_hash, gaia_relationship_t::get(relationship.second));
        }

        // Finally, store the hash string of the database.
        db_hash.hash_calc();
        db_w.hash = db_hash.to_string();
        db_w.update_row();
    }
}

// Apply this algorithm on all non-system databases.
void add_hash()
{
    for (auto& db : gaia_database_t::list())
    {
        if (strcmp(db.name(), "catalog") && strcmp(db.name(), "event_log"))
        {
            add_hash(db.name());
        }
    }
}

} // namespace catalog
} // namespace gaia
