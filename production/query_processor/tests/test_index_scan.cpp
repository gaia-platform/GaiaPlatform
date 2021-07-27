/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <string>

#include "gtest/gtest.h"

#include "gaia_internal/db/catalog_core.hpp"
#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "gaia_index_sandbox.h"
#include "index_scan.hpp"
#include "type_id_mapping.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::direct_access;
using namespace gaia::qp::scan;

constexpr size_t c_num_initial_rows = 20;

class test_index_scan : public gaia::db::db_catalog_test_base_t
{
public:
    test_index_scan()
        : db_catalog_test_base_t("index_sandbox.ddl"){};

    void SetUp() override
    {
        db_catalog_test_base_t::SetUp();

        auto_transaction_t txn;

        for (size_t i = 0; i < c_num_initial_rows; i++)
        {
            auto w = gaia::index_sandbox::sandbox_writer();
            w.str = ""; // SAME
            w.f = static_cast<float>(i); // ASC
            w.i = c_num_initial_rows - i; // DESC
            w.insert_row();
        }

        txn.commit();
    }
};

// Check counts match for all indexes.
TEST_F(test_index_scan, verify_cardinality)
{
    auto_transaction_t txn;

    size_t count = 0;

    for (const auto& obj : gaia::index_sandbox::sandbox_t::list())
    {
        (void)obj;
        count++;
    }

    EXPECT_TRUE(count == c_num_initial_rows);

    gaia_id_t type_record_id = type_id_mapping_t::instance().get_record_id(gaia::index_sandbox::sandbox_t::s_gaia_type);

    for (const auto& idx : catalog_core_t::list_indexes(type_record_id))
    {
        // Open an index scan operator for this operator.
        auto scan = index_scan_t(idx.id());
        size_t scan_count = 0;
        for (const auto& p : scan)
        {
            (void)p;
            scan_count++;
        }

        EXPECT_TRUE(count == scan_count);
    }
}

TEST_F(test_index_scan, rollback_txn)
{
    // Don't insert the row via the writer, rollback.
    gaia::db::begin_transaction();
    auto w = gaia::index_sandbox::sandbox_writer();
    w.str = "HELLO";
    w.f = 21.0;
    w.i = -1;
    gaia::db::commit_transaction();

    // Rollback operation should not be visible to scans.
    gaia::db::begin_transaction();
    gaia_id_t type_record_id = type_id_mapping_t::instance().get_record_id(gaia::index_sandbox::sandbox_t::s_gaia_type);

    for (const auto& idx : catalog_core_t::list_indexes(type_record_id))
    {
        // Open an index scan operator for this operator.
        auto scan = index_scan_t(idx.id());
        size_t scan_count = 0;
        for (const auto& p : scan)
        {
            (void)p;
            scan_count++;
        }
        EXPECT_TRUE(c_num_initial_rows == scan_count);
    }
    gaia::db::commit_transaction();
}

TEST_F(test_index_scan, insert_followed_by_delete)
{
    gaia::common::gaia_id_t to_delete;

    gaia::db::begin_transaction();
    auto w = gaia::index_sandbox::sandbox_writer();
    w.str = "HELLO";
    w.f = 21.0;
    w.i = -1;
    to_delete = w.insert_row();

    // Insert should be visible.
    gaia_id_t type_record_id = type_id_mapping_t::instance().get_record_id(gaia::index_sandbox::sandbox_t::s_gaia_type);

    for (const auto& idx : catalog_core_t::list_indexes(type_record_id))
    {
        // Open an index scan operator for this operator.
        auto scan = index_scan_t(idx.id());
        size_t scan_count = 0;
        for (const auto& p : scan)
        {
            (void)p;
            scan_count++;
        }
        EXPECT_TRUE(c_num_initial_rows + 1 == scan_count);
    }
    gaia::db::commit_transaction();

    gaia::db::begin_transaction();
    gaia::index_sandbox::sandbox_t::get(to_delete).delete_row();

    // Delete in effect.
    for (const auto& idx : catalog_core_t::list_indexes(type_record_id))
    {
        // Open an index scan operator for this operator.
        auto scan = index_scan_t(idx.id());
        size_t scan_count = 0;
        for (const auto& p : scan)
        {
            (void)p;
            scan_count++;
        }
        EXPECT_TRUE(c_num_initial_rows == scan_count);
    }
    gaia::db::commit_transaction();
}

TEST_F(test_index_scan, multi_insert_followed_by_delete)
{
    gaia::common::gaia_id_t to_delete, to_delete2, to_delete3;

    gaia::db::begin_transaction();
    auto w = gaia::index_sandbox::sandbox_writer();
    w.str = "HELLO";
    w.f = 21.0;
    w.i = -1;
    to_delete = w.insert_row();
    to_delete2 = w.insert_row();
    to_delete3 = w.insert_row();

    // Insert should be visible.
    gaia_id_t type_record_id = type_id_mapping_t::instance().get_record_id(gaia::index_sandbox::sandbox_t::s_gaia_type);

    for (const auto& idx : catalog_core_t::list_indexes(type_record_id))
    {
        // Open an index scan operator for this operator.
        auto scan = index_scan_t(idx.id());
        size_t scan_count = 0;
        for (const auto& p : scan)
        {
            (void)p;
            scan_count++;
        }
        EXPECT_TRUE(c_num_initial_rows + 3 == scan_count);
    }
    gaia::db::commit_transaction();

    gaia::db::begin_transaction();
    gaia::index_sandbox::sandbox_t::get(to_delete).delete_row();
    gaia::index_sandbox::sandbox_t::get(to_delete2).delete_row();
    gaia::index_sandbox::sandbox_t::get(to_delete3).delete_row();

    // Delete in effect.
    for (const auto& idx : catalog_core_t::list_indexes(type_record_id))
    {
        // Open an index scan operator for this operator.
        auto scan = index_scan_t(idx.id());
        size_t scan_count = 0;
        for (const auto& p : scan)
        {
            (void)p;
            scan_count++;
        }
        EXPECT_TRUE(c_num_initial_rows == scan_count);
    }

    gaia::db::commit_transaction();
}
