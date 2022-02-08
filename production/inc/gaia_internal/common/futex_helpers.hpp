/////////////////////////////////////////////
// Copyright (c) Gaia Platform LLC
// All rights reserved.
/////////////////////////////////////////////

#include <linux/futex.h>      /* Definition of FUTEX_* constants */
#include <sys/syscall.h>      /* Definition of SYS_* constants */
#include <unistd.h>

namespace gaia
{
/**
 * \addtogroup gaia
 * @{
 */
namespace common
{
/**
 * \addtogroup common
 * @{
 */

/**
 * Note: glibc provides no wrapper for futex(), necessitating the use of syscall(2).
 * https://man7.org/linux/man-pages/man2/futex.2.html
 * http://locklessinc.com/articles/futex_cheat_sheet/
 * 
 * TODO: Get rid of this when the minimum Kernel version is 5.16 where the futex2 API 
 * allows 64 bit futexes.
 */
inline int sys_futex(void *addr1, int op, int val1, struct timespec *timeout, void *addr2, int val3)
{
	return syscall(SYS_futex, addr1, op, val1, timeout, addr2, val3);
}

/**
 * This operation tests that the value at the futex word
 * pointed to by the address addr still contains the
 * expected value val, and if so, then sleeps waiting for a
 * FUTEX_WAKE operation on the futex word. This set of operations is atomic.
 * Until we move to Linux 5.16 only 32 bit futexes are supported.
 * 
 * Since the most common case of using a Futex is to refer to a process-internal lock, 
 * there is an optimization available here. If the FUTEX_PRIVATE_FLAG is set on the multiplex 
 * function number passed to the syscall, then the kernel will assume that the Futex is 
 * private to the process (saving on some internal locking)
 */
inline int futex_wait(void *addr, int val)
{
    return syscall(SYS_futex, addr, FUTEX_WAIT | FUTEX_PRIVATE_FLAG, val, nullptr, nullptr, nullptr);
}

/**
 * Taking the address corresponding to the wait-queue in addr1 and the number to wake in val1.
 * If you wish to wake all the sleeping threads, simply pass INT_MAX to this function.
 * 
 * Since the most common case of using a Futex is to refer to a process-internal lock, 
 * there is an optimization available here. If the FUTEX_PRIVATE_FLAG is set on the multiplex 
 * function number passed to the syscall, then the kernel will assume that the Futex is 
 * private to the process (saving on some internal locking)
 */
inline int futex_wake(void *addr1, int val1)
{
    return syscall(SYS_futex, addr1, FUTEX_WAKE | FUTEX_PRIVATE_FLAG, val1, nullptr, nullptr, nullptr);
}

/*@}*/
} // namespace common
/*@}*/
} // namespace gaia
