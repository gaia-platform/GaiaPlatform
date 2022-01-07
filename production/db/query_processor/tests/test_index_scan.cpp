////////////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
//
// Use of this source code is governed by the MIT
// license that can be found in the LICENSE.txt file
// or at https://opensource.org/licenses/MIT.
////////////////////////////////////////////////////

#include <optional>
#include <string>

#include <gtest/gtest.h>

#include "gaia_internal/db/catalog_core.hpp"
#include "gaia_internal/db/db_catalog_test_base.hpp"

#include "db_object_helpers.hpp"
#include "gaia_index_sandbox.h"
#include "index_key.hpp"
#include "index_scan.hpp"
#include "type_id_mapping.hpp"

using namespace gaia::common;
using namespace gaia::db;
using namespace gaia::direct_access;
using namespace gaia::db::query_processor::scan;
using namespace gaia::catalog;
using namespace gaia::db::index;
using namespace gaia::db::catalog_core;

constexpr size_t c_num_initial_rows = 20;
constexpr size_t c_query_limit_rows = 5;
constexpr size_t c_num_inclusive_rows = 5;
constexpr size_t c_num_exclusive_rows = 3;
constexpr const char* c_no_match_str = "no match";

class db__query_processor__index_scan__test : public gaia::db::db_catalog_test_base_t
{
public:
    db__query_processor__index_scan__test()
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
            w.optional_i = nullopt; // ALL NULLs
            w.insert_row();
        }

        txn.commit();
    }
};

// Check counts match for all indexes.
TEST_F(db__query_processor__index_scan__test, verify_cardinality)
{
    auto_transaction_t txn;

    size_t count = 0;

    for (const auto& obj : gaia::index_sandbox::sandbox_t::list())
    {
        (void)obj;
        count++;
    }

    EXPECT_TRUE(count == c_num_initial_rows);

    gaia_id_t table_id = type_id_mapping_t::instance().get_table_id(gaia::index_sandbox::sandbox_t::s_gaia_type);

    for (const index_view_t& index : list_indexes(table_id))
    {
        // Open an index scan operator for this operator.
        auto scan = index_scan_t(index.id());
        size_t scan_count = 0;
        for (const auto& p : scan)
        {
            (void)p;
            scan_count++;
        }

        EXPECT_TRUE(count == scan_count);
    }
}

// Check scans on empty indexes.
TEST_F(db__query_processor__index_scan__test, verify_cardinality_empty)
{
    auto_transaction_t txn;

    gaia_id_t table_id = type_id_mapping_t::instance().get_table_id(gaia::index_sandbox::empty_t::s_gaia_type);

    for (const index_view_t& index : list_indexes(table_id))
    {
        auto scan = index_scan_t(index.id());
        size_t scan_count = 0;
        for (const auto& row : scan)
        {
            (void)row;
            scan_count++;
        }

        EXPECT_TRUE(scan_count == 0);
    }
}

// Check counts match for all indexes.
TEST_F(db__query_processor__index_scan__test, test_limits)
{
    auto_transaction_t txn;

    size_t limit = c_query_limit_rows;

    gaia_id_t table_id = type_id_mapping_t::instance().get_table_id(gaia::index_sandbox::sandbox_t::s_gaia_type);

    for (const index_view_t& index : list_indexes(table_id))
    {
        // Open an index scan operator for this operator with the fixed limit.
        auto scan = index_scan_t(index.id(), nullptr, limit);
        size_t scan_count = 0;
        for (const auto& p : scan)
        {
            (void)p;
            scan_count++;
        }

        EXPECT_TRUE(limit == scan_count);

        // Open an index scan operator for this operator with limit 0.
        auto scan2 = index_scan_t(index.id(), nullptr, 0);
        scan_count = 0;
        for (const auto& p : scan2)
        {
            (void)p;
            scan_count++;
        }

        EXPECT_TRUE(0 == scan_count);

        // Open an index scan operator for this operator with limit 1.
        auto scan3 = index_scan_t(index.id(), nullptr, 1);
        scan_count = 0;
        for (const auto& p : scan3)
        {
            (void)p;
            scan_count++;
        }

        EXPECT_TRUE(1 == scan_count);

        // Open an index scan operator for this operator greater than the number of rows.
        auto scan4 = index_scan_t(index.id(), nullptr, c_num_initial_rows + 1);
        scan_count = 0;
        for (const auto& p : scan4)
        {
            (void)p;
            scan_count++;
        }

        EXPECT_TRUE(c_num_initial_rows == scan_count);
    }
}

