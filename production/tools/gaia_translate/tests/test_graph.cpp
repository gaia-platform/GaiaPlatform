/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gtest/gtest.h"

#include "gaia/common.hpp"
#include "gaia/direct_access/auto_transaction.hpp"

#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "catalog_graph.hpp"

using namespace gaia::db;
using namespace gaia::catalog;
using namespace gaia::direct_access;
using namespace gaia::translation;
using namespace std;

class test_catalog_graph : public db_catalog_test_base_t
{
public:
    test_catalog_graph()
        : db_catalog_test_base_t("prerequisites.ddl"){};
};

TEST_F(test_catalog_graph, new_fancy_api)
{
    auto_transaction_t txn;

    catalog_graph_t catalog_graph;

    gaia_table_t student_table = *(gaia_table_t::list().where(gaia_table_expr::name == "student").begin());
    gaia_table_t course_table = *(gaia_table_t::list().where(gaia_table_expr::name == "course").begin());

    auto paths = catalog_graph.find_paths(student_table.gaia_id(), course_table.gaia_id());

    for (auto path : paths)
    {
        link_t last_link;
        for (auto link : path.path)
        {
            last_link = link;
            gaia_table_t source = gaia_table_t::get(link.source_table);

            cout << source.name() << "-[" << link.link_name << "]->";
        }

        gaia_table_t target = gaia_table_t::get(last_link.target_table);
        cout << target.name() << endl;
    }
}
