/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "storage_engine.hpp"

#include "com_gaiaplatform_database_CowStorageEngine.h"

using namespace std;
using namespace gaia::db;

// Exposes to C++ the content of a jstring and automatically releases allocated memory.
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

// Template functions that avoid duplicating code
// that is similar for gaia_node_se and gaia_edge_se types.

template <typename T> jboolean update_payload(JNIEnv* env, jlong id, jstring& payload)
{
    try
    {
        payload_t payload_holder(env, payload);
        if (payload_holder.c_str() == NULL)
        {
            return false;
        }
    
        gaia_ptr<T> t = T::open(id);
        if (t)
        {
            t.update_payload(payload_holder.size(), payload_holder.c_str());
        }
    }
    catch(const std::exception& e)
    {
        cerr << "A COW exception occurred during an update_payload() call: " << e.what() << endl;
        return false;
    }

    return true;
}

template <typename T> jboolean remove(jlong id)
{
    try
    {
        gaia_ptr<T> t = T::open(id);
        if (t)
        {
            gaia_ptr<T>::remove(t);
        }
        else
        {
            return false;
        }
    }
    catch(const std::exception&)
    {
        return false;
    }

    return true;
}

template <typename T> jlong find_first(jlong type)
{
    gaia_ptr<T> t = gaia_ptr<T>::find_first(type);
    if (!t)
    {
        return NULL;
    }

    return t.get_id();
}

template <typename T> jlong find_next(jlong id)
{
    gaia_ptr<T> t = T::open(id);
    if (!t)
    {
        return NULL;
    }

    gaia_ptr<T> next_t = t.find_next();
    if (!next_t)
    {
        return NULL;
    }

    return next_t.get_id();
}

template <typename T> jlong get_type(jlong id)
{
    gaia_ptr<T> t = T::open(id);
    if (!t)
    {
        return NULL;
    }

    return t->type;
}

template <typename T> jstring get_payload(JNIEnv* env, jlong id)
{
    gaia_ptr<T> t = T::open(id);
    if (!t)
    {
        return NULL;
    }

    return env->NewStringUTF(t->payload);
}

template <typename T> jlong get_next_edge_first(jlong id)
{
    gaia_ptr<T> t = T::open(id);
    if (!t)
    {
        return NULL;
    }

    gaia_ptr<gaia_se_edge> next_edge_first = t->next_edge_first;
    if (!next_edge_first)
    {
        return NULL;
    }

    return next_edge_first.get_id();
}

template <typename T> jlong get_next_edge_second(jlong id)
{
    gaia_ptr<T> t = T::open(id);
    if (!t)
    {
        return NULL;
    }

    gaia_ptr<gaia_se_edge> next_edge_second = t->next_edge_second;
    if (!next_edge_second)
    {
        return NULL;
    }

    return next_edge_second.get_id();
}

// JNI implementation starts here.

JNIEXPORT jboolean JNICALL Java_com_gaiaplatform_database_CowStorageEngine_create(
    JNIEnv*, jobject)
{
    try
    {
        // Before calling init(), we must ensure that COW is not already open
        // from a previous call, so we'll call reset() which plays the role of a close() call.
        // However, reset() can print some error messages if COW is not open,
        // so we need to silence those.
        bool silent = true;
        gaia_mem_base::reset(silent);

        // To obtain a clear instance, we need to clear the shared memory.
        bool clearMemory = true;
        gaia_mem_base::init(clearMemory);

        return true;
    }
    catch(const std::exception& e)
    {
        cerr << "A COW exception occurred during a create() call: " << e.what() << endl;
        return false;
    }
}

JNIEXPORT jboolean JNICALL Java_com_gaiaplatform_database_CowStorageEngine_open(
    JNIEnv*, jobject)
{
    try
    {
        // To obtain an existing instance, we need to not clear the shared memory.
        bool clearMemory = false;
        gaia_mem_base::init(clearMemory);

        return true;
    }
    catch(const std::exception& e)
    {
        cerr << "A COW exception occurred during an open() call: " << e.what() << endl;
        return false;
    }
}

JNIEXPORT void JNICALL Java_com_gaiaplatform_database_CowStorageEngine_beginTransaction(JNIEnv*, jobject)
{
    begin_transaction();
}

JNIEXPORT void JNICALL Java_com_gaiaplatform_database_CowStorageEngine_commitTransaction(JNIEnv*, jobject)
{
    commit_transaction();
}

