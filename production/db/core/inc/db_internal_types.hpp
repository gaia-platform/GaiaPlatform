/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>

#include <iostream>
#include <ostream>

#include "gaia/common.hpp"

#include "gaia_internal/common/mmap_helpers.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/db/db_types.hpp"

namespace gaia
{
namespace db
{

enum class gaia_operation_t : uint8_t
{
    not_set = 0x0,
    create = 0x1,
    update = 0x2,
    remove = 0x3,
    clone = 0x4
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
    case gaia_operation_t::clone:
        os << "clone";
        break;
    default:
        gaia::common::retail_assert(false, "Unknown value of gaia_operation_t!");
    }
    return os;
}

constexpr char c_server_connect_socket_name[] = "gaia_db_server";

constexpr char c_gaia_mem_locators[] = "gaia_mem_locators";
constexpr char c_gaia_mem_counters[] = "gaia_mem_counters";
constexpr char c_gaia_mem_data[] = "gaia_mem_data";
constexpr char c_gaia_mem_id_index[] = "gaia_mem_id_index";

constexpr char c_gaia_mem_txn_log[] = "gaia_mem_txn_log";

// We allow as many locators as the number of 64B objects (the minimum size)
// that will fit into 256GB, or 2^38 / 2^6 = 2^32.
constexpr size_t c_max_locators = 1ULL << 32;

// With 2^32 objects, 2^20 hash buckets bounds the average hash chain length to
// 2^12. This is still prohibitive overhead for traversal on each reference
// lookup (given that each node traversal is effectively random-access), but we
// should be able to solve this by storing locators directly in each object's
// references array rather than gaia_ids. Other expensive index lookups could be
// similarly optimized by substituting locators for gaia_ids.
constexpr size_t c_hash_buckets = 1ULL << 20;

// This is arbitrary, but we need to keep txn logs to a reasonable size.
constexpr size_t c_max_log_records = 1ULL << 20;

// This is an array of offsets in the data segment corresponding to object
// versions, where each array index is referred to as a "locator."
typedef gaia_offset_t locators_t[c_max_locators];

struct hash_node_t
{
    gaia::common::gaia_id_t id;
    size_t next_offset;
    gaia::db::gaia_locator_t locator;
};

struct txn_log_t
{
    gaia_txn_id_t begin_ts;
    size_t count;

    struct log_record_t
    {
        gaia::db::gaia_locator_t locator;
        gaia::db::gaia_offset_t old_offset;
        gaia::db::gaia_offset_t new_offset;
        gaia::common::gaia_id_t deleted_id;
        gaia_operation_t operation;

        friend std::ostream& operator<<(std::ostream& os, const log_record_t& lr)
        {
            os << "locator: "
               << lr.locator
               << "\told_offset: "
               << lr.old_offset
               << "\tnew_offset: "
               << lr.new_offset
               << "\tdeleted_id: "
               << lr.deleted_id
               << "\toperation: "
               << lr.operation
               << std::endl;
            return os;
        }
    };

    log_record_t log_records[c_max_log_records];

    friend std::ostream& operator<<(std::ostream& os, const txn_log_t& l)
    {
        os << "count: " << l.count << std::endl;
        const log_record_t* const lr_start = static_cast<const log_record_t*>(l.log_records);
        for (const log_record_t* lr = lr_start; lr < lr_start + l.count; ++lr)
        {
            os << *lr << std::endl;
        }
        return os;
    }

    inline size_t size()
    {
        return sizeof(txn_log_t) + (sizeof(txn_log_t::log_record_t) * count);
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
    // REVIEW: these fields should probably be changed to std::atomic<T> (and
    // the explicit calls to atomic intrinsics replaced by atomic methods). NB:
    // all these fields are initialized to 0, even though C++ doesn't guarantee
    // it, because this struct is constructed in a memory-mapped shared-memory
    // segment, and the OS automatically zeroes new pages.
    gaia::common::gaia_id_t last_id;
    gaia::common::gaia_type_t last_type_id;
    gaia::db::gaia_txn_id_t last_txn_id;
    gaia::db::gaia_locator_t last_locator;
};

struct data_t
{
    // This array is actually an untyped array of bytes, but it's defined as an
    // array of uint64_t just to enforce 8-byte alignment. Allocating
    // (c_max_locators * 8) 8-byte words for this array means we reserve 64
    // bytes on average for each object we allocate (or 1 cache line on every
    // common architecture). Since any valid offset must be positive (zero is a
    // reserved invalid value), the first word (at offset 0) is unused by data,
    // so we use it to store the last offset allocated (minus 1 since all
    // offsets are obtained by incrementing the counter by 1).
    // NB: We now align all objects on a 64-byte boundary (for cache efficiency
    // and to allow us to later switch to 32-bit offsets).
    alignas(64) uint64_t objects[c_max_locators * 8];
};

// This is a shared-memory hash table mapping gaia_id keys to locator values. We
// need a hash table node for each locator (to store the gaia_id key and the
// locator value).
struct id_index_t
{
    size_t hash_node_count;
    hash_node_t hash_nodes[c_hash_buckets + c_max_locators];
};

// Base class abstracting common functionality for mapped_data_t and mapped_log_t classes.
template <typename T>
class base_mapped_data_t
{
public:
    base_mapped_data_t()
    {
        reset();
    }

