/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <string>

#include <gtest/gtest.h>

#include "gaia/rules/rules.hpp"

#include "gaia_internal/db/db_catalog_test_base.hpp"
#include "gaia_internal/rules/rules_test_helpers.hpp"

#include "gaia_addr_book.h"
#include "test_rulesets.hpp"

using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::direct_access;
using namespace gaia::addr_book;
using namespace gaia::rules;

extern int g_client_sales;

class test_array : public db_catalog_test_base_t
{
public:
    test_array()
        : db_catalog_test_base_t("addr_book.ddl"){};

protected:
    void SetUp() override
    {
        db_catalog_test_base_t::SetUp();
        gaia::rules::initialize_rules_engine();
    }

    void TearDown() override
    {
        db_catalog_test_base_t::TearDown();
        unsubscribe_rules();
        gaia::rules::shutdown_rules_engine();
    }

    void check_array(gaia_id_t record_id, const std::vector<int32_t>& expected_values)
    {
        gaia::db::begin_transaction();
        auto c = client_t::get(record_id);
        EXPECT_TRUE(std::equal(expected_values.begin(), expected_values.end(), c.sales().data()));
        gaia::db::commit_transaction();
    }
};

TEST_F(test_array, test_array_unqualified_fields)
{
    const std::vector<int32_t> expected_sales{9, 2, 4, 5, 4};
    gaia::db::begin_transaction();

    auto client_id = client_t::insert_row("1", {9, 8, 7, 6, 5});

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    ASSERT_EQ(g_client_sales, 9);
    check_array(client_id, expected_sales);
}

TEST_F(test_array, test_array_qualified_fields)
{
    const std::vector<int32_t> expected_sales{9, 3, 12, 7, 6};
    gaia::db::begin_transaction();

    auto client_id = client_t::insert_row("2", {9, 8, 7, 6, 5});

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    ASSERT_EQ(g_client_sales, 9);
    check_array(client_id, expected_sales);
}

TEST_F(test_array, test_array_unqualified_assignment_init)
{
    const std::vector<int32_t> expected_sales{3, 4, 5};
    gaia::db::begin_transaction();

    auto client_id = client_t::insert_row("3", {9, 8, 7, 6, 5});

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    check_array(client_id, expected_sales);
}

TEST_F(test_array, test_array_unqualified_constant_array)
{
    const std::vector<int32_t> expected_sales{1, 2, 3};
    gaia::db::begin_transaction();

    auto client_id = client_t::insert_row("4", {9, 8, 7, 6, 5});

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    check_array(client_id, expected_sales);
}

TEST_F(test_array, test_array_explicit_navigation)
{
    const std::vector<int32_t> expected_sales{9, 3, 12, 7, 6};
    gaia::db::begin_transaction();

    auto client_id = client_t::insert_row("5", {9, 8, 7, 6, 5});
    auto company = company_t::get(company_t::insert_row("c1"));
    company.clients().connect(client_t::get(client_id));

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    ASSERT_EQ(g_client_sales, 9);
    check_array(client_id, expected_sales);
}

TEST_F(test_array, test_array_implicit_navigation)
{
    const std::vector<int32_t> expected_sales{9, 3, 12, 7, 6};
    gaia::db::begin_transaction();

    auto client_id = client_t::insert_row("zz", {9, 8, 7, 6, 5});
    auto company = company_t::get(company_t::insert_row("c2"));
    company.clients().connect(client_t::get(client_id));

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    ASSERT_EQ(g_client_sales, 9);
    check_array(client_id, expected_sales);
}

TEST_F(test_array, test_array_qualified_assignment_init)
{
    const std::vector<int32_t> expected_sales{10, 11, 12};
    gaia::db::begin_transaction();

    auto client_id = client_t::insert_row("6", {9, 8, 7, 6, 5});

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    check_array(client_id, expected_sales);
}

TEST_F(test_array, test_array_qualified_constant_array)
{
    const std::vector<int32_t> expected_sales{7, 9, 4};
    gaia::db::begin_transaction();

    auto client_id = client_t::insert_row("7", {9, 8, 7, 6, 5});

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    check_array(client_id, expected_sales);
}

