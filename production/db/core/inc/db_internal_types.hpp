/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>

#include <atomic>
#include <memory>
#include <ostream>
#include <unordered_map>

#include "gaia/common.hpp"

#include "gaia_internal/common/mmap_helpers.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/db_object.hpp"
#include "gaia_internal/db/db_types.hpp"

#include "memory_types.hpp"

namespace gaia
{
namespace db
{

template <typename T>
using aligned_storage_for_t =
    typename std::aligned_storage<sizeof(T), alignof(T)>::type;

enum class gaia_operation_t : uint8_t
{
    not_set = 0x0,
    create = 0x1,
    update = 0x2,
    remove = 0x3,
};

inline std::ostream& operator<<(std::ostream& os, const gaia_operation_t& o)
{
    switch (o)
    {
    case gaia_operation_t::not_set:
        os << "not_set";
        break;
    case gaia_operation_t::create:
        os << "create";
        break;
    case gaia_operation_t::update:
        os << "update";
        break;
    case gaia_operation_t::remove:
        os << "remove";
        break;
    default:
        ASSERT_UNREACHABLE("Unknown value of gaia_operation_t!");
    }
    return os;
}

constexpr char c_gaia_mem_locators_prefix[] = "gaia_mem_locators_";
constexpr char c_gaia_mem_counters_prefix[] = "gaia_mem_counters_";
constexpr char c_gaia_mem_data_prefix[] = "gaia_mem_data_";
constexpr char c_gaia_mem_id_index_prefix[] = "gaia_mem_id_index_";

constexpr char c_gaia_mem_txn_log_prefix[] = "gaia_mem_txn_log_";
constexpr char c_gaia_internal_txn_log_prefix[] = "gaia_internal_txn_log_";

#if __has_feature(thread_sanitizer)
// We set the maximum number of locators to 2^29 in TSan builds, which reduces
// the data segment size from 256GB to 32GB. This seems small enough to avoid
// ENOMEM errors when mapping the data segment under TSan. Because our chunk
// address space is unchanged (still 2^16 4MB chunks), we could now segfault if
// we allocate too many chunks! However, given that we still have room for 1
// minimum-sized (64B) object version per locator, this is unlikely, so it's
// probably acceptable for TSan builds (since they're not intended to be used
// in production). If we do encounter this issue, then we can add explicit
// checks to chunk allocation: we just need to define a new constant like
// constexpr size_t c_max_chunks = sizeof(data_t) / c_chunk_size_in_bytes;
// However, this would introduce a circular dependency between the memory
// manager headers and this header (which probably indicates excessive
// modularization).
constexpr size_t c_max_locators = (1ULL << 29) - 1;
#else
// We allow as many locators as the number of 64B objects (the minimum size)
// that will fit into the data segment size of 256GB, or 2^38 / 2^6 = 2^32. We
// also need to account for the fact that offsets are 32 bits, and that the
// first entry of the locators array must be reserved, so we subtract 1.
constexpr size_t c_max_locators = (1ULL << 32) - 1;
#endif

// With 2^32 locators, 2^20 hash buckets bounds the average hash chain length to
// 2^12. This is still prohibitive overhead for traversal on each reference
// lookup (given that each node traversal is effectively random-access), but we
// should be able to solve this by storing locators directly in each object's
// references array rather than gaia_ids. Other expensive index lookups could be
// similarly optimized by substituting locators for gaia_ids.
constexpr size_t c_hash_buckets = 1ULL << 20;

// // Track maximum number of new chunks (apart from the one that the txn is already using)
// // that can be allocated per transaction.
// // This sets an upper bound on txn size: 32MB < txn_size < 36MB
// constexpr size_t c_max_chunks_per_txn = 8;

// This sets an upper bound on the size of the transaction.
constexpr size_t c_max_txn_memory_size_bytes = 64 * 1024 * 1024;

// 8 chunks can hold up to 8 * (2^16 - 2^8) = 522240 64B objects,
constexpr size_t c_max_log_records = 522240;

// This is an array of offsets in the data segment corresponding to object
// versions, where each array index is referred to as a "locator."
// The first entry of the array is reserved for the invalid locator value 0.
// The elements are atomic because reads and writes to shared memory need to be
// synchronized across threads/processes.
typedef std::atomic<gaia_offset_t::value_type> locators_t[c_max_locators + 1];

struct hash_node_t
{
    // To enable atomic operations, we use the base integer type instead of gaia_id_t.
    std::atomic<common::gaia_id_t::value_type> id;
    std::atomic<size_t> next_offset;
    std::atomic<gaia_locator_t::value_type> locator;
};

struct txn_log_t
{
    gaia_txn_id_t begin_ts;
    size_t record_count;
    // size_t chunk_count;

