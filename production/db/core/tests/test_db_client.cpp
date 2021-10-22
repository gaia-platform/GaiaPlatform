/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <gtest/gtest.h>

#include "gaia/db/db.hpp"

#include "gaia_internal/catalog/catalog.hpp"
#include "gaia_internal/catalog/gaia_catalog.h"
#include "gaia_internal/db/db_test_base.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"
#include "gaia_internal/db/type_metadata.hpp"

#include "type_id_mapping.hpp"

using namespace gaia::db;
using namespace gaia::common;
using namespace gaia::catalog;

// duplicated from production/db/core/inc/db_server.hpp
constexpr size_t c_stream_batch_size = 1 << 10;

void print_payload(std::ostream& o, size_t size, const char* payload)
{
    if (size)
    {
        o << " Payload: ";
    }

    for (size_t i = 0; i < size; i++)
    {
        if ('\\' == payload[i])
        {
            o << "\\\\";
        }
        else if (isprint(payload[i]))
        {
            o << payload[i];
        }
        else
        {
            o << "\\x" << std::setw(2) << std::setfill('0') << std::hex << short(payload[i]) << std::dec;
        }
    }
}

void print_node(const gaia_ptr_t& node, bool indent = false)
{
    if (!node)
    {
        return;
    }

    std::cerr << std::endl;

    if (indent)
    {
        std::cerr << "  ";
    }

    std::cerr
        << "Node id:"
        << node.id() << ", type:"
        << node.type();

    print_payload(std::cerr, node.data_size(), node.data());

    if (indent)
    {
        return;
    }

    std::cerr << std::endl;
}

/**
 * Google test fixture object.  This class is used by each
 * test case below.  SetUp() is called before each test is run
 * and TearDown() is called after each test case is done.
 */
class db_client_test : public db_test_base_t
{
private:
    void init_data()
    {
        node1_id = gaia_ptr_t::generate_id();
        node2_id = gaia_ptr_t::generate_id();
        node3_id = gaia_ptr_t::generate_id();
        node4_id = gaia_ptr_t::generate_id();

        gaia_id_t type1_table_id = create_table("test1", {});
        gaia_id_t type2_table_id = create_table("test2", {});

        begin_transaction();
        {
            type1 = gaia_table_t::get(type1_table_id).type();
            type2 = gaia_table_t::get(type2_table_id).type();

            std::cerr << std::endl;
            std::cerr << "*** create test nodes" << std::endl;
            gaia_ptr_t node1 = gaia_ptr_t::create(node1_id, type1, 0, nullptr);
            print_node(node1);
            gaia_ptr_t node2 = gaia_ptr_t::create(node2_id, type1, 0, nullptr);
            print_node(node2);
            gaia_ptr_t node3 = gaia_ptr_t::create(node3_id, type2, 0, nullptr);
            print_node(node3);
            gaia_ptr_t node4 = gaia_ptr_t::create(node4_id, type2, 0, nullptr);
            print_node(node4);
        }
        commit_transaction();
    }

protected:
    gaia_id_t node1_id;
    gaia_id_t node2_id;
    gaia_id_t node3_id;
    gaia_id_t node4_id;
    gaia_type_t type1;
    gaia_type_t type2;

    void SetUp() override
    {
        db_test_base_t::SetUp();
        init_data();
    }
};

TEST_F(db_client_test, early_session_termination)
{
    // Test that closing the session after starting a transaction
    // does not generate any internal assertion failures
    // when attempting to reopen a session.
    begin_transaction();
    EXPECT_THROW(end_session(), transaction_in_progress);
    EXPECT_THROW(begin_session(), session_exists);
    rollback_transaction();
}

TEST_F(db_client_test, creation_fail_for_invalid_type)
{
    begin_transaction();
    {
        const gaia_id_t c_invalid_id = 8888;
        EXPECT_THROW(gaia_ptr_t::create(c_invalid_id, 0, 0), invalid_type);
    }
    commit_transaction();
}

