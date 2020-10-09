/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"

#include "gaia_db.hpp"
#include "gaia_ptr.hpp"
#include "db_test_base.hpp"

using namespace gaia::db;

// duplicated from production/db/storage_engine/inc/storage_engine_server.hpp
constexpr size_t STREAM_BATCH_SIZE = 1 << 10;

void print_payload(std::ostream &o, const size_t size, const char *payload) {
    if (size) {
        o << " Payload: ";
    }

    for (size_t i = 0; i < size; i++) {
        if ('\\' == payload[i]) {
            o << "\\\\";
        } else if (isprint(payload[i])) {
            o << payload[i];
        } else {
            o << "\\x" << std::setw(2) << std::setfill('0') << std::hex << short(payload[i]) << std::dec;
        }
    }
}

void print_node(const gaia_ptr &node, const bool indent = false) {
    if (!node) {
        return;
    }

    std::cerr << std::endl;

    if (indent) {
        std::cerr << "  ";
    }

    std::cerr
        << "Node id:"
        << node.id() << ", type:"
        << node.type();

    print_payload(std::cerr, node.data_size(), node.data());

    if (indent) {
        return;
    }

    std::cerr << std::endl;
}

gaia_id_t node1_id = 1;
gaia_id_t node2_id = 2;
gaia_id_t node3_id = 3;
gaia_id_t node4_id = 4;

gaia_type_t type1 = 1;
gaia_type_t type2 = 2;

/**
 * Google test fixture object.  This class is used by each
 * test case below.  SetUp() is called before each test is run
 * and TearDown() is called after each test case is done.
 */
class storage_engine_client_test : public db_test_base_t {
  private:
    void init_data() {
        begin_transaction();
        {
            std::cerr << std::endl;
            std::cerr << "*** create test nodes" << std::endl;
            gaia_ptr node1 = gaia_ptr::create(node1_id, type1, 0, 0);
            print_node(node1);
            gaia_ptr node2 = gaia_ptr::create(node2_id, type1, 0, 0);
            print_node(node2);
            gaia_ptr node3 = gaia_ptr::create(node3_id, type2, 0, 0);
            print_node(node3);
            gaia_ptr node4 = gaia_ptr::create(node4_id, type2, 0, 0);
            print_node(node4);
        }
        commit_transaction();
    }

  protected:
    void SetUp() override {
        db_test_base_t::SetUp();
        init_data();
    }
};

