/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "gtest/gtest.h"

#include "gaia_db.hpp"
#include "gaia_ptr.hpp"
#include "db_test_helpers.hpp"

using namespace gaia::db;

void print_payload(std::ostream& o, const size_t size, const char* payload)
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
            o << "\\x" <<
                std::setw(2) << std::setfill('0') << std::hex <<
                short(payload[i]) << std::dec;
        }
    }
}

void print_node(const gaia_ptr& node, const bool indent=false)
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

    print_payload (std::cerr, node.data_size(), node.data());

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
class storage_engine_client_test : public ::testing::Test
{
private:
    void init_data() {
        begin_transaction();
        {
            std::cerr << std::endl;
            std::cerr << "*** create test nodes" << std::endl;
            try
            {
                gaia_ptr node1 = gaia_ptr::create(
                    1,1, 0,0);
                gaia_ptr node2 = gaia_ptr::create(
                    2,1, 0,0);
                gaia_ptr node3 = gaia_ptr::create(
                    3,2, 0,0);
                gaia_ptr node4 = gaia_ptr::create(
                    4,2, 0,0);

                print_node(node1);
                print_node(node2);
                print_node(node3);
                print_node(node4);
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what();
            }
        }
        commit_transaction();
    }
protected:
    static void SetUpTestSuite() {
        start_server();
    }

    static void TearDownTestSuite() {
        stop_server();
    }

    void SetUp() override {
        begin_session();
        init_data();
    }

    void TearDown() override {
        end_session();
    }
};

TEST_F(storage_engine_client_test, read_data) {
    begin_transaction();
    {
        std::cerr << std::endl;
        std::cerr << "*** Update payload and verify" << std::endl;
        try
        {
            gaia_ptr node1 = gaia_ptr::open(1);
            gaia_ptr node2 = gaia_ptr::open(2);
            gaia_ptr node3 = gaia_ptr::open(3);
            gaia_ptr node4 = gaia_ptr::open(4);
            print_node(node1);
            print_node(node2);
            print_node(node3);
            print_node(node4);
            EXPECT_EQ(node1.id(), 1);
            EXPECT_EQ(node2.id(), 2);
            EXPECT_EQ(node3.id(), 3);
            EXPECT_EQ(node4.id(), 4);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        std::cerr << std::endl;
    }
    commit_transaction();
}

TEST_F(storage_engine_client_test, update_payload) {
    auto payload = "payload str";
    begin_transaction();
    {
        std::cerr << std::endl;
        std::cerr << "*** Update payload and verify" << std::endl;
        try
        {
            gaia_ptr node1 = gaia_ptr::open(1);
            print_node(node1);
            node1.update_payload(strlen(payload), payload);
            print_node(node1);
            EXPECT_STREQ(node1.data(), payload);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        std::cerr << std::endl;
    }
    commit_transaction();

    begin_transaction();
    {
        std::cerr << std::endl;
        std::cerr << "*** Reload data and verify update" << std::endl;
        try
        {
            gaia_ptr node1 = gaia_ptr::open(1);
            print_node(node1);
            EXPECT_STREQ(node1.data(), payload);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        std::cerr << std::endl;
    }
    commit_transaction();
}

TEST_F(storage_engine_client_test, update_payload_rollback) {
    auto payload = "payload str";
    begin_transaction();
    {
        std::cerr << std::endl;
        std::cerr << "*** Update payload and verify" << std::endl;
        try
        {
            gaia_ptr node1 = gaia_ptr::open(1);
            print_node(node1);
            node1.update_payload(strlen(payload), payload);
            print_node(node1);
            EXPECT_STREQ(node1.data(), payload);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        std::cerr << std::endl;
    }
    rollback_transaction();

    begin_transaction();
    {
        std::cerr << std::endl;
        std::cerr << "*** Reload data and verify update" << std::endl;
        try
        {
            gaia_ptr node1 = gaia_ptr::open(1);
            print_node(node1);
            EXPECT_EQ(node1.data(), nullptr);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        std::cerr << std::endl;
    }
    commit_transaction();
}

TEST_F(storage_engine_client_test, iterate_type) {
    begin_transaction();
    {
        std::cerr << std::endl;
        std::cerr << "*** Iterating over nodes of type 1:" << std::endl;

        try
        {
            gaia_type_t type = 1;
            gaia_id_t id = 1;
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
            type = 2;
            for (auto node_iter = gaia_ptr::find_first(type);
                node_iter;
                node_iter = node_iter.find_next())
            {
                print_node(node_iter);
                EXPECT_EQ(node_iter.id(), id);
                id++;
            }

            std::cerr << std::endl;
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }

        std::cerr << std::endl;
    }
    commit_transaction();
}

TEST_F(storage_engine_client_test, iterate_type_delete) {
    begin_transaction();
    {
        try
        {
            std::cerr << std::endl;
            std::cerr << "*** Iterating over nodes of type 1 before delete:" << std::endl;
            auto node_iter = gaia_ptr::find_first(1);
            print_node(node_iter);
            EXPECT_EQ(node_iter.id(), 1);
            std::cerr << std::endl;
            std::cerr << "*** Preparing to delete first node of type 1:" << std::endl;
            gaia_ptr::remove(node_iter);
            std::cerr << "*** Iterating over nodes of type 1 after delete:" << std::endl;
            node_iter = gaia_ptr::find_first(1);
            print_node(node_iter);
            EXPECT_EQ(node_iter.id(), 2);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }

        std::cerr << std::endl;
    }
    commit_transaction();

    begin_transaction();
    {
        try
        {
            std::cerr << std::endl;
            std::cerr << "*** Reloading data: iterating over nodes of type 1 after delete:" << std::endl;
            auto node_iter = gaia_ptr::find_first(1);
            print_node(node_iter);
            EXPECT_EQ(node_iter.id(), 2);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        std::cerr << std::endl;
    }
    commit_transaction();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
