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
constexpr char c_gaia_mem_logs_prefix[] = "gaia_mem_logs_";
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
constexpr size_t c_max_locators{(1ULL << 29) - 1};
#else
// We allow as many locators as the number of 64B objects (the minimum size)
// that will fit into the data segment size of 256GB, or 2^38 / 2^6 = 2^32. The
// first entry of the locators array must be reserved for the invalid locator
// value, so we subtract 1.
constexpr size_t c_max_locators{(1ULL << 32) - 1};
#endif

// With 2^32 locators, 2^20 hash buckets bounds the average hash chain length to
// 2^12. This is still prohibitive overhead for traversal on each reference
// lookup (given that each node traversal is effectively random-access), but we
// should be able to solve this by storing locators directly in each object's
// references array rather than gaia_ids. Other expensive index lookups could be
// similarly optimized by substituting locators for gaia_ids.
constexpr size_t c_hash_buckets{1ULL << 20};

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

// We can reference at most 2^16 logs from the 16 bits available in a txn
// metadata entry, and we must reserve the value 0 for an invalid log offset.
constexpr size_t c_max_logs{(1ULL << 16) - 1};

// The total size of a txn log in shared memory.
// We need to allow as many log records as the maximum number of live object
// versions, and each log record occupies 16 bytes.
constexpr size_t c_txn_log_size{((c_max_locators + 1) * sizeof(log_record_t)) / (c_max_logs + 1)};

// We want to ensure that the txn log header size never changes accidentally.
constexpr size_t c_txn_log_header_size = 16;

// We need to ensure that the log header size is a multiple of the log record size, for alignment.
static_assert(
    (c_txn_log_header_size >= sizeof(log_record_t))
        && (c_txn_log_header_size % sizeof(log_record_t) == 0),
    "Header size must be multiple of record size!");

// There can be at most 2^32 live versions in the data segment, and we can have
// at most 2^16 logs, so each log can contain at most 2^16 records, minus the
// number of record slots occupied by the log header.
constexpr size_t c_max_log_records{((c_max_locators + 1) / (c_max_logs + 1)) - (c_txn_log_header_size / sizeof(log_record_t))};

struct txn_log_t
{
    // This header should be a multiple of 16 bytes.
    gaia_txn_id_t begin_ts;
    size_t record_count;

    log_record_t log_records[c_max_log_records];

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

    // TODO: delete this after client integration
    inline size_t size()
    {
        return sizeof(txn_log_t);
    }
};

// The txn log header size may change in the future, but we want to explicitly
// assert that it is the expected size to catch any inadvertent changes.
static_assert(c_txn_log_header_size == offsetof(txn_log_t, log_records), "Txn log header size must be 16 bytes!");

static_assert(c_txn_log_size == sizeof(txn_log_t), "Txn log size must be 1MB!");

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

// Represents txn logs as offsets within the "log segment" of shared memory.
// We can fit 2^16 1MB txn logs into 64GB of memory, so txn log offsets can be
// represented as 16-bit integers. A log_offset_t value is just the offset of a
// txn log object in 1MB units, relative to the base address of the log segment.
// We use 0 to represent an invalid log offset (so the first log offset is unused).
class log_offset_t : public common::int_type_t<uint16_t, 0>
{
public:
    // By default, we should initialize to an invalid value.
    constexpr log_offset_t()
        : common::int_type_t<uint16_t, 0>()
    {
    }

    constexpr log_offset_t(uint16_t value)
        : common::int_type_t<uint16_t, 0>(value)
    {
    }
};

static_assert(
    sizeof(log_offset_t) == sizeof(log_offset_t::value_type),
    "log_offset_t has a different size than its underlying integer type!");

constexpr log_offset_t c_invalid_log_offset;

// This assertion ensures that the default type initialization
// matches the value of the invalid constant.
static_assert(
    c_invalid_log_offset.value() == log_offset_t::c_default_invalid_value,
    "Invalid c_invalid_log_offset initialization!");

// The first valid log offset (1) immediately follows the invalid log offset (0).
constexpr log_offset_t c_first_log_offset{1};
constexpr log_offset_t c_last_log_offset{c_max_logs};

// This is an array with 2^16 elements ("logs"), each holding 2^16 16-byte entries ("records").
// The first element is unused because we need to reserve offset 0 for the "invalid log offset" value.
// (This wastes only 1MB of virtual memory, which is inconsequential.)
typedef txn_log_t logs_t[c_max_logs + 1];

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