TEST_F(db_client_test, gaia_ptr_no_transaction_fail)
{
    begin_transaction();
    gaia_ptr_t node1 = gaia_ptr_t::open(node1_id);
    commit_transaction();

    // Create with existent type fail
    EXPECT_THROW(gaia_ptr_t::create(type1, 0, ""), no_open_transaction);
    EXPECT_THROW(gaia_ptr_t::create(99999, type1, 0, ""), no_open_transaction);
    EXPECT_THROW(gaia_ptr_t::create(99999, type1, 5, 0, ""), no_open_transaction);
    EXPECT_THROW(gaia_ptr_t::open(node1_id), no_open_transaction);
    EXPECT_THROW(node1.id(), no_open_transaction);
    EXPECT_THROW(node1.type(), no_open_transaction);
    EXPECT_THROW(node1.data_size(), no_open_transaction);
    EXPECT_THROW(node1.references(), no_open_transaction);
    EXPECT_THROW(node1.find_next(), no_open_transaction);
    EXPECT_THROW(node1.update_payload(0, ""), no_open_transaction);
    EXPECT_THROW(node1.add_child_reference(1, 2), no_open_transaction);
    EXPECT_THROW(node1.add_parent_reference(1, 2), no_open_transaction);
    EXPECT_THROW(node1.remove_child_reference(1, 2), no_open_transaction);
    EXPECT_THROW(node1.remove_parent_reference(2), no_open_transaction);
    EXPECT_THROW(node1.update_parent_reference(1, 2), no_open_transaction);
    EXPECT_THROW(gaia_ptr_t::remove(node1), no_open_transaction);

    // Test with non existent type
    // TODO there is a bug in GNU libstdc that will the type_id_mapping hang if the initialization
    //  throws an exception and call_once is called again. Disabling the tests for now.
    //  see type_id_mapping_t for more details.
    //    gaia_type_t type3 = 3;
    //    EXPECT_THROW(gaia_ptr_t::create(type3, 0, 0), transaction_not_open);
    //    EXPECT_THROW(gaia_ptr_t::create(99999, type3, 0, 0), transaction_not_open);
    //    EXPECT_THROW(gaia_ptr_t::create(99999, type3, 5, 0, 0), transaction_not_open);
    //    EXPECT_THROW(gaia_ptr_t::open(99999), transaction_not_open);
}

TEST_F(db_client_test, read_data)
{
    begin_transaction();
    {
        std::cerr << std::endl;
        std::cerr << "*** Update payload and verify" << std::endl;
        gaia_ptr_t node1 = gaia_ptr_t::open(node1_id);
        gaia_ptr_t node2 = gaia_ptr_t::open(node2_id);
        gaia_ptr_t node3 = gaia_ptr_t::open(node3_id);
        gaia_ptr_t node4 = gaia_ptr_t::open(node4_id);
        print_node(node1);
        print_node(node2);
        print_node(node3);
        print_node(node4);
        EXPECT_EQ(node1.id(), node1_id);
        EXPECT_EQ(node2.id(), node2_id);
        EXPECT_EQ(node3.id(), node3_id);
        EXPECT_EQ(node4.id(), node4_id);
    }
    commit_transaction();
}

TEST_F(db_client_test, update_payload)
{
    auto payload = "payload str";
    begin_transaction();
    {
        std::cerr << std::endl;
        std::cerr << "*** Update payload and verify" << std::endl;
        gaia_ptr_t node1 = gaia_ptr_t::open(node1_id);
        print_node(node1);
        node1.update_payload(strlen(payload), payload);
        print_node(node1);
        EXPECT_STREQ(node1.data(), payload);
    }
    commit_transaction();

    begin_transaction();
    {
        std::cerr << std::endl;
        std::cerr << "*** Reload data and verify update" << std::endl;
        gaia_ptr_t node1 = gaia_ptr_t::open(node1_id);
        print_node(node1);
        EXPECT_STREQ(node1.data(), payload);
    }
    commit_transaction();
}

TEST_F(db_client_test, update_payload_rollback)
{
    auto payload = "payload str";
    begin_transaction();
    {
        std::cerr << std::endl;
        std::cerr << "*** Update payload and verify" << std::endl;
        gaia_ptr_t node1 = gaia_ptr_t::open(node1_id);
        print_node(node1);
        node1.update_payload(strlen(payload), payload);
        print_node(node1);
        EXPECT_STREQ(node1.data(), payload);
    }
    rollback_transaction();

    begin_transaction();
    {
        std::cerr << std::endl;
        std::cerr << "*** Reload data and verify update" << std::endl;
        gaia_ptr_t node1 = gaia_ptr_t::open(node1_id);
        print_node(node1);
        EXPECT_EQ(node1.data(), nullptr);
    }
    commit_transaction();
}

TEST_F(db_client_test, iterate_type)
{
    begin_transaction();
    {
        std::cerr << std::endl;
        std::cerr << "*** Iterating over nodes of type 1:" << std::endl;

        gaia_type_t type = type1;
        gaia_id_t id = node1_id;
        for (auto node_iter = gaia_ptr_t::find_first(type);
             node_iter;
             node_iter = node_iter.find_next())
        {
            print_node(node_iter);
            EXPECT_EQ(node_iter.id(), id);
            id++;
        }

        std::cerr << std::endl;
        std::cerr << "*** Iterating over nodes of type 2:" << std::endl;
        type = type2;
        for (auto node_iter = gaia_ptr_t::find_first(type);
             node_iter;
             node_iter = node_iter.find_next())
        {
            print_node(node_iter);
            EXPECT_EQ(node_iter.id(), id);
            id++;
        }
    }
    commit_transaction();
}

