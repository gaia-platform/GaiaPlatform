/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <initializer_list>
#include <iostream>

#include "gtest/gtest.h"

#include "gaia/common.hpp"
#include "gaia/direct_access/auto_transaction.hpp"

#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/common/retail_assert.hpp"
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

    void check_path(const table_path_t& actual_path, std::initializer_list<std::string> expected_path)
    {
        ASSERT_PRECONDITION(expected_path.size() >= 3, "The expected path must be of made of at least 3 pieces.");
        ASSERT_PRECONDITION(expected_path.size() % 2 == 1, "The expected path must have an odd number of pieces.");

        auto actual_iter = actual_path.path.begin();
        auto expected_iter = expected_path.begin();

        bool path_matches = true;

        while (actual_iter != actual_path.path.end())
        {
            string source_node_name = *expected_iter;
            string link_name = *(++expected_iter);
            string target_node_name = *(++expected_iter);

            gaia_table_t source_table = gaia_table_t::get(actual_iter->source_table);
            gaia_table_t target_table = gaia_table_t::get(actual_iter->target_table);

            if (source_node_name != source_table.name()
                || link_name != actual_iter->link_name
                || target_node_name != target_table.name())
            {
                path_matches = false;
                break;
            }

            actual_iter++;
        }

        if (!path_matches)
        {
            FAIL() << "The paths do not match.";
        }
    }
};

TEST_F(test_catalog_graph, test_find_all_paths)
{
    auto_transaction_t txn;

    catalog_graph_t catalog_graph;

    gaia_table_t student_table = *(gaia_table_t::list().where(gaia_table_expr::name == "student").begin());
    gaia_table_t course_table = *(gaia_table_t::list().where(gaia_table_expr::name == "course").begin());

    auto paths = catalog_graph.find_paths(student_table.gaia_id(), course_table.gaia_id());

    ASSERT_EQ(paths.size(), 3);

    // student-[registrations]->registration-[registered_course]->course
    check_path(paths[0], {"student", "registrations", "registration", "registered_course", "course"});

    // student-[registrations]->registration-[registered_course]->course-[requires]->prereq-[course]->course
    check_path(paths[1], {"student", "registrations", "registration", "registered_course", "course", "requires", "prereq", "course", "course"});

    // student-[registrations]->registration-[registered_course]->course-[requires]->prereq-[prereq]->course
    check_path(paths[2], {"student", "registrations", "registration", "registered_course", "course", "requires", "prereq", "prereq", "course"});
}

TEST_F(test_catalog_graph, test_find_shortest_paths)
{
    auto_transaction_t txn;

    catalog_graph_t catalog_graph;

    gaia_table_t student_table = *(gaia_table_t::list().where(gaia_table_expr::name == "student").begin());
    gaia_table_t course_table = *(gaia_table_t::list().where(gaia_table_expr::name == "course").begin());

    auto paths = catalog_graph.find_shortest_paths(student_table.gaia_id(), course_table.gaia_id());

    ASSERT_EQ(paths.size(), 1);
    check_path(paths[0], {"student", "registrations", "registration", "registered_course", "course"});
}
