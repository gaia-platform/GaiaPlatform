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

JNIEXPORT void JNICALL Java_CowStorageEngine_initialize(JNIEnv*, jobject)
{
    gaia_mem_base::init(true);
}

JNIEXPORT void JNICALL Java_CowStorageEngine_beginTransaction(JNIEnv*, jobject)
{
    begin_transaction();
}

JNIEXPORT void JNICALL Java_CowStorageEngine_commitTransaction(JNIEnv*, jobject)
{
    commit_transaction();
}

JNIEXPORT jlong JNICALL Java_CowStorageEngine_createNode(
    JNIEnv* env, jobject, jlong id, jlong type, jstring payload)
{
    jsize payload_size = env->GetStringUTFLength(payload);
    const char* payload_characters = env->GetStringUTFChars(payload, NULL);
    if (payload_characters == NULL)
    {
        return NULL;
    }
 
    gaia_ptr<gaia_se_node> node = gaia_se_node::create(
        id, type, payload_size, payload_characters);

    // We need to call this after we're done using nameCharacters.
    env->ReleaseStringUTFChars(payload, payload_characters);

    return node.get_row_id();
}

JNIEXPORT void JNICALL Java_CowStorageEngine_updateNode(
    JNIEnv* env, jobject, jlong id, jstring payload)
{
    jsize payload_size = env->GetStringUTFLength(payload);
    const char* payload_characters = env->GetStringUTFChars(payload, NULL);
    if (payload_characters == NULL)
    {
        return;
    }
 
    gaia_ptr<gaia_se_node> node(id);
    if (node)
    {
        node.update_payload(payload_size, payload_characters);
    }

    // We need to call this after we're done using nameCharacters.
    env->ReleaseStringUTFChars(payload, payload_characters);
}

JNIEXPORT void JNICALL Java_CowStorageEngine_removeNode(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_node> node(id);
    if (node)
    {
        gaia_ptr<gaia_se_node>::remove(node);
    }
}

JNIEXPORT void JNICALL Java_CowStorageEngine_printNode(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_node> node(id);
    print_node(node, false);
    cout << endl;
}

JNIEXPORT jlong JNICALL Java_CowStorageEngine_findFirstNode(
    JNIEnv* env, jobject, jlong type)
{
    gaia_ptr<gaia_se_node> node = gaia_ptr<gaia_se_node>::find_first(type);
    if (!node)
    {
        return NULL;
    }

    return node.get_row_id();
}

JNIEXPORT jlong JNICALL Java_CowStorageEngine_findNextNode(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_node> node(id);
    if (!node)
    {
        return NULL;
    }

    gaia_ptr<gaia_se_node> next_node = node.find_next();
    if (!next_node)
    {
        return NULL;
    }

    return next_node.get_row_id();
}

JNIEXPORT jlong JNICALL Java_CowStorageEngine_createEdge(
    JNIEnv* env, jobject, jlong id, jlong type, jlong idFirstNode, jlong idSecondNode, jstring payload)
{
    jsize payload_size = env->GetStringUTFLength(payload);
    const char* payload_characters = env->GetStringUTFChars(payload, NULL);
    if (payload_characters == NULL)
    {
        return NULL;
    }
 
    gaia_ptr<gaia_se_edge> edge = gaia_se_edge::create(
        id, type, idFirstNode, idSecondNode, payload_size, payload_characters);

    // We need to call this after we're done using nameCharacters.
    env->ReleaseStringUTFChars(payload, payload_characters);

    return edge.get_row_id();
}

JNIEXPORT void JNICALL Java_CowStorageEngine_updateEdge(
    JNIEnv* env, jobject, jlong id, jstring payload)
{
    jsize payload_size = env->GetStringUTFLength(payload);
    const char* payload_characters = env->GetStringUTFChars(payload, NULL);
    if (payload_characters == NULL)
    {
        return;
    }
 
    gaia_ptr<gaia_se_edge> edge(id);
    if (edge)
    {
        edge.update_payload(payload_size, payload_characters);
    }

    // We need to call this after we're done using nameCharacters.
    env->ReleaseStringUTFChars(payload, payload_characters);
}

JNIEXPORT void JNICALL Java_CowStorageEngine_removeEdge(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_edge> edge(id);
    if (edge)
    {
        gaia_ptr<gaia_se_edge>::remove(edge);
    }
}

JNIEXPORT void JNICALL Java_CowStorageEngine_printEdge(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_edge> edge(id);
    print_edge(edge, false);
    cout << endl;
}

JNIEXPORT jlong JNICALL Java_CowStorageEngine_findFirstEdge(
    JNIEnv* env, jobject, jlong type)
{
    gaia_ptr<gaia_se_edge> edge = gaia_ptr<gaia_se_edge>::find_first(type);
    if (!edge)
    {
        return NULL;
    }

    return edge.get_row_id();
}

JNIEXPORT jlong JNICALL Java_CowStorageEngine_findNextEdge(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_edge> edge(id);
    if (!edge)
    {
        return NULL;
    }

    gaia_ptr<gaia_se_edge> next_edge = edge.find_next();
    if (!next_edge)
    {
        return NULL;
    }

    return next_edge.get_row_id();
}