constexpr size_t c_buffer_size_exact = c_stream_batch_size;
constexpr size_t c_buffer_size_exact_multiple = c_stream_batch_size * 2;
constexpr size_t c_buffer_size_inexact_multiple = c_stream_batch_size * 2 + 3;
constexpr size_t c_buffer_size_minus_one = c_stream_batch_size - 1;
constexpr size_t c_buffer_size_plus_one = c_stream_batch_size + 1;

constexpr int c_total_test_types = 7;
std::array<gaia_type_t, c_total_test_types> g_test_types;

void iterate_test_create_nodes()
{
    std::vector<gaia_id_t> table_ids;
    for (size_t i = 0; i < c_total_test_types; i++)
    {
        gaia_id_t id = create_table("test_table" + std::to_string(i), {});
        table_ids.push_back(id);
    }

    // Reset cache otherwise it will not load.
    type_id_mapping_t::instance().clear();

    begin_transaction();
    for (size_t i = 0; i < c_total_test_types; i++)
    {
        g_test_types[i] = gaia_table_t::get(table_ids[i]).type();
    }

    std::cerr << "*** Creating nodes for cursor test..." << std::endl;

    size_t test_type_index = 1;

    // Create objects for iterator test.
    // "One node" test.
    gaia_ptr_t::create(gaia_ptr_t::generate_id(), g_test_types[test_type_index], 0, nullptr);

    test_type_index++;
    // "Exact buffer size" test.
    for (size_t i = 0; i < c_buffer_size_exact; i++)
    {
        gaia_ptr_t::create(gaia_ptr_t::generate_id(), g_test_types[test_type_index], 0, nullptr);
    }

    test_type_index++;
    // "Exact multiple of buffer size" test
    for (size_t i = 0; i < c_buffer_size_exact_multiple; i++)
    {
        gaia_ptr_t::create(gaia_ptr_t::generate_id(), g_test_types[test_type_index], 0, nullptr);
    }

    test_type_index++;
    // "Inexact multiple of buffer size" test
    for (size_t i = 0; i < c_buffer_size_inexact_multiple; i++)
    {
        gaia_ptr_t::create(gaia_ptr_t::generate_id(), g_test_types[test_type_index], 0, nullptr);
    }

    test_type_index++;
    // "One less than buffer size" test
    for (size_t i = 0; i < c_buffer_size_minus_one; i++)
    {
        gaia_ptr_t::create(gaia_ptr_t::generate_id(), g_test_types[test_type_index], 0, nullptr);
    }

    test_type_index++;
    // "One more than buffer size" test
    for (size_t i = 0; i < c_buffer_size_plus_one; i++)
    {
        gaia_ptr_t::create(gaia_ptr_t::generate_id(), g_test_types[test_type_index], 0, nullptr);
    }
    commit_transaction();
}

void iterate_test_validate_iterations()
{
    size_t count, expected_count;

    size_t test_type_index = 0;

    std::cerr << std::endl;
    std::cerr << "*** Iterating over empty type:" << std::endl;
    gaia_type_t type = g_test_types[test_type_index];
    count = 0;
    expected_count = 0;
    for (auto node : gaia_ptr_t::find_all_range(type))
    {
        EXPECT_EQ(node.type(), type);
        count++;
    }
    EXPECT_EQ(count, expected_count);

    std::cerr << std::endl;
    std::cerr << "*** Iterating over one node in type:" << std::endl;
    type = g_test_types[++test_type_index];
    count = 0;
    expected_count = 1;
    for (auto node : gaia_ptr_t::find_all_range(type))
    {
        EXPECT_EQ(node.type(), type);
        count++;
    }
    EXPECT_EQ(count, expected_count);

    std::cerr << std::endl;
    std::cerr << "*** Iterating over nodes with exact buffer size:" << std::endl;
    type = g_test_types[++test_type_index];
    count = 0;
    expected_count = c_buffer_size_exact;
    for (auto node : gaia_ptr_t::find_all_range(type))
    {
        EXPECT_EQ(node.type(), type);
        count++;
    }
    EXPECT_EQ(count, expected_count);

    std::cerr << std::endl;
    std::cerr << "*** Iterating over nodes with exact multiple of buffer size:" << std::endl;
    type = g_test_types[++test_type_index];
    count = 0;
    expected_count = c_buffer_size_exact_multiple;
    for (auto node : gaia_ptr_t::find_all_range(type))
    {
        EXPECT_EQ(node.type(), type);
        count++;
    }
    EXPECT_EQ(count, expected_count);

    std::cerr << std::endl;
    std::cerr << "*** Iterating over nodes with inexact multiple of buffer size:" << std::endl;
    type = g_test_types[++test_type_index];
    count = 0;
    expected_count = c_buffer_size_inexact_multiple;
    for (auto node : gaia_ptr_t::find_all_range(type))
    {
        EXPECT_EQ(node.type(), type);
        count++;
    }
    EXPECT_EQ(count, expected_count);

    std::cerr << std::endl;
    std::cerr << "*** Iterating over nodes with one less than buffer size:" << std::endl;
    type = g_test_types[++test_type_index];
    count = 0;
    expected_count = c_buffer_size_minus_one;
    for (auto node : gaia_ptr_t::find_all_range(type))
    {
        EXPECT_EQ(node.type(), type);
        count++;
    }
    EXPECT_EQ(count, expected_count);

    std::cerr << std::endl;
    std::cerr << "*** Iterating over nodes with one more than buffer size:" << std::endl;
    type = g_test_types[++test_type_index];
    count = 0;
    expected_count = c_buffer_size_plus_one;
    for (auto node : gaia_ptr_t::find_all_range(type))
    {
        EXPECT_EQ(node.type(), type);
        count++;
    }
    EXPECT_EQ(count, expected_count);

    std::cerr << std::endl;
}