TEST_F(storage_engine_client_test, read_data) {
    begin_transaction();
    {
        std::cerr << std::endl;
        std::cerr << "*** Update payload and verify" << std::endl;
        gaia_ptr node1 = gaia_ptr::open(node1_id);
        gaia_ptr node2 = gaia_ptr::open(node2_id);
        gaia_ptr node3 = gaia_ptr::open(node3_id);
        gaia_ptr node4 = gaia_ptr::open(node4_id);
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

TEST_F(storage_engine_client_test, update_payload) {
    auto payload = "payload str";
    begin_transaction();
    {
        std::cerr << std::endl;
        std::cerr << "*** Update payload and verify" << std::endl;
        gaia_ptr node1 = gaia_ptr::open(node1_id);
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
        gaia_ptr node1 = gaia_ptr::open(node1_id);
        print_node(node1);
        EXPECT_STREQ(node1.data(), payload);
    }
    commit_transaction();
}

TEST_F(storage_engine_client_test, update_payload_rollback) {
    auto payload = "payload str";
    begin_transaction();
    {
        std::cerr << std::endl;
        std::cerr << "*** Update payload and verify" << std::endl;
        gaia_ptr node1 = gaia_ptr::open(node1_id);
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
        gaia_ptr node1 = gaia_ptr::open(node1_id);
        print_node(node1);
        EXPECT_EQ(node1.data(), nullptr);
    }
    commit_transaction();
}

TEST_F(storage_engine_client_test, iterate_type) {
    begin_transaction();
    {
        std::cerr << std::endl;
        std::cerr << "*** Iterating over nodes of type 1:" << std::endl;

        gaia_type_t type = type1;
        gaia_id_t id = node1_id;
        for (auto node_iter = gaia_ptr::find_first(type);
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
        for (auto node_iter = gaia_ptr::find_first(type);
            node_iter;
            node_iter = node_iter.find_next())
        {
            print_node(node_iter);
            EXPECT_EQ(node_iter.id(), id);
            id++;
        }
TEST_F(storage_engine_client_test, iterate_type_cursor) {
    constexpr size_t BUFFER_SIZE_EXACT = STREAM_BATCH_SIZE;
    constexpr size_t BUFFER_SIZE_EXACT_MULTIPLE = STREAM_BATCH_SIZE * 2;
    constexpr size_t BUFFER_SIZE_INEXACT_MULTIPLE = STREAM_BATCH_SIZE * 2 + 3;
    constexpr size_t BUFFER_SIZE_MINUS_ONE = STREAM_BATCH_SIZE - 1;
    constexpr size_t BUFFER_SIZE_PLUS_ONE = STREAM_BATCH_SIZE + 1;

    begin_transaction();
    {
        std::cerr << "*** Creating nodes for cursor test..." << std::endl;

        // create objects for iterator test
        gaia_id_t next_id = 5;
        // skip type ID 3 for "zero nodes" test
        // "one node" test
        gaia_id_t next_type = 4;
        gaia_ptr::create(next_id++, next_type, 0, 0);
        // "exact buffer size" test
        ++next_type;
        for (size_t i = 0; i < BUFFER_SIZE_EXACT; i++) {
            gaia_ptr::create(next_id++, next_type, 0, 0);
        }
        // "exact multiple of buffer size" test
        ++next_type;
        for (size_t i = 0; i < BUFFER_SIZE_EXACT_MULTIPLE; i++) {
            gaia_ptr::create(next_id++, next_type, 0, 0);
        }
        // "inexact multiple of buffer size" test
        ++next_type;
        for (size_t i = 0; i < BUFFER_SIZE_INEXACT_MULTIPLE; i++) {
            gaia_ptr::create(next_id++, next_type, 0, 0);
        }
        // "one less than buffer size" test
        ++next_type;
        for (size_t i = 0; i < BUFFER_SIZE_MINUS_ONE; i++) {
            gaia_ptr::create(next_id++, next_type, 0, 0);
        }
        // "one more than buffer size" test
        ++next_type;
        for (size_t i = 0; i < BUFFER_SIZE_PLUS_ONE; i++) {
            gaia_ptr::create(next_id++, next_type, 0, 0);
        }
    }
    commit_transaction();

    begin_transaction();
    {
        gaia_id_t id = 1;
        gaia_type_t type = 1;

        std::cerr << std::endl;
        std::cerr << "*** Iterating over nodes of type 1:" << std::endl;
        for (auto node : gaia_ptr::find_all_range(type)) {
            print_node(node);
            EXPECT_EQ(node.id(), id);
            id++;
        }

        std::cerr << std::endl;
        std::cerr << "*** Iterating over nodes of type 2:" << std::endl;
        type = 2;
        for (auto node : gaia_ptr::find_all_range(type)) {
            print_node(node);
            EXPECT_EQ(node.id(), id);
            id++;
        }

        std::cerr << std::endl;
        std::cerr << "*** Iterating over nodes in type with predicate:" << std::endl;
        type = 1;
        for (auto node : gaia_ptr::find_all_range(type,
                 [](gaia_ptr ptr) { return ptr.id() == 1; })) {
            print_node(node);
            EXPECT_EQ(node.id(), 1);
        }

        size_t count, expected_count;

        std::cerr << std::endl;
        std::cerr << "*** Iterating over empty type:" << std::endl;
        type = 3;
        count = 0;
        expected_count = 0;
        for (auto node : gaia_ptr::find_all_range(type)) {
            EXPECT_EQ(node.type(), type);
            count++;
        }
        EXPECT_EQ(count, expected_count);

        std::cerr << std::endl;
        std::cerr << "*** Iterating over one node in type:" << std::endl;
        type = 4;
        count = 0;
        expected_count = 1;
        for (auto node : gaia_ptr::find_all_range(type)) {
            EXPECT_EQ(node.type(), type);
            count++;
        }
        EXPECT_EQ(count, expected_count);

        std::cerr << std::endl;
        std::cerr << "*** Iterating over nodes with exact buffer size:" << std::endl;
        type = 5;
        count = 0;
        expected_count = BUFFER_SIZE_EXACT;
        for (auto node : gaia_ptr::find_all_range(type)) {
            EXPECT_EQ(node.type(), type);
            count++;
        }
        EXPECT_EQ(count, expected_count);

        std::cerr << std::endl;
        std::cerr << "*** Iterating over nodes with exact multiple of buffer size:" << std::endl;
        type = 6;
        count = 0;
        expected_count = BUFFER_SIZE_EXACT_MULTIPLE;
        for (auto node : gaia_ptr::find_all_range(type)) {
            EXPECT_EQ(node.type(), type);
            count++;
        }
        EXPECT_EQ(count, expected_count);

        std::cerr << std::endl;
        std::cerr << "*** Iterating over nodes with inexact multiple of buffer size:" << std::endl;
        type = 7;
        count = 0;
        expected_count = BUFFER_SIZE_INEXACT_MULTIPLE;
        for (auto node : gaia_ptr::find_all_range(type)) {
            EXPECT_EQ(node.type(), type);
            count++;
        }
        EXPECT_EQ(count, expected_count);

        std::cerr << std::endl;
        std::cerr << "*** Iterating over nodes with one less than buffer size:" << std::endl;
        type = 8;
        count = 0;
        expected_count = BUFFER_SIZE_MINUS_ONE;
        for (auto node : gaia_ptr::find_all_range(type)) {
            EXPECT_EQ(node.type(), type);
            count++;
        }
        EXPECT_EQ(count, expected_count);

        std::cerr << std::endl;
        std::cerr << "*** Iterating over nodes with one more than buffer size:" << std::endl;
        type = 9;
        count = 0;
        expected_count = BUFFER_SIZE_PLUS_ONE;
        for (auto node : gaia_ptr::find_all_range(type)) {
            EXPECT_EQ(node.type(), type);
            count++;
        }
        EXPECT_EQ(count, expected_count);

        std::cerr << std::endl;
    }
    commit_transaction();
}

TEST_F(storage_engine_client_test, iterate_type_delete) {
    begin_transaction();
    {
        std::cerr << std::endl;
        std::cerr << "*** Iterating over nodes of type 1 before delete:" << std::endl;
        auto node_iter = gaia_ptr::find_first(type1);
        print_node(node_iter);
        EXPECT_EQ(node_iter.id(), node1_id);
        std::cerr << std::endl;
        std::cerr << "*** Preparing to delete first node of type 1:" << std::endl;
        gaia_ptr::remove(node_iter);
        std::cerr << "*** Iterating over nodes of type 1 after delete:" << std::endl;
        node_iter = gaia_ptr::find_first(type1);
        print_node(node_iter);
        EXPECT_EQ(node_iter.id(), node2_id);
    }
    commit_transaction();

    begin_transaction();
    {
        std::cerr << std::endl;
        std::cerr << "*** Reloading data: iterating over nodes of type 1 after delete:" << std::endl;
        auto node_iter = gaia_ptr::find_first(type1);
        print_node(node_iter);
        EXPECT_EQ(node_iter.id(), node2_id);
    }
    commit_transaction();
}