TEST_F(test_array, test_array_unqualified_field_to_qualified_assignment)
{
    const std::vector<int32_t> expected_sales{7, 9, 4};
    gaia::db::begin_transaction();

    auto client_id = client_t::insert_row("8", expected_sales);

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    check_array(client_id, expected_sales);
}

TEST_F(test_array, test_array_unqualified_field_to_unqualified_assignment)
{
    const std::vector<int32_t> expected_sales{7, 9, 4};
    gaia::db::begin_transaction();

    auto client_id = client_t::insert_row("9", expected_sales);

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    check_array(client_id, expected_sales);
}

TEST_F(test_array, test_array_qualified_field_to_unqualified_assignment)
{
    const std::vector<int32_t> expected_sales{7, 9, 4};
    gaia::db::begin_transaction();

    auto client_id = client_t::insert_row("10", expected_sales);

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    check_array(client_id, expected_sales);
}

TEST_F(test_array, test_array_qualified_field_to_qualified_assignment)
{
    const std::vector<int32_t> expected_sales{7, 9, 4};
    gaia::db::begin_transaction();

    auto client_id = client_t::insert_row("11", expected_sales);

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    check_array(client_id, expected_sales);
}

TEST_F(test_array, test_array_explicit_navigation_init_assignment)
{
    const std::vector<int32_t> expected_sales{6, 9, 8};
    gaia::db::begin_transaction();

    auto client_id = client_t::insert_row("yy", {9, 8, 7, 6, 5});
    auto company = company_t::get(company_t::insert_row("c3"));
    company.clients().connect(client_t::get(client_id));

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    check_array(client_id, expected_sales);
}

TEST_F(test_array, test_array_explicit_navigation_array_assignment)
{
    const std::vector<int32_t> expected_sales{1, 5, 3};
    gaia::db::begin_transaction();

    auto client_id = client_t::insert_row("xx", {9, 8, 7, 6, 5});
    auto company = company_t::get(company_t::insert_row("c4"));
    company.clients().connect(client_t::get(client_id));

    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    check_array(client_id, expected_sales);
}

TEST_F(test_array, test_array_insert)
{
    gaia::db::begin_transaction();
    auto client_id = client_t::insert_row("dsf", {9, 8, 5, 6, 1});
    auto company = company_t::get(company_t::insert_row("c5"));
    company.clients().connect(client_t::get(client_id));
    gaia::db::commit_transaction();

    gaia::db::begin_transaction();
    auto company_writer = company.writer();
    company_writer.company_name = "c6";
    company_writer.update_row();
    gaia::db::commit_transaction();

    gaia::rules::test::wait_for_rules_to_complete();

    gaia::db::begin_transaction();
    for (const auto& client : client_t::list())
    {
        std::string name = client.client_name();
        if (name.empty())
        {
            ASSERT_TRUE(client.sales().size() == 3 || client.sales().size() == 4);
            if (client.sales().size() == 3)
            {
                check_array(client.gaia_id(), {4, 7, 8});
            }
            else if (client.sales().size() == 4)
            {
                check_array(client.gaia_id(), {1, 2, 3, 5});
            }
        }
        else if (name == "a")
        {
            ASSERT_EQ(client.sales().size(), 0);
        }
        else if (name == "b")
        {
            check_array(client.gaia_id(), {1, 2, 3, 5});
        }
        else if (name == "c")
        {
            check_array(client.gaia_id(), {5, 7, 9});
        }
        else if (name == "d")
        {
            check_array(client.gaia_id(), {1, 2, 3, 5});
        }
        else if (name == "e")
        {
            check_array(client.gaia_id(), {4, 7, 8});
        }
        else if (name == "f")
        {
            check_array(client.gaia_id(), {9, 8, 5, 6, 1});
        }
        else if (name == "g")
        {
            check_array(client.gaia_id(), {9, 8, 5, 6, 1});
        }
    }
    gaia::db::commit_transaction();
}