TEST_F(db_client_test, iterate_type_cursor_separate_txn)
{
    // Test that we can see additions across transactions.
    iterate_test_create_nodes();

    begin_transaction();
    {
        iterate_test_validate_iterations();
    }
    commit_transaction();
}

TEST_F(db_client_test, iterate_type_cursor_same_txn)
{
    iterate_test_create_nodes();

    // Test that we can see additions in the transaction that made them.
    begin_transaction();
    {
        iterate_test_validate_iterations();
    }
    commit_transaction();
}

TEST_F(db_client_test, iterate_type_delete)
{
    begin_transaction();
    {
        std::cerr << std::endl;
        std::cerr << "*** Iterating over nodes of type 1 before delete:" << std::endl;
        auto node_iter = gaia_ptr_t::find_first(type1);
        print_node(node_iter);
        EXPECT_EQ(node_iter.id(), node1_id);
        std::cerr << std::endl;
        std::cerr << "*** Preparing to delete first node of type 1:" << std::endl;
        gaia_ptr_t::remove(node_iter);
        std::cerr << "*** Iterating over nodes of type 1 after delete:" << std::endl;
        node_iter = gaia_ptr_t::find_first(type1);
        print_node(node_iter);
        EXPECT_EQ(node_iter.id(), node2_id);
    }
    commit_transaction();

    begin_transaction();
    {
        std::cerr << std::endl;
        std::cerr << "*** Reloading data: iterating over nodes of type 1 after delete:" << std::endl;
        auto node_iter = gaia_ptr_t::find_first(type1);
        print_node(node_iter);
        EXPECT_EQ(node_iter.id(), node2_id);
    }
    commit_transaction();
}

TEST_F(db_client_test, null_payload_check)
{
    begin_transaction();
    {
        constexpr size_t c_test_payload_size = 50;

        constexpr size_t c_num_refs = 50;
        std::cerr << std::endl;
        std::cerr << "*** Creating a zero-length node with references:" << std::endl;
        EXPECT_NE(gaia_ptr_t::create(gaia_ptr_t::generate_id(), type1, c_num_refs, 0, nullptr), nullptr);

        std::cerr << std::endl;
        std::cerr << "*** Creating a zero-length node without references:" << std::endl;
        EXPECT_NE(gaia_ptr_t::create(gaia_ptr_t::generate_id(), type1, 0, 0, nullptr), nullptr);

        std::cerr << std::endl;
        std::cerr << "*** Creating a node with no payload and non-zero payload size (error):" << std::endl;
        EXPECT_THROW(gaia_ptr_t::create(gaia_ptr_t::generate_id(), type1, c_num_refs, c_test_payload_size, nullptr), retail_assertion_failure);
    }
    commit_transaction();
}

TEST_F(db_client_test, create_large_object)
{
    begin_transaction();
    {
        uint8_t payload[c_db_object_max_payload_size];

        size_t num_refs = 50;
        size_t payload_size = c_db_object_max_payload_size - (num_refs * sizeof(gaia_id_t));
        std::cerr << std::endl;
        std::cerr << "*** Creating the largest node (" << payload_size << " bytes):" << std::endl;
        EXPECT_NE(gaia_ptr_t::create(gaia_ptr_t::generate_id(), type1, num_refs, payload_size, payload), nullptr);

        num_refs = 51;
        std::cerr << std::endl;
        std::cerr << "*** Creating a too-large node (" << payload_size + sizeof(gaia_id_t) << " bytes):" << std::endl;
        EXPECT_THROW(gaia_ptr_t::create(gaia_ptr_t::generate_id(), type1, num_refs, payload_size, payload), object_too_large);
    }
    commit_transaction();
}
