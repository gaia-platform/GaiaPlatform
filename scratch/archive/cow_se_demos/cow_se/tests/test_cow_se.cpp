/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>
#include <iomanip>

#include "cow_se.h"

using namespace gaia_se;

void print_payload(std::ostream& o, const size_t size, const char* payload)
{
    if (size)
    {
        o << " Payload: ";
    }

    for (auto i = 0; i < size; i++)
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

void print_node(const gaia_ptr<gaia_se_node>& node, const bool indent = false);

void print_edge(const gaia_ptr<gaia_se_edge>& edge, const bool indent = false)
{
    if (!edge)
    {
        return;
    }

    std::cout << std::endl;
    
    if (indent)
    {
        std::cout << "  ";
    }

    std::cout
        << "Edge id:" 
        << edge->id << ", type:"
        << edge->type;

    print_payload (std::cout, edge->payload_size, edge->payload);

    if (!indent) 
    {
        print_node (edge->node_first, true);
        print_node (edge->node_second, true);
        std::cout << std::endl;
    }
    else
    {
    std::cout
        << " first: " << edge->first
        << " second: " << edge->second;
    }
}

void print_node(const gaia_ptr<gaia_se_node>& node, const bool indent)
{
    if (!node)
    {
        return;
    }

    std::cout << std::endl;

    if (indent)
    {
        std::cout << "  ";
    }

    std::cout
        << "Node id:" 
        << node->id << ", type:"
        << node->type;

    print_payload (std::cout, node->payload_size, node->payload);

    if (indent || (!node->next_edge_first && ! node->next_edge_second))
    {
        return;
    }

    for (auto edge = node->next_edge_first; edge; edge = edge->next_edge_first)
    {
        print_edge(edge, true);
    }

    for (auto edge = node->next_edge_second; edge; edge = edge->next_edge_second)
    {
        print_edge(edge, true);
    }

    std::cout << std::endl;
}

int main(int, char*[])
{
    auto const TEST_OBJECTS = 100000;

    gaia_mem_base::init(true);

    gaia_se::begin_transaction();
    {
        std::cout << std::endl;
        std::cout << "*** create and verify test nodes and rollback" << std::endl;
        try
        {
            gaia_ptr<gaia_se_node> node1 = gaia_se_node::create(
                1,1, 0,0);
            gaia_ptr<gaia_se_node> node2 = gaia_se_node::create(
                2,1, 0,0);
            gaia_ptr<gaia_se_node> node3 = gaia_se_node::create(
                3,2, 0,0);
            gaia_ptr<gaia_se_node> node4 = gaia_se_node::create(
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
        
        std::cout << std::endl;
    }
    gaia_se::rollback_transaction();

    gaia_se::begin_transaction();
    {
        std::cout << std::endl;
        std::cout << "*** create and verify test nodes and commit" << std::endl;
        
        try
        {
            auto payload1 = "payload 1";

            gaia_ptr<gaia_se_node> node1 = gaia_se_node::create(
                1,1, strlen(payload1),payload1);
            gaia_ptr<gaia_se_node> node2 = gaia_se_node::create(
                2,1, 0,0);
            gaia_ptr<gaia_se_node> node3 = gaia_se_node::create(
                3,2, 0,0);
            gaia_ptr<gaia_se_node> node4 = gaia_se_node::create(
                4,2, 0,0);

            print_node(node1);
            print_node(node2);
            print_node(node3);
            print_node(node4);

            std::cout << std::endl;

            auto payload2 = "payload \x12 string \t2";
            node1.update_payload(strlen(payload2), payload2);
            print_node(node1);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what();
        }

        std::cout << std::endl;
    }
    gaia_se::commit_transaction();

    gaia_se::begin_transaction();
    {
        std::cout << std::endl;
        std::cout << "*** Update payload and verify";
        try
        {
            gaia_ptr<gaia_se_node> node1 = gaia_se_node::open(1);

            print_node(node1);

            auto payload = "payload str";
            node1.update_payload(strlen(payload), payload);
            print_node(node1);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }

        std::cout << std::endl;
    }
    gaia_se::commit_transaction();

    gaia_se::begin_transaction();
    {
        std::cout << std::endl;
        std::cout << "*** create and verify " << TEST_OBJECTS << " nodes and rollback" << std::endl;;
        try
        {
            for (auto i = 100; i < TEST_OBJECTS; i++)
            {
                gaia_se_node::create(
                    i, 100, sizeof(i), &i);
                gaia_se_node::create(
                    i + TEST_OBJECTS, 101, sizeof(i), &i);
            }

            for (auto i = 100; i < TEST_OBJECTS; i++)
            {
                gaia_ptr<gaia_se_node> node_i = gaia_se_node::open(i);
                assert (node_i->type == 100);
                assert (node_i->payload_size == sizeof (i));
                assert (*(int*)node_i->payload == i);

                assert(!node_i->next_edge_first);
                assert(!node_i->next_edge_second);
            }

            auto count = 0;
            for (auto node = gaia_ptr<gaia_se_node>::find_first(101); node; ++node)
            {
                assert(node->type = 100);

                assert (node->type == 100);
                assert (node->payload_size == sizeof (count));
                assert (*(int*)node->payload == 100 + count);

                assert(!node->next_edge_first);
                assert(!node->next_edge_second);

                ++count;
            }
            assert (count == TEST_OBJECTS - 100);

        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }

        std::cout << std::endl;
    }
    gaia_se::rollback_transaction();

    gaia_se::begin_transaction();
    {
        std::cout << std::endl;
        std::cout << "*** create and verify some edges and commit" << std::endl;;

        try
        {
            gaia_se_edge::create(5,3, 1,2, 0,0);
            gaia_se_edge::create(6,3, 1,3, 0,0);
            gaia_se_edge::create(7,4, 4,1, 0,0);
            gaia_se_edge::create(8,4, 2,3, 0,0);
            gaia_se_edge::create(9,4, 2,1, 0,0);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }
    gaia_se::commit_transaction();

    gaia_se::begin_transaction();
    {
        std::cout << std::endl;
        std::cout << "*** open and print nodes with edges" << std::endl;;

        try
        {
            gaia_ptr<gaia_se_node> node1 = gaia_se_node::open(1);
            gaia_ptr<gaia_se_node> node2 = gaia_se_node::open(2);
            gaia_ptr<gaia_se_node> node3 = gaia_se_node::open(3);
            gaia_ptr<gaia_se_node> node4 = gaia_se_node::open(4);

            print_node(node1);
            print_node(node2);
            print_node(node3);
            print_node(node4);

            std::cout << std::endl;

            std::cout << std::endl;
            std::cout << "*** open and print edges with nodes" << std::endl;;
            
            gaia_ptr<gaia_se_edge> edge1 = gaia_se_edge::open(5);
            gaia_ptr<gaia_se_edge> edge2 = gaia_se_edge::open(6);
            gaia_ptr<gaia_se_edge> edge3 = gaia_se_edge::open(7);
            gaia_ptr<gaia_se_edge> edge4 = gaia_se_edge::open(8);

            print_edge(edge1);
            print_edge(edge2);
            print_edge(edge3);
            print_edge(edge4);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }

        std::cout << std::endl;
    }
    gaia_se::commit_transaction();

    gaia_se::begin_transaction();
    {
        std::cout << std::endl;
        std::cout << "*** create and verify " << TEST_OBJECTS << " nodes and rollback" << std::endl;;
        
        try
        {
            for (auto i = 100; i < TEST_OBJECTS; i++)
            {
                gaia_se_node::create(i,100, sizeof(i), &i);
            }
        
            for (auto i = 100; i < (TEST_OBJECTS + 100) / 2; i++)
            {
                gaia_se_edge::create(
                    TEST_OBJECTS + i + 100, 200, i, TEST_OBJECTS + 100 - i - 1, sizeof(i), &i);
            }

            for (auto i = 100; i < (TEST_OBJECTS + 100) / 2; i++)
            {
                gaia_ptr<gaia_se_edge> node_i = gaia_se_edge::open(TEST_OBJECTS + i + 100);
                assert (node_i->type == 200);
                assert (node_i->payload_size == sizeof (i));
                assert (*(int*)node_i->payload == i);

                assert(!node_i->next_edge_first);
                assert(!node_i->next_edge_second);

                assert(node_i->node_first->id == i);
                assert(node_i->node_second->id == TEST_OBJECTS + 100 - i - 1);
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }

        std::cout << std::endl;
    }
    gaia_se::rollback_transaction();

    gaia_se::begin_transaction();
    {
        std::cout << std::endl;
        std::cout << "*** Iterating over nodes of type 1:" << std::endl;

        try
        {
            gaia_type_t type = 1;
            
            for (auto node_iter = gaia_ptr<gaia_se_node>::find_first(type);
                node_iter;
                node_iter = node_iter.find_next())
            {
                print_node(node_iter);
            }

            std::cout << std::endl;

            std::cout << "*** Iterating over edges of type 4:" << std::endl;
            type = 4;

            for (auto edge_iter = gaia_ptr<gaia_se_edge>::find_first(type);
                edge_iter;
                edge_iter = edge_iter.find_next())
            {
                print_edge(edge_iter);
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }

        std::cout << std::endl;
    }
    gaia_se::commit_transaction();

    gaia_se::begin_transaction();
    {
        std::cout << "*** Iterating over nodes of type 1 before delete:" << std::endl;

        try
        {
            gaia_type_t type = 1;
            
            for (auto node_iter = gaia_ptr<gaia_se_node>::find_first(type);
                node_iter;
                node_iter = node_iter.find_next())
            {
                print_node(node_iter);
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }

        std::cout << std::endl;
    }
    gaia_se::commit_transaction();

    gaia_se::begin_transaction();
    {
        std::cout << std::endl << "*** Removing a node and related edges" << std::endl;

        try
        {
            gaia_ptr<gaia_se_node> node1 = gaia_se_node::open(1);

            print_node(node1);

            std::cout << std::endl;

            gaia_ptr<gaia_se_edge> edge1 = gaia_se_edge::open(5);
            gaia_ptr<gaia_se_edge> edge2 = gaia_se_edge::open(6);
            gaia_ptr<gaia_se_edge> edge3 = gaia_se_edge::open(7);
            gaia_ptr<gaia_se_edge> edge4 = gaia_se_edge::open(9);

            gaia_ptr<gaia_se_edge>::remove(edge3);
            gaia_ptr<gaia_se_edge>::remove(edge1);
            gaia_ptr<gaia_se_edge>::remove(edge2);
            gaia_ptr<gaia_se_edge>::remove(edge4);

            gaia_ptr<gaia_se_node>::remove(node1);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }

        std::cout << std::endl;
    }
    gaia_se::commit_transaction();

    gaia_se::begin_transaction();
    {
        std::cout << "*** Iterating over nodes of type 1 after delete:" << std::endl;

        try
        {
            assert(!gaia_se_edge::open(5));
            assert(!gaia_se_edge::open(6));
            assert(!gaia_se_edge::open(7));
            assert(!gaia_se_node::open(1));

            gaia_type_t type = 1;
            
            for (auto node_iter = gaia_ptr<gaia_se_node>::find_first(type);
                node_iter;
                node_iter = node_iter.find_next())
            {
                print_node(node_iter);
            }
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }

        std::cout << std::endl;
    }
    gaia_se::commit_transaction();

    std::cout << "All tests passed!" << std::endl;

    return 1;
}
