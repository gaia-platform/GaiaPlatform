/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gaia/db/db.hpp"
#include "com_gaiaplatform_database_CowStorageEngine.h"
#include "gaia_ptr.hpp"

using namespace std;
using namespace gaia::db;

// Exposes to C++ the content of a jbyteArray and automatically releases allocated memory.
class payload_t
{
protected:
    JNIEnv* m_env;
    jbyteArray* m_payload;

    jsize m_size;
    jbyte* m_bytes;

    void clear()
    {
        m_env = NULL;
        m_payload = NULL;

        m_size = 0;
        m_bytes = NULL;
    }

public:
    payload_t()
    {
        clear();
    }

    payload_t(JNIEnv* env, jbyteArray& payload)
    {
        set(env, payload);
    }

    void set(JNIEnv* env, jbyteArray& payload)
    {
        m_env = env;
        m_payload = &payload;

        m_size = m_env->GetArrayLength(*m_payload);
        m_bytes = m_env->GetByteArrayElements(*m_payload, NULL);
    }

    ~payload_t()
    {
        if (m_env != NULL && m_bytes != NULL)
        {
            m_env->ReleaseByteArrayElements(*m_payload, m_bytes, 0);
            clear();
        }
    }

    jsize size()
    {
        return m_size;
    }

    const jbyte* bytes()
    {
        return m_bytes;
    }
};

// Template functions that avoid duplicating code
// that is similar for gaia_node_se and gaia_edge_se types.

jboolean update_payload(
    JNIEnv* env, jlong id, jbyteArray& payload)
{
    try
    {
        payload_t payload_holder(env, payload);
        if (payload_holder.bytes() == NULL)
        {
            return false;
        }

        gaia_ptr t = gaia_ptr::open(id);
        if (t)
        {
            t.update_payload(payload_holder.size(), payload_holder.bytes());
        }
    }
    catch (const std::exception& e)
    {
        cerr << "A COW exception occurred during an update_payload() call: " << e.what() << endl;
        return false;
    }

    return true;
}

jboolean remove(jlong id)
{
    try
    {
        gaia_ptr t = gaia_ptr::open(id);
        if (t)
        {
            gaia_ptr::remove(t);
        }
        else
        {
            return false;
        }
    }
    catch (const std::exception&)
    {
        return false;
    }

    return true;
}

jlong find_first(jlong type)
{
    gaia_ptr t = gaia_ptr::find_first(type);
    if (!t)
    {
        return NULL;
    }

    return t.id();
}

jlong find_next(jlong id)
{
    gaia_ptr t = gaia_ptr::open(id);
    if (!t)
    {
        return NULL;
    }

    gaia_ptr next_t = t.find_next();
    if (!next_t)
    {
        return NULL;
    }

    return next_t.id();
}

jlong get_type(jlong id)
{
    gaia_ptr t = gaia_ptr::open(id);
    if (!t)
    {
        return NULL;
    }

    return t.type();
}

jbyteArray get_payload(JNIEnv* env, jlong id)
{
    gaia_ptr t = gaia_ptr::open(id);
    if (!t || t.data_size() == 0)
    {
        return NULL;
    }

    // Copy results into an array that we can return.
    jbyteArray outputArray = env->NewByteArray(t.data_size());
    env->SetByteArrayRegion(outputArray, 0, t.data_size(), (jbyte*)(t.data()));

    return outputArray;
}

// JNI implementation starts here.

JNIEXPORT void JNICALL Java_com_gaiaplatform_database_CowStorageEngine_beginSession(JNIEnv*, jobject)
{
    begin_session();
}

JNIEXPORT void JNICALL Java_com_gaiaplatform_database_CowStorageEngine_endSession(JNIEnv*, jobject)
{
    end_session();
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
    JNIEnv* env, jobject, jlong id, jlong type, jbyteArray payload)
{
    payload_t payload_holder(env, payload);
    if (payload_holder.bytes() == NULL)
    {
        return NULL;
    }

    gaia_ptr node;

    try
    {
        node = gaia_ptr::create(
            id, type, payload_holder.size(), payload_holder.bytes());
    }
    catch (const std::exception& e)
    {
        cerr << "A COW exception occurred during a createNode() call: " << e.what() << endl;
        return NULL;
    }

    return node.id();
}

JNIEXPORT jboolean JNICALL Java_com_gaiaplatform_database_CowStorageEngine_updateNodePayload(
    JNIEnv* env, jobject, jlong id, jbyteArray payload)
{
    return update_payload(env, id, payload);
}

JNIEXPORT jboolean JNICALL Java_com_gaiaplatform_database_CowStorageEngine_removeNode(
    JNIEnv*, jobject, jlong id)
{
    return remove(id);
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_CowStorageEngine_findFirstNode(
    JNIEnv*, jobject, jlong type)
{
    return find_first(type);
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_CowStorageEngine_findNextNode(
    JNIEnv*, jobject, jlong id)
{
    return find_next(id);
}

JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_CowStorageEngine_getNodeType(
    JNIEnv*, jobject, jlong id)
{
    return get_type(id);
}

JNIEXPORT jbyteArray JNICALL Java_com_gaiaplatform_database_CowStorageEngine_getNodePayload(
    JNIEnv* env, jobject, jlong id)
{
    return get_payload(env, id);
}
