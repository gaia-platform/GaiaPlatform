/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "com_gaiaplatform_truegraphdb_CowStorageEngine.h"

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

class payload_t
{
protected:

    JNIEnv* m_env;
    jstring* m_payload;

    jsize m_size;
    const char* m_characters;

    void clear()
    {
        m_env = NULL;
        m_payload = NULL;

        m_size = 0;
        m_characters = NULL;
    }

public:

    payload_t()
    {
        clear();
    }

    payload_t(JNIEnv* env, jstring& payload)
    {
        set(env, payload);
    }

    void set(JNIEnv* env, jstring& payload)
    {
        m_env = env;
        m_payload = &payload;

        m_size = m_env->GetStringUTFLength(*m_payload);
        m_characters = m_env->GetStringUTFChars(*m_payload, NULL);
    }

    ~payload_t()
    {
        if (m_env != NULL && m_characters != NULL)
        {
            m_env->ReleaseStringUTFChars(*m_payload, m_characters);
            clear();
        }
    }

    jsize size()
    {
        return m_size;
    }

    const char* c_str()
    {
        return m_characters;
    }
};

// JNI Implementation starts here.

JNIEXPORT void JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_initialize(JNIEnv*, jobject)
{
    gaia_mem_base::init(true);
}

JNIEXPORT void JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_beginTransaction(JNIEnv*, jobject)
{
    begin_transaction();
}

JNIEXPORT void JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_commitTransaction(JNIEnv*, jobject)
{
    commit_transaction();
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_createNode(
    JNIEnv* env, jobject, jlong id, jlong type, jstring payload)
{
    payload_t payload_holder(env, payload);
    if (payload_holder.c_str() == NULL)
    {
        return NULL;
    }
 
    gaia_ptr<gaia_se_node> node = gaia_se_node::create(
        id, type, payload_holder.size(), payload_holder.c_str());

    return node.get_id();
}

JNIEXPORT void JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_updateNodePayload(
    JNIEnv* env, jobject, jlong id, jstring payload)
{
    payload_t payload_holder(env, payload);
    if (payload_holder.c_str() == NULL)
    {
        return;
    }
 
    gaia_ptr<gaia_se_node> node = gaia_se_node::open(id);
    if (node)
    {
        node.update_payload(payload_holder.size(), payload_holder.c_str());
    }
}

JNIEXPORT void JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_removeNode(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_node> node = gaia_se_node::open(id);
    if (node)
    {
        gaia_ptr<gaia_se_node>::remove(node);
    }
}

JNIEXPORT void JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_printNode(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_node> node = gaia_se_node::open(id);
    print_node(node, false);
    cout << endl;
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_findFirstNode(
    JNIEnv* env, jobject, jlong type)
{
    gaia_ptr<gaia_se_node> node = gaia_ptr<gaia_se_node>::find_first(type);
    if (!node)
    {
        return NULL;
    }

    return node.get_id();
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_findNextNode(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_node> node = gaia_se_node::open(id);
    if (!node)
    {
        return NULL;
    }

    gaia_ptr<gaia_se_node> next_node = node.find_next();
    if (!next_node)
    {
        return NULL;
    }

    return next_node.get_id();
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_getNodeType(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_node> node = gaia_se_node::open(id);
    if (!node)
    {
        return NULL;
    }

    return node->type;
}

JNIEXPORT jstring JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_getNodePayload(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_node> node = gaia_se_node::open(id);
    if (!node)
    {
        return NULL;
    }

    return env->NewStringUTF(node->payload);
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_getNextEdgeWithNodeAsFirst(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_node> node = gaia_se_node::open(id);
    if (!node)
    {
        return NULL;
    }

    gaia_ptr<gaia_se_edge> next_edge_first = node->next_edge_first;
    if (!next_edge_first)
    {
        return NULL;
    }

    return next_edge_first.get_id();
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_getNextEdgeWithNodeAsSecond(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_node> node = gaia_se_node::open(id);
    if (!node)
    {
        return NULL;
    }

    gaia_ptr<gaia_se_edge> next_edge_second = node->next_edge_second;
    if (!next_edge_second)
    {
        return NULL;
    }

    return next_edge_second.get_id();
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_createEdge(
    JNIEnv* env, jobject, jlong id, jlong type, jlong idFirstNode, jlong idSecondNode, jstring payload)
{
    payload_t payload_holder(env, payload);
    if (payload_holder.c_str() == NULL)
    {
        return NULL;
    }
 
    gaia_ptr<gaia_se_edge> edge = gaia_se_edge::create(
        id, type, idFirstNode, idSecondNode, payload_holder.size(), payload_holder.c_str());

    return edge.get_id();
}

JNIEXPORT void JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_updateEdgePayload(
    JNIEnv* env, jobject, jlong id, jstring payload)
{
    payload_t payload_holder(env, payload);
    if (payload_holder.c_str() == NULL)
    {
        return;
    }
 
    gaia_ptr<gaia_se_edge> edge = gaia_se_edge::open(id);
    if (edge)
    {
        edge.update_payload(payload_holder.size(), payload_holder.c_str());
    }
}

JNIEXPORT void JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_removeEdge(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_edge> edge = gaia_se_edge::open(id);
    if (edge)
    {
        gaia_ptr<gaia_se_edge>::remove(edge);
    }
}

JNIEXPORT void JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_printEdge(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_edge> edge = gaia_se_edge::open(id);
    print_edge(edge, false);
    cout << endl;
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_findFirstEdge(
    JNIEnv* env, jobject, jlong type)
{
    gaia_ptr<gaia_se_edge> edge = gaia_ptr<gaia_se_edge>::find_first(type);
    if (!edge)
    {
        return NULL;
    }

    return edge.get_id();
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_findNextEdge(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_edge> edge = gaia_se_edge::open(id);
    if (!edge)
    {
        return NULL;
    }

    gaia_ptr<gaia_se_edge> next_edge = edge.find_next();
    if (!next_edge)
    {
        return NULL;
    }

    return next_edge.get_id();
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_getEdgeType(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_edge> edge = gaia_se_edge::open(id);
    if (!edge)
    {
        return NULL;
    }

    return edge->type;
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_getEdgeFirstNode(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_edge> edge = gaia_se_edge::open(id);
    if (!edge)
    {
        return NULL;
    }

    return edge->first;
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_getEdgeSecondNode(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_edge> edge = gaia_se_edge::open(id);
    if (!edge)
    {
        return NULL;
    }

    return edge->second;
}

JNIEXPORT jstring JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_getEdgePayload(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_edge> edge = gaia_se_edge::open(id);
    if (!edge)
    {
        return NULL;
    }

    return env->NewStringUTF(edge->payload);
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_getNextEdgeWithSameFirstNode(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_edge> edge = gaia_se_edge::open(id);
    if (!edge)
    {
        return NULL;
    }

    gaia_ptr<gaia_se_edge> next_edge_first = edge->next_edge_first;
    if (!next_edge_first)
    {
        return NULL;
    }

    return next_edge_first.get_id();
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_truegraphdb_CowStorageEngine_getNextEdgeWithSameSecondNode(
    JNIEnv* env, jobject, jlong id)
{
    gaia_ptr<gaia_se_edge> edge = gaia_se_edge::open(id);
    if (!edge)
    {
        return NULL;
    }

    gaia_ptr<gaia_se_edge> next_edge_second = edge->next_edge_second;
    if (!next_edge_second)
    {
        return NULL;
    }

    return next_edge_second.get_id();
}