    base_mapped_data_t(base_mapped_data_t& other)
    {
        take_ownership(other);
    }

    base_mapped_data_t& operator=(base_mapped_data_t& rhs)
    {
        take_ownership(rhs);
        return *this;
    }

    base_mapped_data_t& operator=(base_mapped_data_t&& rhs)
    {
        take_ownership(rhs);
        return *this;
    }

    ~base_mapped_data_t()
    {
        close();
    }

    virtual void reset()
    {
        m_is_initialized = false;
        m_fd = -1;
        m_data = nullptr;
        m_mapped_data_size = 0;
    }

    // Unmaps the data and closes the file descriptor, if one is tracked.
    // This permits manual cleanup, before instance destruction time.
    virtual void close()
    {
        gaia::common::unmap_fd_data(m_data, m_mapped_data_size);
        m_mapped_data_size = 0;

        gaia::common::close_fd(m_fd);

        m_is_initialized = false;
    }

    T* data()
    {
        return m_data;
    }

    int fd()
    {
        return m_fd;
    }

    bool is_initialized()
    {
        return m_is_initialized;
    }

protected:
    virtual void take_ownership(base_mapped_data_t<T>& other)
    {
        gaia::common::retail_assert(
            !m_is_initialized,
            "An initialized base_mapped_data_t instance cannot take ownership of another instance!");

        m_is_initialized = other.m_is_initialized;
        m_fd = other.m_fd;
        m_data = other.m_data;
        m_mapped_data_size = other.m_mapped_data_size;

        other.reset();
    }

protected:
    bool m_is_initialized;
    int m_fd;
    T* m_data;

    // This is used to track the mapped data size, so we can call munmap() with the same value.
    size_t m_mapped_data_size;
};

// This class abstracts the server and client operations with memory-mapped data.
// T indicates the type of data structure that is managed by an instance of this class.
template <typename T>
class mapped_data_t : public base_mapped_data_t<T>
{
public:
    mapped_data_t() = default;
    mapped_data_t(mapped_data_t& other) = default;
    mapped_data_t& operator=(mapped_data_t& rhs) = default;
    mapped_data_t& operator=(mapped_data_t&& rhs) = default;
    ~mapped_data_t() = default;

    // Creates a memory-mapping for a data structure.
    void create(const char* name)
    {
        gaia::common::retail_assert(
            !this->m_is_initialized,
            "Calling create() on an already initialized mapped_data_t instance!");

        this->m_fd = ::memfd_create(name, MFD_ALLOW_SEALING);
        if (this->m_fd == -1)
        {
            gaia::common::throw_system_error("memfd_create() failed in mapped_data_t::create()!");
        }

        this->m_mapped_data_size = sizeof(T);

        gaia::common::truncate_fd(this->m_fd, this->m_mapped_data_size);

        // Note that unless we supply the MAP_POPULATE flag to mmap(), only the
        // pages we actually use will ever be allocated. However, Linux may refuse
        // to allocate the virtual memory we requested if overcommit is disabled
        // (i.e., /proc/sys/vm/overcommit_memory = 2). Using MAP_NORESERVE (don't
        // reserve swap space) should ensure that overcommit always succeeds, unless
        // it is disabled. We may need to document the requirement that overcommit
        // is not disabled (i.e., /proc/sys/vm/overcommit_memory != 2).
        //
        // Alternatively, we could use the more tedious but reliable approach of
        // using mmap(PROT_NONE) and calling mprotect(PROT_READ|PROT_WRITE) on any
        // pages we need to use (this is analogous to VirtualAlloc(MEM_RESERVE)
        // followed by VirtualAlloc(MEM_COMMIT) in Windows).
        gaia::common::map_fd_data(
            this->m_data,
            this->m_mapped_data_size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_NORESERVE,
            this->m_fd,
            0);

        this->m_is_initialized = true;
    }