TEST_F(db__query_processor__index_scan__test, query_single_match)
{
    // Lookup index_id for integer field.
    gaia_id_t table_id = type_id_mapping_t::instance().get_table_id(gaia::index_sandbox::sandbox_t::s_gaia_type);
    gaia_id_t range_index_id = c_invalid_gaia_id;
    gaia_id_t hash_index_id = c_invalid_gaia_id;

    auto_transaction_t txn;

    for (const index_view_t& index : list_indexes(table_id))
    {
        for (const gaia::common::gaia_id_t field_id : *index.fields())
        {
            const auto& field = field_view_t(gaia::db::id_to_ptr(field_id));
            {
                range_index_id = index.id();
                break;
            }
            else if (field.data_type() == data_type_t::e_int32 && !field.optional() && index.type() == index_type_t::hash)
            {
                hash_index_id = index.id();
                break;
            }
        }
    }

    EXPECT_TRUE(range_index_id != c_invalid_gaia_id && hash_index_id != c_invalid_gaia_id);

    // Queries the indexes.
    int32_t value = c_num_initial_rows;

    // Equal-range query on range index.
    size_t num_results = 0;
    auto index_key = index_key_t(value);
    auto equal_predicate = std::make_shared<index_equal_range_predicate_t>(index_key);
    for (const auto& scan : index_scan_t(range_index_id, equal_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, 1);

    // Equal-range query on hash index.
    num_results = 0;
    for (const auto& scan : index_scan_t(hash_index_id, equal_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, 1);

    auto point_predicate = std::make_shared<index_point_read_predicate_t>(index_key);

    // Point-query on range index.
    num_results = 0;
    for (const auto& scan : index_scan_t(range_index_id, point_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, 1);

    // Point-query on hash index.
    num_results = 0;
    for (const auto& scan : index_scan_t(hash_index_id, point_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, 1);
}

TEST_F(db__query_processor__index_scan__test, query_multi_match)
{
    // Lookup index_id for integer field.
    gaia_id_t table_id = type_id_mapping_t::instance().get_table_id(gaia::index_sandbox::sandbox_t::s_gaia_type);
    gaia_id_t range_index_id = c_invalid_gaia_id;
    gaia_id_t hash_index_id = c_invalid_gaia_id;

    auto_transaction_t txn;

    for (const index_view_t& index : list_indexes(table_id))
    {
        for (const gaia::common::gaia_id_t field_id : *index.fields())
        {
            const auto& field = field_view_t(gaia::db::id_to_ptr(field_id));
            if (field.data_type() == data_type_t::e_string && index.type() == index_type_t::range)
            {
                range_index_id = index.id();
                break;
            }
            else if (field.data_type() == data_type_t::e_string && index.type() == index_type_t::hash)
            {
                hash_index_id = index.id();
                break;
            }
        }
    }

    EXPECT_TRUE(range_index_id != c_invalid_gaia_id && hash_index_id != c_invalid_gaia_id);

    // Equal-range query on range index.
    size_t num_results = 0;
    auto index_key = index_key_t("");
    auto equal_predicate = std::make_shared<index_equal_range_predicate_t>(index_key);
    for (const auto& scan : index_scan_t(range_index_id, equal_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, c_num_initial_rows);

    // Equal-range query on hash index.
    num_results = 0;
    for (const auto& scan : index_scan_t(hash_index_id, equal_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, c_num_initial_rows);

    auto point_predicate = std::make_shared<index_point_read_predicate_t>(index_key);

    // Point-query on range index.
    num_results = 0;
    for (const auto& scan : index_scan_t(range_index_id, point_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, 1);

    // Point-query on hash index.
    num_results = 0;
    for (const auto& scan : index_scan_t(hash_index_id, point_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, 1);
}

TEST_F(db__query_processor__index_scan__test, query_match_optional)
{
    // Lookup index_id for integer field.
    gaia_id_t table_id = type_id_mapping_t::instance().get_table_id(gaia::index_sandbox::sandbox_t::s_gaia_type);
    gaia_id_t range_index_id = c_invalid_gaia_id;
    gaia_id_t hash_index_id = c_invalid_gaia_id;

    auto_transaction_t txn;

    for (const index_view_t& index : catalog_core::list_indexes(table_id))
    {
        for (const auto field_id : *index.fields())
        {
            const auto& field = field_view_t(gaia::db::id_to_ptr(field_id));

            if (field.data_type() == data_type_t::e_int32 && field.optional() && index.type() == index_type_t::range)
            {
                range_index_id = index.id();
                break;
            }
            else if (field.data_type() == data_type_t::e_int32 && field.optional() && index.type() == index_type_t::hash)
            {
                hash_index_id = index.id();
                break;
            }
        }
    }

    EXPECT_TRUE(range_index_id != c_invalid_gaia_id && hash_index_id != c_invalid_gaia_id);

    // Equal-range query on range index.
    size_t num_results = 0;
    payload_types::data_holder_t nullopt_i32 = payload_types::data_holder_t(reflection::Int, nullptr);
    auto index_key = index_key_t(nullopt_i32);
    auto equal_predicate = std::make_shared<index_equal_range_predicate_t>(index_key);
    for (const auto& scan : index_scan_t(range_index_id, equal_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, c_num_initial_rows);

    // Equal-range query on hash index.
    num_results = 0;
    for (const auto& scan : index_scan_t(hash_index_id, equal_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, c_num_initial_rows);

    auto point_predicate = std::make_shared<index_point_read_predicate_t>(index_key);

    // Point-query on range index.
    num_results = 0;
    for (const auto& scan : index_scan_t(range_index_id, point_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, 1);

    // Point-query on hash index.
    num_results = 0;
    for (const auto& scan : index_scan_t(hash_index_id, point_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, 1);
}

TEST_F(db__query_processor__index_scan__test, query_local_modify_no_match)
{
    // Lookup index_id for integer field.
    gaia_id_t table_id = type_id_mapping_t::instance().get_table_id(gaia::index_sandbox::sandbox_t::s_gaia_type);
    gaia_id_t range_index_id = c_invalid_gaia_id;
    gaia_id_t hash_index_id = c_invalid_gaia_id;

    gaia::db::begin_transaction();

    auto w = gaia::index_sandbox::sandbox_writer();
    w.str = "NO MATCH";
    w.f = 21.0;
    w.i = -1;
    w.insert_row();

    for (const index_view_t& index : list_indexes(table_id))
    {
        for (const gaia::common::gaia_id_t field_id : *index.fields())
        {
            const auto& field = field_view_t(gaia::db::id_to_ptr(field_id));
            if (field.data_type() == data_type_t::e_string && index.type() == index_type_t::range)
            {
                range_index_id = index.id();
                break;
            }
            else if (field.data_type() == data_type_t::e_string && index.type() == index_type_t::hash)
            {
                hash_index_id = index.id();
                break;
            }
        }
    }

    EXPECT_TRUE(range_index_id != c_invalid_gaia_id && hash_index_id != c_invalid_gaia_id);

    // Equal-range query on range index.
    size_t num_results = 0;
    auto index_key = index_key_t("");
    auto equal_predicate = std::make_shared<index_equal_range_predicate_t>(index_key);
    for (const auto& scan : index_scan_t(range_index_id, equal_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, c_num_initial_rows);

    // Equal-range query on hash index.
    num_results = 0;
    for (const auto& scan : index_scan_t(hash_index_id, equal_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, c_num_initial_rows);

    auto point_predicate = std::make_shared<index_point_read_predicate_t>(index_key);

    // Point-query on range index.
    num_results = 0;
    for (const auto& scan : index_scan_t(range_index_id, point_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, 1);

    // Point-query on hash index.
    num_results = 0;
    for (const auto& scan : index_scan_t(hash_index_id, point_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, 1);

    gaia::db::rollback_transaction();
}

TEST_F(db__query_processor__index_scan__test, query_local_modify_match)
{
    // Lookup index_id for integer field.
    gaia_id_t table_id = type_id_mapping_t::instance().get_table_id(gaia::index_sandbox::sandbox_t::s_gaia_type);
    gaia_id_t range_index_id = c_invalid_gaia_id;
    gaia_id_t hash_index_id = c_invalid_gaia_id;

    gaia::db::begin_transaction();

    auto w = gaia::index_sandbox::sandbox_writer();
    w.str = "";
    w.f = 21.0;
    w.i = -1;
    w.insert_row();

    for (const index_view_t& index : list_indexes(table_id))
    {
        for (const gaia::common::gaia_id_t field_id : *index.fields())
        {
            const auto& field = field_view_t(gaia::db::id_to_ptr(field_id));
            if (field.data_type() == data_type_t::e_string && index.type() == index_type_t::range)
            {
                range_index_id = index.id();
                break;
            }
            else if (field.data_type() == data_type_t::e_string && index.type() == index_type_t::hash)
            {
                hash_index_id = index.id();
                break;
            }
        }
    }

    EXPECT_TRUE(range_index_id != c_invalid_gaia_id && hash_index_id != c_invalid_gaia_id);

    // Equal-range query on range index.
    size_t num_results = 0;
    auto index_key = index_key_t("");
    auto equal_predicate = std::make_shared<index_equal_range_predicate_t>(index_key);
    for (const auto& scan : index_scan_t(range_index_id, equal_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, c_num_initial_rows + 1);

    // Equal-range query on hash index.
    num_results = 0;
    for (const auto& scan : index_scan_t(hash_index_id, equal_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, c_num_initial_rows + 1);

    auto point_predicate = std::make_shared<index_point_read_predicate_t>(index_key);

    // Point-query on range index.
    num_results = 0;
    for (const auto& scan : index_scan_t(range_index_id, point_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, 1);

    // Point-query on hash index.
    num_results = 0;
    for (const auto& scan : index_scan_t(hash_index_id, point_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, 1);

    gaia::db::rollback_transaction();
}

TEST_F(test_index_scan, query_rang_inclusive)
{
    // Lookup index_id for integer field.
    gaia_id_t type_record_id = type_id_mapping_t::instance().get_record_id(gaia::index_sandbox::sandbox_t::s_gaia_type);
    gaia_id_t range_index_id = c_invalid_gaia_id;
    gaia_id_t hash_index_id = c_invalid_gaia_id;

    gaia::db::begin_transaction();

    for (const index_view_t& index : catalog_core_t::list_indexes(type_record_id))
    {
        for (const gaia::common::gaia_id_t field_id : *index.fields())
        {
            const auto field = field_view_t(gaia::db::id_to_ptr(field_id));
            if (field.data_type() == data_type_t::e_int32 && index.type() == index_type_t::range)
            {
                range_index_id = index.id();
                break;
            }
            else if (field.data_type() == data_type_t::e_int32 && index.type() == index_type_t::hash)
            {
                hash_index_id = index.id();
                break;
            }
        }
    }

    EXPECT_TRUE(range_index_id != c_invalid_gaia_id);
    EXPECT_TRUE(hash_index_id != c_invalid_gaia_id);

    auto lower_key = std::make_optional<index_key_t>(1);
    auto upper_key = std::make_optional<index_key_t>(5);
    size_t num_results = 0;
    bool is_inclusive = true;

    auto lower_bound_inclusive = range_bound_t(lower_key, is_inclusive);
    auto upper_bound_inclusive = range_bound_t(upper_key, is_inclusive);

    auto null_bound = range_bound_t(std::nullopt, is_inclusive);

    auto range_predicate
        = std::make_shared<index_range_predicate_t>(range_index_id, lower_bound_inclusive, upper_bound_inclusive);

    for (const auto& scan : index_scan_t(range_index_id, range_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, c_num_inclusive_rows);

    num_results = 0;
    range_predicate
        = std::make_shared<index_range_predicate_t>(range_index_id, null_bound, null_bound);
    for (const auto& scan : index_scan_t(range_index_id, range_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, c_num_initial_rows);

    range_predicate
        = std::make_shared<index_range_predicate_t>(hash_index_id, null_bound, null_bound);
    ASSERT_THROW(index_scan_t(hash_index_id, range_predicate).begin(), index_operation_not_supported);

    auto w = gaia::index_sandbox::sandbox_writer();
    w.str = "";
    w.f = 21.0;
    w.i = 5;
    w.insert_row();

    range_predicate
        = std::make_shared<index_range_predicate_t>(range_index_id, lower_bound_inclusive, upper_bound_inclusive);

    num_results = 0;
    for (const auto& scan : index_scan_t(range_index_id, range_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, c_num_inclusive_rows + 1);

    gaia::db::rollback_transaction();
}

TEST_F(test_index_scan, query_range_exclusive)
{
    // Lookup index_id for integer field.
    gaia_id_t type_record_id = type_id_mapping_t::instance().get_record_id(gaia::index_sandbox::sandbox_t::s_gaia_type);
    gaia_id_t range_index_id = c_invalid_gaia_id;
    gaia_id_t hash_index_id = c_invalid_gaia_id;

    gaia::db::begin_transaction();

    for (const index_view_t& index : catalog_core_t::list_indexes(type_record_id))
    {
        for (const gaia::common::gaia_id_t field_id : *index.fields())
        {
            const auto field = field_view_t(gaia::db::id_to_ptr(field_id));
            if (field.data_type() == data_type_t::e_int32 && index.type() == index_type_t::range)
            {
                range_index_id = index.id();
                break;
            }
            else if (field.data_type() == data_type_t::e_int32 && index.type() == index_type_t::hash)
            {
                hash_index_id = index.id();
                break;
            }
        }
    }

    EXPECT_TRUE(range_index_id != c_invalid_gaia_id);
    EXPECT_TRUE(hash_index_id != c_invalid_gaia_id);

    auto lower_key = std::make_optional<index_key_t>(1);
    auto upper_key = std::make_optional<index_key_t>(5);

    bool is_inclusive = false;

    auto lower_bound_exclusive = range_bound_t(lower_key, is_inclusive);
    auto upper_bound_exclusive = range_bound_t(upper_key, is_inclusive);

    auto null_bound = range_bound_t(std::nullopt, is_inclusive);

    size_t num_results = 0;
    auto range_predicate
        = std::make_shared<index_range_predicate_t>(range_index_id, lower_bound_exclusive, upper_bound_exclusive);
    for (const auto& scan : index_scan_t(range_index_id, range_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, c_num_exclusive_rows);

    num_results = 0;
    range_predicate
        = std::make_shared<index_range_predicate_t>(range_index_id, null_bound, null_bound);
    for (const auto& scan : index_scan_t(range_index_id, range_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, c_num_initial_rows);

    range_predicate
        = std::make_shared<index_range_predicate_t>(hash_index_id, null_bound, null_bound);
    ASSERT_THROW(index_scan_t(hash_index_id, range_predicate).begin(), index_operation_not_supported);

    auto w = gaia::index_sandbox::sandbox_writer();
    w.str = "";
    w.f = 21.0;
    w.i = 5;
    w.insert_row();

    range_predicate
        = std::make_shared<index_range_predicate_t>(range_index_id, lower_bound_exclusive, upper_bound_exclusive);

    num_results = 0;
    for (const auto& scan : index_scan_t(range_index_id, range_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, c_num_exclusive_rows);

    gaia::db::rollback_transaction();
}

TEST_F(db__query_processor__index_scan__test, query_no_match)
{
    // Lookup index_id for integer field.
    gaia_id_t table_id = type_id_mapping_t::instance().get_table_id(gaia::index_sandbox::sandbox_t::s_gaia_type);
    gaia_id_t range_index_id = c_invalid_gaia_id;
    gaia_id_t hash_index_id = c_invalid_gaia_id;

    auto_transaction_t txn;

    for (const index_view_t& index : list_indexes(table_id))
    {
        for (const gaia::common::gaia_id_t field_id : *index.fields())
        {
            const auto& field = field_view_t(gaia::db::id_to_ptr(field_id));
            if (field.data_type() == data_type_t::e_string && index.type() == index_type_t::range)
            {
                range_index_id = index.id();
                break;
            }
            else if (field.data_type() == data_type_t::e_string && index.type() == index_type_t::hash)
            {
                hash_index_id = index.id();
                break;
            }
        }
    }

    EXPECT_TRUE(range_index_id != c_invalid_gaia_id && hash_index_id != c_invalid_gaia_id);

    // Equal-range query on range index.
    size_t num_results = 0;
    auto index_key = index_key_t(c_no_match_str);
    auto equal_predicate = std::make_shared<index_equal_range_predicate_t>(index_key);
    for (const auto& scan : index_scan_t(range_index_id, equal_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, 0);

    // Equal-range query on hash index.
    num_results = 0;
    for (const auto& scan : index_scan_t(hash_index_id, equal_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, 0);

    auto point_predicate = std::make_shared<index_point_read_predicate_t>(index_key);

    // Point-query on range index.
    num_results = 0;
    for (const auto& scan : index_scan_t(range_index_id, point_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, 0);

    // Point-query on hash index.
    num_results = 0;
    for (const auto& scan : index_scan_t(hash_index_id, point_predicate))
    {
        (void)scan;
        ++num_results;
    }

    EXPECT_EQ(num_results, 0);
}

TEST_F(db__query_processor__index_scan__test, rollback_txn)
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
    gaia_id_t table_id = type_id_mapping_t::instance().get_table_id(gaia::index_sandbox::sandbox_t::s_gaia_type);

    for (const index_view_t& index : list_indexes(table_id))
    {
        // Open an index scan operator for this operator.
        auto scan = index_scan_t(index.id());
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

TEST_F(db__query_processor__index_scan__test, insert_followed_by_delete)
{
    gaia::common::gaia_id_t to_delete;

    gaia::db::begin_transaction();
    auto w = gaia::index_sandbox::sandbox_writer();
    w.str = "HELLO";
    w.f = 21.0;
    w.i = -1;
    to_delete = w.insert_row();

    // Insert should be visible.
    gaia_id_t table_id = type_id_mapping_t::instance().get_table_id(gaia::index_sandbox::sandbox_t::s_gaia_type);

    for (const index_view_t& index : list_indexes(table_id))
    {
        // Open an index scan operator for this operator.
        auto scan = index_scan_t(index.id());
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
    for (const index_view_t& index : list_indexes(table_id))
    {
        // Open an index scan operator for this operator.
        auto scan = index_scan_t(index.id());
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

TEST_F(db__query_processor__index_scan__test, multi_insert_followed_by_delete)
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
    gaia_id_t table_id = type_id_mapping_t::instance().get_table_id(gaia::index_sandbox::sandbox_t::s_gaia_type);

    for (const index_view_t& index : list_indexes(table_id))
    {
        // Open an index scan operator for this operator.
        auto scan = index_scan_t(index.id());
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
    for (const index_view_t& index : list_indexes(table_id))
    {
        // Open an index scan operator for this operator.
        auto scan = index_scan_t(index.id());
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