    struct log_record_t
    {
        gaia_locator_t locator;
        gaia_offset_t old_offset;
        gaia_offset_t new_offset;

        friend std::ostream& operator<<(std::ostream& os, const log_record_t& lr)
        {
            os << "locator: "
               << lr.locator
               << "\told_offset: "
               << lr.old_offset
               << "\tnew_offset: "
               << lr.new_offset
               << "\toperation: "
               << lr.operation()
               << std::endl;
            return os;
        }

        inline gaia_operation_t operation() const
        {
            bool is_old_offset_valid = old_offset.is_valid();
            bool is_new_offset_valid = new_offset.is_valid();
            if (is_old_offset_valid && is_new_offset_valid)
            {
                return gaia_operation_t::update;
            }
            else if (!is_old_offset_valid && is_new_offset_valid)
            {
                return gaia_operation_t::create;
            }
            else if (is_old_offset_valid && !is_new_offset_valid)
            {
                return gaia_operation_t::remove;
            }
            else
            {
                ASSERT_UNREACHABLE("At least one offset in a log record must be valid!");
            }
        }
    };

    log_record_t log_records[c_max_log_records];
    // gaia_offset_t chunks[c_max_chunks_per_txn];

    friend std::ostream& operator<<(std::ostream& os, const txn_log_t& l)
    {
        os << "begin_ts: " << l.begin_ts << std::endl;
        os << "record_count: " << l.record_count << std::endl;
        for (size_t i = 0; i < l.record_count; ++i)
        {
            os << l.log_records[i];
        }
        os << std::endl;
        return os;
    }

    inline size_t size()
    {
        return sizeof(txn_log_t) + (sizeof(txn_log_t::log_record_t) * record_count);
    }
};

constexpr size_t c_initial_log_size = sizeof(txn_log_t) + (sizeof(txn_log_t::log_record_t) * c_max_log_records);

struct counters_t
{
    // These fields are used as cross-process atomic counters. We don't need
    // something like a cross-process mutex for this, as long as we use atomic
    // intrinsics for mutating the counters. This is because the instructions
    // targeted by the intrinsics operate at the level of physical memory, not
    // virtual addresses.
    // NB: All these fields are initialized to 0, even though C++ doesn't guarantee
    // it, because this struct is constructed in a memory-mapped shared-memory
    // segment, and the OS automatically zeroes new pages.
    //
    // To enable atomic operations, we use the base integer types of each custom Gaia type.
    std::atomic<common::gaia_id_t::value_type> last_id;
    std::atomic<common::gaia_type_t::value_type> last_type_id;
    std::atomic<gaia_txn_id_t::value_type> last_txn_id;
    std::atomic<gaia_locator_t::value_type> last_locator;
};

struct data_t
{
    // We use std::aligned_storage to ensure that the backing array can only be
    // accessed at the proper alignment (64 bytes). An instance of db_object_t
    // is always aligned to the beginning of an element of the array, but it may
    // occupy multiple elements of the array.
    // The first entry of the array is reserved for the invalid offset value 0.
    aligned_storage_for_t<db_object_t> objects[c_max_locators + 1];
};

// This is a shared-memory hash table mapping gaia_id keys to locator values. We
// need a hash table node for each locator (to store the gaia_id key and the
// locator value).
struct id_index_t
{
    size_t hash_node_count;
    hash_node_t hash_nodes[c_hash_buckets + c_max_locators];
};

// These are types meant to access index types from the client/server.
namespace index
{
class base_index_t;
typedef std::shared_ptr<base_index_t> db_index_t;
typedef std::unordered_map<common::gaia_id_t, db_index_t> indexes_t;

} // namespace index

} // namespace db
} // namespace gaia