    // Opens a memory-mapped structure using a file descriptor.
    // manage_fd is used to indicate whether the fd should be managed
    // (i.e. closed at destruction time) by this class or not.
    //
    // Note: manage_fd also impacts the type of mapping: SHARED if true; PRIVATE otherwise.
    // This is done for coding convenience because it suits current implementation,
    // but could be changed in the future if we wish more control over this behavior.
    void open(int fd, bool manage_fd = true)
    {
        gaia::common::retail_assert(
            !this->m_is_initialized,
            "Calling open() on an already initialized mapped_data_t instance!");

        gaia::common::retail_assert(fd != -1, "mapped_data_t::open() was called with an invalid fd!");

        this->m_mapped_data_size = sizeof(T);

        if (manage_fd)
        {
            this->m_fd = fd;

            gaia::common::map_fd_data(
                this->m_data,
                this->m_mapped_data_size,
                PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_NORESERVE,
                this->m_fd,
                0);

            gaia::common::close_fd(this->m_fd);
        }
        else
        {
            gaia::common::map_fd_data(
                this->m_data,
                this->m_mapped_data_size,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_NORESERVE,
                fd,
                0);
        }

        this->m_is_initialized = true;
    }
};

// This is a variant of mapped_data_t that is specialized for operation on log data structures.
// There are enough differences from mapped_data_t to warrant a separate implementation.
class mapped_log_t : public base_mapped_data_t<txn_log_t>
{
public:
    mapped_log_t() = default;
    mapped_log_t(mapped_log_t& other) = default;
    mapped_log_t& operator=(mapped_log_t& rhs) = default;
    mapped_log_t& operator=(mapped_log_t&& rhs) = default;
    ~mapped_log_t() = default;

    // Creates a memory-mapping for a log data structure.
    void create(const char* name)
    {
        gaia::common::retail_assert(
            !this->m_is_initialized,
            "Calling create() on an already initialized mapped_log_t instance!");

        this->m_fd = ::memfd_create(name, MFD_ALLOW_SEALING);
        if (this->m_fd == -1)
        {
            gaia::common::throw_system_error("memfd_create() failed in mapped_log_t::create()!");
        }

        this->m_mapped_data_size = c_initial_log_size;

        gaia::common::truncate_fd(this->m_fd, this->m_mapped_data_size);

        gaia::common::map_fd_data(
            this->m_data,
            this->m_mapped_data_size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            this->m_fd,
            0);

        this->m_is_initialized = true;
    }

    // Opens a memory-mapped log structure using a file descriptor.
    void open(int fd)
    {
        gaia::common::retail_assert(
            !this->m_is_initialized,
            "Calling open() on an already initialized mapped_log_t instance!");

        gaia::common::retail_assert(fd != -1, "mapped_log_t::open() was called with an invalid fd!");

        this->m_mapped_data_size = gaia::common::get_fd_size(fd);

        gaia::common::map_fd_data(
            this->m_data,
            this->m_mapped_data_size,
            PROT_READ,
            MAP_PRIVATE,
            fd,
            0);

        this->m_is_initialized = true;
    }

    // Truncates and seals a memory-mapped log structure.
    // Closes the mapped_log_t instance in the sense that it is left in an uninitialized state.
    // The file descriptor is *NOT* closed.
    // Passes back the file descriptor and the size of the log.
    void truncate_seal_and_close(int& fd, size_t& log_size)
    {
        gaia::common::retail_assert(
            this->m_is_initialized,
            "Calling truncate_seal_and_close() on an uninitialized mapped_log_t instance!");

        gaia::common::retail_assert(
            this->m_fd != -1,
            "truncate_seal_and_close() was called on a mapped_log_t instance that lacks a valid fd!");

        fd = this->m_fd;
        log_size = this->m_data->size();

        gaia::common::unmap_fd_data(this->m_data, this->m_mapped_data_size);
        this->m_mapped_data_size = 0;

        gaia::common::truncate_fd(this->m_fd, log_size);

        // Seal the txn log memfd for writes/resizing before sending it to the server.
        if (-1 == ::fcntl(this->m_fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE))
        {
            gaia::common::throw_system_error(
                "fcntl(F_ADD_SEALS) failed in mapped_log_t::truncate_seal_and_close()!");
        }

        this->m_fd = -1;

        this->m_is_initialized = false;
    }
};

} // namespace db
} // namespace gaia
