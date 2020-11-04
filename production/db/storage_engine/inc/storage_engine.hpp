/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <string.h>
#include <unistd.h>

#include <cassert>

#include <iomanip>
#include <iostream>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>

#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "db_types.hpp"
#include "gaia_common.hpp"
#include "gaia_db.hpp"
#include "gaia_db_internal.hpp"
#include "gaia_exception.hpp"
#include "gaia_se_object.hpp"
#include "retail_assert.hpp"

namespace gaia
{
namespace db
{

using namespace common;

// 1K oughta be enough for anybody...
constexpr size_t c_max_msg_size = 1 << 10;

enum class gaia_operation_t : uint8_t
{
    create = 0x1,
    update = 0x2,
    remove = 0x3,
    clone = 0x4
};

class se_base
{
    friend class gaia_ptr;
    friend class gaia_hash_map;

protected:
    static constexpr char c_server_connect_socket_name[] = "gaia_se_server";
    static constexpr char c_sch_mem_locators[] = "gaia_mem_locators";
    static constexpr char c_sch_mem_data[] = "gaia_mem_data";
    static constexpr char c_sch_mem_log[] = "gaia_mem_log";

    static constexpr size_t c_max_locators = 32 * 128L * 1024L;
    static constexpr size_t c_hash_buckets = 12289;
    static constexpr size_t c_hash_list_elements = c_max_locators;
    static constexpr size_t c_max_log_records = 1000000;
    static constexpr size_t c_max_objects = c_max_locators * 8;

    typedef gaia_locator_t locators[c_max_locators];

    struct hash_node
    {
        gaia_id_t id;
        size_t next_offset;
        gaia_locator_t locator;
    };

    struct data
    {
        // The first two fields are used as cross-process atomic counters. We
        // don't need something like a cross-process mutex for this, as long as
        // we use atomic intrinsics for mutating the counters. This is because
        // the instructions targeted by the intrinsics operate at the level of
        // physical memory, not virtual addresses.
        gaia_id_t next_object_id;
        gaia_type_t next_type_id;
        gaia_txn_id_t next_txn_id;
        size_t locator_count;
        size_t hash_node_count;
        hash_node hash_nodes[c_hash_buckets + c_hash_list_elements];
        // This array is actually an untyped array of bytes, but it's defined as
        // an array of uint64_t just to enforce 8-byte alignment. Allocating
        // (c_max_locators * 8) 8-byte words for this array means we reserve 64
        // bytes on average for each object we allocate (or 1 cache line on
        // every common architecture).
        uint64_t objects[c_max_locators * 8];
    };

    struct log
    {
        size_t count;
        struct log_record
        {
            gaia_locator_t locator;
            gaia_offset_t old_offset;
            gaia_offset_t new_offset;
            gaia_id_t deleted_id;
            gaia_operation_t operation;
        } log_records[c_max_log_records];
    };

    thread_local static inline log* s_log = nullptr;
    thread_local static inline int s_session_socket = -1;
    thread_local static inline gaia_txn_id_t s_txn_id = c_invalid_gaia_txn_id;

public:

    // Counters for ID and type will be initialized on recovery.
    static gaia_id_t allocate_object_id(data* data)
    {
        gaia_id_t next_object_id = __sync_add_and_fetch(&data->next_object_id, 1);
        return next_object_id;
    }

    static gaia_type_t allocate_type_id(data* data) {
        gaia_type_t next_type_id = __sync_add_and_fetch(&data->next_type_id, 1);
        return next_type_id;
    }

    static gaia_txn_id_t allocate_txn_id(data* s_data)
    {
        gaia_txn_id_t txn_id = __sync_add_and_fetch(&s_data->next_txn_id, 1);
        return txn_id;
    }

    static log* get_txn_log()
    {
        return s_log;
    }

    static inline gaia_locator_t allocate_locator(locators* locators, data* data, bool invoked_by_server = false)
    {
        if (invoked_by_server)
        {
            retail_assert(locators, "Server locators should be non-null");
        }

        if (locators == nullptr)
        {
            throw transaction_not_open();
        }

        if (data->locator_count >= c_max_locators)
        {
            throw oom();
        }

        return 1 + __sync_fetch_and_add(&data->locator_count, 1);
    }

    static inline void allocate_object(gaia_locator_t locator, size_t size, locators* locators, data* data, bool invoked_by_server = false)
    {
        if (invoked_by_server)
        {
            retail_assert(locators, "Server locators should be non-null");
        }

        if (locators == nullptr)
        {
            throw transaction_not_open();
        }

        if (data->objects[0] >= c_max_objects)
        {
            throw oom();
        }

        (*locators)[locator] = 1 + __sync_fetch_and_add(&data->objects[0], (size + sizeof(uint64_t) - 1) / sizeof(uint64_t));
    }

    static inline bool locator_exists(se_base::locators* locators, gaia_locator_t locator)
    {
        return (*locators)[locator];
    }

    static gaia_se_object_t* locator_to_ptr(locators* locators, data* data, gaia_locator_t locator)
    {
        retail_assert(locators);
        return (locator && (*locators)[locator])
            ? reinterpret_cast<gaia_se_object_t*>(data->objects + (*locators)[locator])
            : nullptr;
    }
};

} // namespace db
} // namespace gaia
