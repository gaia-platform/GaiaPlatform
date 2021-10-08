/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include "system_checks.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <charconv>

#include <iostream>
#include <sstream>

#include <libexplain/getrlimit.h>
#include <libexplain/open.h>
#include <libexplain/read.h>
#include <libexplain/readlink.h>
#include <sys/xattr.h>

#include "gaia/exception.hpp"

#include "gaia_internal/common/fd_helpers.hpp"
#include "gaia_internal/common/retail_assert.hpp"
#include "gaia_internal/common/scope_guard.hpp"
#include "gaia_internal/common/system_error.hpp"
#include "gaia_internal/db/gaia_db_internal.hpp"

namespace gaia
{
namespace db
{

bool is_little_endian()
{
    uint32_t val = 1;
    uint8_t least_significant_byte = *(reinterpret_cast<uint8_t*>(&val));
    return (least_significant_byte == val);
}

static uint64_t read_integer_from_proc_fd(int proc_fd)
{
    constexpr size_t c_max_digits{16};
    char digits[c_max_digits];
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

static size_t get_process_path(char* buf, size_t len)
{
    // readlink(2) does not null-terminate the written value,
    // so leave a byte at the end for the null terminator.
    ssize_t bytes_read = ::readlink("/proc/self/exe", buf, len - 1);
    if (bytes_read == -1)
    {
        int err = errno;
        const char* reason = ::explain_readlink("/proc/self/exe", buf, len - 1);
        throw gaia::common::system_error(reason, err);
    }
    ASSERT_POSTCONDITION(bytes_read > 0, "Expected non-empty path!");

    // readlink(2) does not null-terminate the written value, so insert a null
    // terminator immediately after the last written byte.
    buf[bytes_read] = '\0';

    return static_cast<size_t>(bytes_read);
}

bool set_warn_once_attribute()
{
    char path[c_path_size_bytes];
    get_process_path(path, sizeof(path));

    if (-1 == ::setxattr(path, c_warn_once_attribute_name, c_warn_once_attribute_value, sizeof(c_warn_once_attribute_value), XATTR_CREATE))
    {
        // Ignore failure if the attribute already exists, or the user has
        // insufficient permissions.
        if (errno == EEXIST || errno == EACCES)
        {
            return false;
        }

        gaia::common::throw_system_error("setxattr(XATTR_CREATE) failed!");
    }

    return true;
}

int get_warn_once_attribute()
{
    char path[c_path_size_bytes];
    get_process_path(path, sizeof(path));
    char value[sizeof(c_warn_once_attribute_value)];

    ssize_t bytes_read = ::getxattr(
        path, c_warn_once_attribute_name, value, sizeof(value));
    if (bytes_read == -1)
    {
        if (errno == ENODATA || errno == ENOTSUP)
        {
            return errno;
        }
        gaia::common::throw_system_error("getxattr() failed!");
    }

    if (strcmp(value, c_warn_once_attribute_value) != 0)
    {
        std::stringstream strstream;
        strstream
            << "Unexpected value of extended attribute '"
            << c_warn_once_attribute_name << "': '" << value
            << "' (expected value: '" << c_warn_once_attribute_value << "')";
        throw common::gaia_exception(strstream.str());
    }

    return 0;
}

uint64_t check_overcommit_policy()
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

    uint64_t policy_id = read_integer_from_proc_fd(proc_fd);
    ASSERT_POSTCONDITION(
        policy_id == c_heuristic_overcommit_policy_id
            || policy_id == c_always_overcommit_policy_id
            || policy_id == c_never_overcommit_policy_id,
        "Unrecognized overcommit policy!");

    return policy_id;
}

bool check_vma_limit()
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
        std::cerr
            << "vm.max_map_count has a value of " << value
            << ", which is less than the minimum required value of "
            << c_min_vma_limit << "." << std::endl;

        return false;
    }

    return true;
}

static bool check_and_adjust_resource_limit(int rlimit_id, const char* rlimit_desc, uint64_t min_hard_limit)
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
        std::cerr
            << "The per-process " << rlimit_desc
            << " hard limit is " << hard_limit
            << ", which is less than the minimum required limit "
            << min_hard_limit << "." << std::endl;

        return false;
    }

    if (soft_limit < hard_limit)
    {
        // Try to increase the soft limit to the minimum hard limit.
        rlimit new_rlimit{min_hard_limit, hard_limit};
        if (-1 == ::setrlimit(rlimit_id, &new_rlimit))
        {
            // If we don't have privileges, return failure.
            if (errno == EPERM)
            {
                std::cerr
                    << "The " << c_db_server_name
                    << " process does not have sufficient privileges to increase the "
                    << rlimit_desc << " soft limit to " << min_hard_limit
                    << "." << std::endl;

                return false;
            }

            // For all other failures, throw an exception.
            // (Sadly, explain_setrlimit() is not implemented.)
            gaia::common::throw_system_error("setrlimit() failed!");
        }
        std::cerr
            << "The " << c_db_server_name
            << " process increased the " << rlimit_desc
            << " soft limit from " << soft_limit
            << " to " << min_hard_limit
            << "." << std::endl;
    }

    return true;
}

bool check_and_adjust_vm_limit()
{
    return check_and_adjust_resource_limit(RLIMIT_AS, "virtual address space", c_min_vm_limit);
}

bool check_and_adjust_fd_limit()
{
    return check_and_adjust_resource_limit(RLIMIT_NOFILE, "open file descriptor", c_min_fd_limit);
}

} // namespace db
} // namespace gaia
