/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#pragma once

#include <cstddef>
#include <cstdint>

#include <limits>

#include <sys/resource.h>

namespace gaia
{
namespace db
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

constexpr char c_warn_once_attr_name[] = "user.gaia.warn_once";
constexpr char c_warn_once_attr_value[] = "true";

// This buffer size includes the null terminator.
constexpr size_t c_path_size_bytes{256};
struct vm_overcommit_policy
{
    size_t id;
    const char* desc;
};

constexpr size_t c_heuristic_overcommit_policy_id{0};
constexpr size_t c_always_overcommit_policy_id{1};
constexpr size_t c_never_overcommit_policy_id{2};

inline constexpr vm_overcommit_policy c_vm_overcommit_policies[] = {
    {c_heuristic_overcommit_policy_id, "heuristic overcommit"},
    {c_always_overcommit_policy_id, "always overcommit"},
    {c_never_overcommit_policy_id, "never overcommit"},
};

bool is_little_endian();

uint64_t check_overcommit_policy();

// This returns true if the attribute was set and false otherwise.
bool set_warn_once_attribute();

// This returns 0 on success or `errno` on failure, with ENODATA indicating the
// attribute was not present, and ENOTSUP that extended attributes are not
// supported on this system.
int get_warn_once_attribute();

bool check_vma_limit();

bool check_and_adjust_vm_limit();

bool check_and_adjust_fd_limit();

} // namespace db
} // namespace gaia
