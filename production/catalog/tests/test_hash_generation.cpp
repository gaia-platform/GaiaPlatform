/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include <gtest/gtest.h>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/ddl_execution.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/hash.hpp"
#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_airport.h"
#include "gaia_parser.hpp"

using namespace gaia::airport;
using namespace gaia::catalog;
using namespace gaia::db;
using namespace gaia::common::hash;
using namespace std;

class hash_generation : public db_catalog_test_base_t
{
protected:
    hash_generation()
        : db_catalog_test_base_t("airport.ddl"){};
};

// Using the catalog manager's create_table(), create a catalog and an DAC header from that.
TEST_F(hash_generation, visit_catalog_definitions)
{
    create_database("airport_test");
    ddl::field_def_list_t fields;
    fields.emplace_back(make_unique<ddl::data_field_def_t>("name", data_type_t::e_string, 1));
    create_table("airport_test", "airport", fields);

    add_catalog_hashes(string("airport"));

    return;
#if 0
    begin_transaction();

    // The database hash is composed of the hash for the database name, followed by the hashes
    // of all database tables.
    for (auto& db : gaia_database_t::list().where(gaia_database_expr::name == "airport"))
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

fprintf(stderr, "  table name = %s\n", table.name());
            // Include this table's hash in the database's composite, and
            // store the hash string in this table.
            table_hash.hash_calc();
            db_hash.hash_include(table_hash.hash());
            auto table_w = table.writer();
            table_w.hash = table_hash.to_string();
            table_w.update_row();
        }

fprintf(stderr, "database name = %s\n", db.name());
        // Finally, store the hash string of the database.
        db_hash.hash_calc();
        db_w.hash = db_hash.to_string();
        db_w.update_row();
    }

    commit_transaction();
#endif
}
