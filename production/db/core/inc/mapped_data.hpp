/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <vector>

#include "db_internal_types.hpp"

namespace gaia
{
namespace db
{

// Base class declaring a core management interface for mapped data classes.
struct data_mapping_t;
class base_mapped_data_t
{
    friend struct data_mapping_t;

public:
    virtual void create(const char* name) = 0;
    virtual void close() = 0;
    virtual int fd() = 0;
    virtual bool is_set() = 0;

protected:
    // This method is only meant to be called from data_mapping_t methods.
    virtual void open_shared(int fd) = 0;
};

// Core class implementing common functionality for mapped data classes.
template <typename T_data>
class core_mapped_data_t : public base_mapped_data_t
{
public:
    core_mapped_data_t();

    // Copy semantics is disabled and moves should be performed via reset().
    core_mapped_data_t(const core_mapped_data_t& other) = delete;
    core_mapped_data_t(core_mapped_data_t&& other) = delete;
    core_mapped_data_t& operator=(const core_mapped_data_t& rhs) = delete;
    core_mapped_data_t& operator=(core_mapped_data_t&& rhs) = delete;

    ~core_mapped_data_t();

    // Stops tracking any data and reverts back to uninitialized state.
    void clear();

    // Transfers data tracked by another instance into this instance.
    void reset(core_mapped_data_t<T_data>& other);

    // Unmaps the data and closes the file descriptor, if one is tracked.
    // Reverts back to uninitialized state.
    // This permits manual cleanup, before instance destruction time.
    // Can be called repeatedly.
    void close() override;

    inline T_data* data();
    inline int fd() override;
    inline bool is_set() override;

protected:
    // Non-virtual internal implementation that can be safely called from destructor.
    void close_internal();

protected:
    bool m_is_set;
    int m_fd;
    T_data* m_data;

    // This is used to track the mapped data size, so we can call unmap_fd_data()/munmap() with the same value.
    size_t m_mapped_data_size;
};

// This class abstracts the server and client operations with memory-mapped data.
// T indicates the type of data structure that is managed by an instance of this class.
template <typename T_data>
class mapped_data_t : public core_mapped_data_t<T_data>
{
public:
    mapped_data_t() = default;

    // Copy semantics is disabled and moves should be performed via reset().
    mapped_data_t(const mapped_data_t& other) = delete;
    mapped_data_t(mapped_data_t&& other) = delete;
    mapped_data_t& operator=(const mapped_data_t& rhs) = delete;
    mapped_data_t& operator=(mapped_data_t&& rhs) = delete;

    ~mapped_data_t() = default;

    // Creates a memory-mapping for a data structure.
    void create(const char* name) override;

    // Opens a memory-mapped structure using a file descriptor.
    //
    // manage_fd is used to indicate whether the fd should be managed
    // (i.e. closed at destruction time) by this class or not.
    //
    // is_shared indicates the type of mapping: SHARED if true; PRIVATE otherwise.
    void open(int fd, bool manage_fd, bool is_shared);

protected:
    void open_shared(int fd) override;
};

// This class is similar to mapped_data_t, but is specialized for operation on log data structures.
// There are enough differences from mapped_data_t to warrant a separate implementation.
class mapped_log_t : public core_mapped_data_t<txn_log_t>
{
public:
    mapped_log_t() = default;

    // Copy semantics is disabled and moves should be performed via reset().
    mapped_log_t(const mapped_log_t& other) = delete;
    mapped_log_t(mapped_log_t&& other) = delete;
    mapped_log_t& operator=(const mapped_log_t& rhs) = delete;
    mapped_log_t& operator=(mapped_log_t&& rhs) = delete;

    ~mapped_log_t() = default;

    // Creates a memory-mapping for a log data structure.
    void create(const char* name) override;

    // Opens a memory-mapped log structure using a file descriptor.
    void open(int fd);

    // Truncates and seals a memory-mapped log structure.
    // Closes the mapped_log_t instance in the sense that it is left in an uninitialized state.
    // The file descriptor is *NOT* closed - its ownership is transferred to the caller.
    // Passes back the file descriptor and the size of the log.
    void truncate_seal_and_close(int& fd, size_t& log_size);

protected:
    void open_shared(int fd) override;
};

// Structure describing a data mapping.
// This is used to enable common handling of multiple mappings based on an array of such descriptions,
// via the methods of this class.
struct data_mapping_t
{
    enum class index_t
    {
        locators,
        counters,
        data,
        id_index,

        count_mappings
    };

    index_t mapping_index;
    base_mapped_data_t* mapped_data;
    const char* name_prefix;

    // Tells whether to perform mapping (through open()) automatically.
    // If set to false, the mapping will have to be performed manually.
    bool enable_open;

    inline void create(const char* server_instance_name) const;
    inline void open(int fd) const;
    inline void close() const;
    inline int fd() const;
    inline bool is_set() const;

    inline static void validate(const data_mapping_t mappings[], size_t count_mappings);
    inline static void create(const data_mapping_t mappings[], size_t count_mappings, const char* server_instance_name);
    inline static void close(const data_mapping_t mappings[], size_t count_mappings);
    inline static void collect_fds(const data_mapping_t mappings[], size_t count_mappings, std::vector<int>& fd_list);
};

#include "mapped_data.inc"

} // namespace db
} // namespace gaia
