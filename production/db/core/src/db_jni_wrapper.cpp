/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <iostream>

#include "gaia/db/db.hpp"
#include "gaia/system.hpp"
#include "gaia_internal/db/gaia_ptr.hpp"

#include "com_gaiaplatform_database_GaiaDatabase.h"

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
        // NOLINTNEXTLINE(modernize-use-nullptr)
        m_env = NULL;
        // NOLINTNEXTLINE(modernize-use-nullptr)
        m_payload = NULL;

        m_size = 0;
        // NOLINTNEXTLINE(modernize-use-nullptr)
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
        // NOLINTNEXTLINE(modernize-use-nullptr)
        m_bytes = m_env->GetByteArrayElements(*m_payload, NULL);
    }

    ~payload_t()
    {
        // NOLINTNEXTLINE(modernize-use-nullptr)
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
        // NOLINTNEXTLINE(modernize-use-nullptr)
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
        cerr << "A Gaia database exception occurred during an update_payload() call: '" << e.what() << "'." << endl;
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
        // NOLINTNEXTLINE(modernize-use-nullptr)
        return NULL;
    }

    // Copy results into an array that we can return.
    jbyteArray output_array = env->NewByteArray(t.data_size());
    env->SetByteArrayRegion(output_array, 0, t.data_size(), reinterpret_cast<jbyte*>(t.data()));

    return output_array;
}

// JNI implementation starts here.

// NOLINTNEXTLINE(readability-identifier-naming)
JNIEXPORT void JNICALL Java_com_gaiaplatform_database_GaiaDatabase_beginSession(JNIEnv*, jobject)
{
    gaia::system::initialize();
}

// NOLINTNEXTLINE(readability-identifier-naming)
JNIEXPORT void JNICALL Java_com_gaiaplatform_database_GaiaDatabase_endSession(JNIEnv*, jobject)
{
    end_session();
}

// NOLINTNEXTLINE(readability-identifier-naming)
JNIEXPORT void JNICALL Java_com_gaiaplatform_database_GaiaDatabase_beginTransaction(JNIEnv*, jobject)
{
    begin_transaction();
}

// NOLINTNEXTLINE(readability-identifier-naming)
JNIEXPORT void JNICALL Java_com_gaiaplatform_database_GaiaDatabase_commitTransaction(JNIEnv*, jobject)
{
    commit_transaction();
}

// NOLINTNEXTLINE(readability-identifier-naming)
JNIEXPORT void JNICALL Java_com_gaiaplatform_database_GaiaDatabase_rollbackTransaction(JNIEnv*, jobject)
{
    rollback_transaction();
}

// NOLINTNEXTLINE(readability-identifier-naming)
JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_GaiaDatabase_createNode(
    JNIEnv* env, jobject, jlong id, jlong type, jbyteArray payload)
{
    payload_t payload_holder(env, payload);
    // NOLINTNEXTLINE(modernize-use-nullptr)
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
        cerr << "A Gaia database exception occurred during a createNode() call: '" << e.what() << "'." << endl;
        return NULL;
    }

    return node.id();
}

// NOLINTNEXTLINE(readability-identifier-naming)
JNIEXPORT jboolean JNICALL Java_com_gaiaplatform_database_GaiaDatabase_updateNodePayload(
    JNIEnv* env, jobject, jlong id, jbyteArray payload)
{
    return update_payload(env, id, payload);
}

// NOLINTNEXTLINE(readability-identifier-naming)
JNIEXPORT jboolean JNICALL Java_com_gaiaplatform_database_GaiaDatabase_removeNode(
    JNIEnv*, jobject, jlong id)
{
    return remove(id);
}

// NOLINTNEXTLINE(readability-identifier-naming)
JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_GaiaDatabase_findFirstNode(
    JNIEnv*, jobject, jlong type)
{
    return find_first(type);
}

// NOLINTNEXTLINE(readability-identifier-naming)
JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_GaiaDatabase_findNextNode(
    JNIEnv*, jobject, jlong id)
{
    return find_next(id);
}

// NOLINTNEXTLINE(readability-identifier-naming)
JNIEXPORT jlong JNICALL Java_com_gaiaplatform_database_GaiaDatabase_getNodeType(
    JNIEnv*, jobject, jlong id)
{
    return get_type(id);
}

// NOLINTNEXTLINE(readability-identifier-naming)
JNIEXPORT jbyteArray JNICALL Java_com_gaiaplatform_database_GaiaDatabase_getNodePayload(
    JNIEnv* env, jobject, jlong id)
{
    return get_payload(env, id);
}
