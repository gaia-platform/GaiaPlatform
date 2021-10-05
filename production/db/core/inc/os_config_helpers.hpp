/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <fcntl.h>

#include <charconv>

#include <libexplain/getrlimit.h>
#include <libexplain/open.h>
#include <libexplain/read.h>
#include <sys/resource.h>

#include "gaia/exception.hpp"

#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/scope_guard.hpp"
#include "gaia_internal/common/system_error.hpp"

namespace gaia
{
namespace db
{
namespace os
{

// https://www.kernel.org/doc/html/latest/admin-guide/sysctl/vm.html says
// the default is 65536, but Ubuntu seems to default to 65530.
constexpr uint64_t c_min_vma_limit{65530};

// https://man7.org/linux/man-pages/man2/getrlimit.2.html:
// "This [RLIMIT_AS] is the maximum size of the process's virtual memory
// (address space).  The limit is specified in bytes, and is
// rounded down to the system page size."
// NB: POSIX guarantees that rlim_t is an unsigned integer
// (https://pubs.opengroup.org/onlinepubs/009695399/basedefs/sys/resource.h.html).
constexpr uint64_t c_min_vm_limit{RLIM_INFINITY};

// https://man7.org/linux/man-pages/man2/getrlimit.2.html: "This [RLIMIT_NOFILE]
// specifies a value one greater than the maximum file descriptor number that
// can be opened by this process."
// NB: (2^16 - 1) is the "invalid" value of a txn log fd, so the maximum valid
// value of a txn log fd is (2^16 - 2). Also, we plan to support up to 2^7
// sessions (to avoid exhausting virtual memory), and each session requires a
// few fds for its socket fd, epoll fd, cancellation eventfd, server-side txn
// log fd, etc., so (2^16 - 1 + 512) seems to be a reasonable minimum hard
// limit. (The default hard limit on Ubuntu 20.04, as reported by `prlimit -n`,
// seems to be 2^20, while the default soft limit seems to be 2^10, so a soft
// limit of (2^16 + 511) is compatible with our preferred platform, in the sense
// that it should not require any reconfiguration by a privileged
// administrator.)
constexpr uint64_t c_min_fd_limit{std::numeric_limits<uint16_t>::max() + 512};

inline bool is_little_endian()
{
    uint32_t val = 1;
    uint8_t least_significant_byte = *(reinterpret_cast<uint8_t*>(&val));
    return (least_significant_byte == val);
}

inline uint64_t read_integer_from_proc_fd(int proc_fd)
{
    char digits[16];
    ssize_t bytes_read = ::read(proc_fd, &digits, sizeof(digits));
    if (bytes_read == -1)
    {
        int err = errno;
        const char* reason = ::explain_read(proc_fd, &digits, sizeof(digits));
        throw gaia::common::system_error(reason, err);
    }

    // Replace the trailing newline terminator with a null terminator.
    ASSERT_POSTCONDITION(digits[bytes_read - 1] == '\n', "Expected trailing newline!");
    digits[bytes_read - 1] = '\0';

    int value{};
    const auto result = std::from_chars(digits, digits + bytes_read - 1, value);
    ASSERT_POSTCONDITION(result.ec == std::errc{}, "Failed to convert bytes to integer!");

    return static_cast<uint64_t>(value);
}

inline bool is_overcommit_unlimited()
{
    // See https://www.kernel.org/doc/html/latest/admin-guide/sysctl/vm.html and
    // https://www.kernel.org/doc/Documentation/vm/overcommit-accounting.
    int proc_fd = ::open("/proc/sys/vm/overcommit_memory", O_RDONLY, 0);
    if (proc_fd == -1)
    {
        int err = errno;
        const char* reason = ::explain_open("/proc/sys/vm/overcommit_memory", O_RDONLY, 0);
        throw gaia::common::system_error(reason, err);
    }
    auto cleanup_proc_fd = gaia::common::scope_guard::make_scope_guard(
        [&]() { gaia::common::close_fd(proc_fd); });

    uint64_t value = read_integer_from_proc_fd(proc_fd);
    if (value != 1)
    {
        std::cerr << "vm.overcommit_memory has value " << value << ", but the only supported value is 1." << std::endl;
        return false;
    }

    return true;
}

inline bool check_vma_limit()
{
    // See https://www.kernel.org/doc/html/latest/admin-guide/sysctl/vm.html.
    int proc_fd = ::open("/proc/sys/vm/max_map_count", O_RDONLY, 0);
    if (proc_fd == -1)
    {
        int err = errno;
        const char* reason = ::explain_open("/proc/sys/vm/max_map_count", O_RDONLY, 0);
        throw gaia::common::system_error(reason, err);
    }
    auto cleanup_proc_fd = gaia::common::scope_guard::make_scope_guard(
        [&]() { gaia::common::close_fd(proc_fd); });

    uint64_t value = read_integer_from_proc_fd(proc_fd);
    if (value < c_min_vma_limit)
    {
        std::cerr << "vm.max_map_count has value " << value << ", which is smaller than the minimum required, " << c_min_vma_limit << "." << std::endl;
        return false;
    }

    return true;
}

inline bool check_and_adjust_resource_limit(int rlimit_id, const char* rlimit_desc, uint64_t min_hard_limit)
{
    // See https://man7.org/linux/man-pages/man2/getrlimit.2.html.
    rlimit old_rlimit{};

    if (-1 == ::getrlimit(rlimit_id, &old_rlimit))
    {
        int err = errno;
        const char* reason = ::explain_getrlimit(rlimit_id, &old_rlimit);
        throw gaia::common::system_error(reason, err);
    }

    // Fail if the hard limit does not meet our requirements. (Even if our
    // process has the privileges to adjust the hard limit, don't attempt this,
    // because we don't want to second-guess the system administrator.) If the
    // soft limit does not meet our requirements, but the hard limit does, then
    // try to adjust the soft limit to the hard limit.

    // POSIX guarantees that rlim_t is an unsigned integer
    // (https://pubs.opengroup.org/onlinepubs/009695399/basedefs/sys/resource.h.html).
    uint64_t soft_limit = old_rlimit.rlim_cur;
    uint64_t hard_limit = old_rlimit.rlim_max;

    if (hard_limit < min_hard_limit)
    {
        std::cerr << "The per-process " << rlimit_desc << " hard limit is " << hard_limit << ", rather than " << min_hard_limit << "." << std::endl;
        return false;
    }

    if (soft_limit < hard_limit)
    {
        // Try to increase the soft limit to the hard limit.
        rlimit new_rlimit{min_hard_limit, min_hard_limit};
        if (-1 == ::setrlimit(rlimit_id, &new_rlimit))
        {
            // If we don't have privileges, return failure.
            if (errno == EPERM)
            {
                std::cerr << "The current process does not have sufficient privileges to increase the " << rlimit_desc << " soft limit to " << min_hard_limit << "." << std::endl;
                return false;
            }

            // For all other failures, throw an exception.
            // Sadly, explain_setrlimit() is not implemented.
            gaia::common::throw_system_error("setrlimit() failed!");
        }
    }

    return true;
}

inline bool check_and_adjust_vm_limit()
{
    return check_and_adjust_resource_limit(RLIMIT_AS, "virtual address space", c_min_vm_limit);
}

inline bool check_and_adjust_fd_limit()
{
    return check_and_adjust_resource_limit(RLIMIT_NOFILE, "open file descriptor", c_min_fd_limit);
}

} // namespace os
} // namespace db
} // namespace gaia
