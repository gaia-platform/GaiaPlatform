/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include <gtest/gtest.h>

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/ddl_execution.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_airport.h"
#include "gaia_parser.hpp"

using namespace gaia::airport;
using namespace gaia::catalog;
using namespace gaia::db;
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

    begin_transaction();

    for (auto& db : gaia_database_t::list().where(gaia_database_expr::name == "airport"))
    {
        auto database_w = db.writer();
        fprintf(stderr, "database name = %s\n", database_w.name.c_str());

        for (auto& table : db.gaia_tables())
        {
            auto table_w = table.writer();
            fprintf(stderr, "  table name = %s\n", table_w.name.c_str());

            for (auto& field : table.gaia_fields())
            {
                auto field_w = field.writer();
                fprintf(stderr, "    field name = %s\n", field_w.name.c_str());
            }

            for (auto& parent_relationship : table.outgoing_relationships())
            {
                auto parent_relationship_w = parent_relationship.writer();
                fprintf(stderr, "    outgoing relationship name = %s\n", parent_relationship_w.name.c_str());
            }

            for (auto& child_relationship : table.incoming_relationships())
            {
                auto child_relationship_w = child_relationship.writer();
                fprintf(stderr, "    incoming relationship name = %s\n", child_relationship_w.name.c_str());
                child_relationship_w.hash = "child_relationship_hash";
                child_relationship_w.update_row();
            }
        }
    }

    commit_transaction();
}
