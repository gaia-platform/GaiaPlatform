/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include "db_internal_types.hpp"

namespace gaia
{
namespace db
{

// Base class abstracting common functionality for mapped_data_t and mapped_log_t classes.
template <typename T>
class base_mapped_data_t
{
public:
    base_mapped_data_t();

    // Copy semantics is disabled and moves should be performed via reset().
    base_mapped_data_t(const base_mapped_data_t& other) = delete;
    base_mapped_data_t(base_mapped_data_t&& other) = delete;
    base_mapped_data_t& operator=(const base_mapped_data_t& rhs) = delete;
    base_mapped_data_t& operator=(base_mapped_data_t&& rhs) = delete;

    ~base_mapped_data_t();

    // Stops tracking any data and reverts back to uninitialized state.
    void clear();

    // Transfers data tracked by another instance into this instance.
    void reset(base_mapped_data_t<T>& other);

    // Unmaps the data and closes the file descriptor, if one is tracked.
    // Reverts back to uninitialized state.
    // This permits manual cleanup, before instance destruction time.
    // Can be called repeatedly.
    void close();

    inline T* data();
    inline int fd();
    inline bool is_set();

protected:
    bool m_is_set;
    int m_fd;
    T* m_data;

    // This is used to track the mapped data size, so we can call unmap_fd_data()/munmap() with the same value.
    size_t m_mapped_data_size;
};

// This class abstracts the server and client operations with memory-mapped data.
// T indicates the type of data structure that is managed by an instance of this class.
template <typename T>
class mapped_data_t : public base_mapped_data_t<T>
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
    void create(const char* name);

    // Opens a memory-mapped structure using a file descriptor.
    //
    // manage_fd is used to indicate whether the fd should be managed
    // (i.e. closed at destruction time) by this class or not.
    //
    // Note: manage_fd also impacts the type of mapping: SHARED if true; PRIVATE otherwise.
    // This is done for coding convenience because it suits current implementation,
    // but could be changed in the future if we wish more control over this behavior.
    void open(int fd, bool manage_fd = true);
};

// This class is similar to mapped_data_t, but is specialized for operation on log data structures.
// There are enough differences from mapped_data_t to warrant a separate implementation.
class mapped_log_t : public base_mapped_data_t<txn_log_t>
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
    void create(const char* name);

    // Opens a memory-mapped log structure using a file descriptor.
    void open(int fd);

    // Truncates and seals a memory-mapped log structure.
    // Closes the mapped_log_t instance in the sense that it is left in an uninitialized state.
    // The file descriptor is *NOT* closed - its ownership is transferred to the caller.
    // Passes back the file descriptor and the size of the log.
    void truncate_seal_and_close(int& fd, size_t& log_size);
};

#include "mapped_data.inc"

} // namespace db
} // namespace gaia