JNIEXPORT void JNICALL Java_com_gaiaplatform_database_CowStorageEngine_rollbackTransaction(JNIEnv*, jobject)
{
    rollback_transaction();
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_CowStorageEngine_createNode(
    JNIEnv* env, jobject, jlong id, jlong type, jstring payload)
{
    payload_t payload_holder(env, payload);
    if (payload_holder.c_str() == NULL)
    {
        return NULL;
    }
 
    gaia_ptr<gaia_se_node> node;

    try
    {
        node = gaia_se_node::create(
            id, type, payload_holder.size(), payload_holder.c_str());
    }
    catch(const std::exception& e)
    {
        cerr << "A COW exception occurred during a createNode() call: " << e.what() << endl;
        return false;
    }

    return node.get_id();
}

JNIEXPORT jboolean JNICALL Java_com_gaiaplatform_database_CowStorageEngine_updateNodePayload(
    JNIEnv* env, jobject, jlong id, jstring payload)
{
    return update_payload<gaia_se_node>(env, id, payload);
}

JNIEXPORT jboolean JNICALL Java_com_gaiaplatform_database_CowStorageEngine_removeNode(
    JNIEnv*, jobject, jlong id)
{
    return remove<gaia_se_node>(id);
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_CowStorageEngine_findFirstNode(
    JNIEnv*, jobject, jlong type)
{
    return find_first<gaia_se_node>(type);
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_CowStorageEngine_findNextNode(
    JNIEnv*, jobject, jlong id)
{
    return find_next<gaia_se_node>(id);
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_CowStorageEngine_getNodeType(
    JNIEnv*, jobject, jlong id)
{
    return get_type<gaia_se_node>(id);
}

JNIEXPORT jstring JNICALL Java_com_gaiaplatform_database_CowStorageEngine_getNodePayload(
    JNIEnv* env, jobject, jlong id)
{
    return get_payload<gaia_se_node>(env, id);
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_CowStorageEngine_getFirstEdgeWithNodeAsFirst(
    JNIEnv*, jobject, jlong id)
{
    return get_next_edge_first<gaia_se_node>(id);
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_CowStorageEngine_getFirstEdgeWithNodeAsSecond(
    JNIEnv*, jobject, jlong id)
{
    return get_next_edge_second<gaia_se_node>(id);
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_CowStorageEngine_createEdge(
    JNIEnv* env, jobject, jlong id, jlong type, jlong idFirstNode, jlong idSecondNode, jstring payload)
{
    payload_t payload_holder(env, payload);
    if (payload_holder.c_str() == NULL)
    {
        return NULL;
    }
 
    gaia_ptr<gaia_se_edge> edge;
    
    try
    {
        edge = gaia_se_edge::create(
            id, type, idFirstNode, idSecondNode, payload_holder.size(), payload_holder.c_str());
    }
    catch(const std::exception& e)
    {
        cerr << "A COW exception occurred during a createEdge() call: " << e.what() << endl;
        return false;
    }

    return edge.get_id();
}

JNIEXPORT jboolean JNICALL Java_com_gaiaplatform_database_CowStorageEngine_updateEdgePayload(
    JNIEnv* env, jobject, jlong id, jstring payload)
{
    return update_payload<gaia_se_edge>(env, id, payload);
}

JNIEXPORT jboolean JNICALL Java_com_gaiaplatform_database_CowStorageEngine_removeEdge(
    JNIEnv*, jobject, jlong id)
{
    return remove<gaia_se_edge>(id);
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_CowStorageEngine_findFirstEdge(
    JNIEnv*, jobject, jlong type)
{
    return find_first<gaia_se_edge>(type);
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_CowStorageEngine_findNextEdge(
    JNIEnv*, jobject, jlong id)
{
    return find_next<gaia_se_edge>(id);
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_CowStorageEngine_getEdgeType(
    JNIEnv*, jobject, jlong id)
{
    return get_type<gaia_se_edge>(id);
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_CowStorageEngine_getEdgeFirstNode(
    JNIEnv*, jobject, jlong id)
{
    gaia_ptr<gaia_se_edge> edge = gaia_se_edge::open(id);
    if (!edge)
    {
        return NULL;
    }

    return edge->first;
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_CowStorageEngine_getEdgeSecondNode(
    JNIEnv*, jobject, jlong id)
{
    gaia_ptr<gaia_se_edge> edge = gaia_se_edge::open(id);
    if (!edge)
    {
        return NULL;
    }

    return edge->second;
}

JNIEXPORT jstring JNICALL Java_com_gaiaplatform_database_CowStorageEngine_getEdgePayload(
    JNIEnv* env, jobject, jlong id)
{
    return get_payload<gaia_se_edge>(env, id);
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_CowStorageEngine_getNextEdgeWithSameFirstNode(
    JNIEnv*, jobject, jlong id)
{
    return get_next_edge_first<gaia_se_edge>(id);
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_CowStorageEngine_getNextEdgeWithSameSecondNode(
    JNIEnv*, jobject, jlong id)
{
    return get_next_edge_second<gaia_se_edge>(id);
}
