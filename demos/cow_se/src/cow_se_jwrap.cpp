/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "CowStorageEngine.h"

#include <iostream>

#include "cow_se.h"

using namespace std;
using namespace gaia_se;

void print_payload(ostream& o, const size_t size, const char* payload)
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
                setw(2) << setfill('0') << hex <<
                short(payload[i]) << dec;
        }
    }
}

void print_node(const gaia_ptr<gaia_se_node>& node, const bool indent = false);

void print_edge(const gaia_ptr<gaia_se_edge>& edge, const bool indent = false)
{
    cout << endl;
    
    if (indent)
    {
        cout << "  ";
    }

    if (!edge)
    {
        cout << "<null_edge>";
        return;
    }

    cout
        << "Edge id:" 
        << edge->id << ", type:"
        << edge->type;

    print_payload (cout, edge->payload_size, edge->payload);

    if (!indent) 
    {
        print_node (edge->node_first, true);
        print_node (edge->node_second, true);
        cout << endl;
    }
    else
    {
    cout
        << " first: " << edge->first
        << " second: " << edge->second;
    }
}

void print_node(const gaia_ptr<gaia_se_node>& node, const bool indent)
{
    cout << endl;

    if (indent)
    {
        cout << "  ";
    }

    if (!node)
    {
        cout << "<null_node>";
        return;
    }

    cout
        << "Node id:" 
        << node->id << ", type:"
        << node->type;

    print_payload (cout, node->payload_size, node->payload);

    if (indent || (!node->next_edge_first && ! node->next_edge_second))
    {
        return;
    }

    for (auto edge = node->next_edge_first; edge; edge = edge->next_edge_first)
    {
        print_edge(edge, true);
    }

    for (auto edge = node->next_edge_second; edge; edge = edge->next_edge_first)
    {
        print_edge(edge, true);
    }

    cout << endl;
}

// JNI Implementation starts here.

JNIEXPORT void JNICALL Java_CowStorageEngine_helloCow(JNIEnv*, jobject)
{
    std::cout << "CowStorageEngine says: Moo!" << std::endl;
}
