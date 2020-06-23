/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/file.h>

#include <iostream>
#include <iomanip>
#include <cassert>
#include <set>
#include <thread>
#include <mutex>
#include <stdexcept>
#include <string>
#include <sstream>

#include "scope_guard.hpp"
#include "system_error.hpp"
#include "gaia_common.hpp"
#include "gaia_exception.hpp"

namespace gaia
{

namespace db
{

using namespace common;

typedef uint64_t gaia_edge_type_t;
typedef void (* gaia_tx_hook)(void);

// highest order bit to indicate the object is an edge
const gaia_id_t c_edge_flag = 0x8000000000000000;
// 1K oughta be enough for anybody...
const size_t MAX_MSG_SIZE = 1 << 10;

class session_exists: public gaia_exception
{
public:
    session_exists()
    {
        m_message = "Close the current session before creating a new one.";
    }
};

class no_session_active: public gaia_exception
{
public:
    no_session_active()
    {
        m_message = "Create a new session before opening a transaction.";
    }
};

class tx_in_progress: public gaia_exception
{
public:
    tx_in_progress()
    {
        m_message = "Commit or roll back the current transaction before beginning a new transaction.";
    }
};

class tx_not_open: public gaia_exception
{
public:
    tx_not_open()
    {
        m_message = "Begin a transaction before performing data access.";
    }
};

class tx_update_conflict: public gaia_exception
{
public:
    tx_update_conflict()
    {
        m_message = "Transaction was aborted due to a serialization error.";
    }
};

class duplicate_id: public gaia_exception
{
public:
    duplicate_id(gaia_id_t id)
    {
        std::stringstream strs;
        strs << "An object with the same ID (" << id << ") already exists.";
        m_message = strs.str();    }
};

class oom: public gaia_exception
{
public:
    oom()
    {
        m_message = "Out of memory.";
    }
};

class dependent_edges_exist: public gaia_exception
{
public:
    dependent_edges_exist(gaia_id_t id)
    {
        std::stringstream strs;
        strs << "Cannot remove node with ID " << id << " because it has dependent edges.";
        m_message = strs.str();
    }
};

class invalid_node_id: public gaia_exception
{
public:
    invalid_node_id(int64_t id)
    {
        std::stringstream strs;
        strs << "Cannot find a node with ID " << id << ".";
            m_message = strs.str();
    }


    class invalid_id_value: public gaia_exception
    {
    public:
        invalid_id_value(gaia_id_t id)
        {
            std::stringstream strs;
            strs << "ID value " << id << " is larger than the maximum ID value 2^63.";
            m_message = strs.str();
        }
    };

class node_not_disconnected: public gaia_exception
{
    public:
        node_not_disconnected(gaia_id_t id, gaia_type_t object_type) {
            stringstream msg;
            msg << "Cannot delete object " << id << ", type " << object_type << " because it is still connected to other object.";
            m_message = msg.str();
        }
    };

inline void check_id(gaia_id_t id)
    {
        if (id & c_edge_flag)
        {
            throw invalid_id_value(id);
        }
    }
}

class se_base
{
    template<typename T>
    friend class gaia_ptr;
    friend class gaia_hash_map;
protected:
    static const char* const SERVER_CONNECT_SOCKET_NAME;
    static const char* const SCH_MEM_OFFSETS;
    static const char* const SCH_MEM_DATA;
    static const char* const SCH_MEM_LOG;

    auto static const MAX_RIDS = 32 * 128L * 1024L;
    static const auto HASH_BUCKETS = 12289;
    static const auto HASH_LIST_ELEMENTS = MAX_RIDS;
    static const auto MAX_LOG_RECS = 1000000;
    static const auto MAX_OBJECTS = MAX_RIDS * 8;

    typedef int64_t offsets[MAX_RIDS];

    struct hash_node
    {
        gaia_id_t id;
        int64_t next;
        int64_t row_id;
    };

    struct data
    {
        // The first two fields are used as cross-process atomic counters.
        // We don't need something like a cross-process mutex for this,
        // as long as we use atomic intrinsics for mutating the counters.
        // This is because the instructions targeted by the intrinsics
        // operate at the level of physical memory, not virtual addresses.
        gaia_id_t next_id;
        int64_t row_id_count;
        int64_t hash_node_count;
        hash_node hash_nodes[HASH_BUCKETS + HASH_LIST_ELEMENTS];
        int64_t objects[MAX_RIDS * 8];
    };

    struct log
    {
        int64_t count;
        struct log_record {
            int64_t row_id;
            int64_t old_object;
            int64_t new_object;
        } log_records[MAX_LOG_RECS];
    };

    static int s_fd_offsets;
    static data *s_data;
    thread_local static log *s_log;
    thread_local static int s_session_socket;

public:
    // The real implementation will need
    // to do something better than increment
    // a counter.  It will need to guarantee
    // that the generated id is not in use
    // already by a database that is
    // restored.
    static gaia_id_t generate_id()
    {
        gaia_id_t id = __sync_fetch_and_add(&s_data->next_id, 1);
        return id;
    }
};

} // db
} // gaia
