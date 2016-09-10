/*****************************************************************************
Copyright (c) 1995, 2016, Oracle and/or its affiliates. All Rights Reserved.
Copyright (c) 2008, Google Inc.

Portions of this file contain modifications contributed and copyrighted by
Google, Inc. Those modifications are gratefully acknowledged and are described
briefly in the InnoDB documentation. The contributions by Google are
incorporated with their permission, and subject to the conditions contained in
the file COPYING.Google.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA

*****************************************************************************/

/**************************************************//**
@file include/os0atomic.h
Macros for using atomics

Created 2012-09-23 Sunny Bains (Split from os0sync.h)
*******************************************************/

#ifndef os0atomic_h
#define os0atomic_h

#include "univ.i"
#include <my_atomic.h>

#ifdef _WIN32

/** On Windows, InterlockedExchange operates on LONG variable */
typedef LONG	lock_word_t;

#elif defined(MUTEX_FUTEX)

typedef int	lock_word_t;

# else

typedef ulint	lock_word_t;

#endif /* _WIN32 */

#ifdef _WIN32

/**********************************************************//**
Atomic compare and exchange of 32 bit unsigned integers.
@return value found before the exchange.
If it is not equal to old_value the exchange did not happen. */
UNIV_INLINE
DWORD
win_cmp_and_xchg_dword(
/*===================*/
	volatile DWORD*	ptr,		/*!< in/out: source/destination */
	DWORD		new_val,	/*!< in: exchange value */
	DWORD		old_val);	/*!< in: value to compare to */

/* windows thread objects can always be passed to windows atomic functions */
# define os_compare_and_swap_thread_id(ptr, old_val, new_val) \
	(win_cmp_and_xchg_dword(ptr, new_val, old_val) == old_val)

# define INNODB_RW_LOCKS_USE_ATOMICS
# define IB_ATOMICS_STARTUP_MSG \
	"Mutexes and rw_locks use Windows interlocked functions"

#else
/* Fall back to GCC-style atomic builtins. */

# ifdef HAVE_IB_ATOMIC_PTHREAD_T_GCC
#if defined(HAVE_GCC_SYNC_BUILTINS)
#  define os_compare_and_swap_thread_id(ptr, old_val, new_val) \
	os_compare_and_swap(ptr, old_val, new_val)
#else
UNIV_INLINE
bool
os_compare_and_swap_thread_id(volatile os_thread_id_t* ptr, os_thread_id_t old_val, os_thread_id_t new_val)
{
#ifdef HAVE_IB_GCC_ATOMIC_SEQ_CST
	return __atomic_compare_exchange_n(ptr, &old_val, new_val, 0,
			__ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
#else
	return __sync_bool_compare_and_swap(ptr, old_val, new_val);
#endif
}
#endif /* HAVE_GCC_SYNC_BUILTINS */
#  define INNODB_RW_LOCKS_USE_ATOMICS
#  define IB_ATOMICS_STARTUP_MSG \
	"Mutexes and rw_locks use GCC atomic builtins"
# else /* HAVE_IB_ATOMIC_PTHREAD_T_GCC */
#  define IB_ATOMICS_STARTUP_MSG \
	"Mutexes use GCC atomic builtins, rw_locks do not"
# endif /* HAVE_IB_ATOMIC_PTHREAD_T_GCC */

#endif

/** barrier definitions for memory ordering */
#ifdef HAVE_IB_GCC_ATOMIC_THREAD_FENCE
# define HAVE_MEMORY_BARRIER
# define os_rmb	__atomic_thread_fence(__ATOMIC_ACQUIRE)
# define os_wmb	__atomic_thread_fence(__ATOMIC_RELEASE)
# define IB_MEMORY_BARRIER_STARTUP_MSG \
	"GCC builtin __atomic_thread_fence() is used for memory barrier"

#elif defined(HAVE_IB_GCC_SYNC_SYNCHRONISE)
# define HAVE_MEMORY_BARRIER
# define os_rmb	__sync_synchronize()
# define os_wmb	__sync_synchronize()
# define IB_MEMORY_BARRIER_STARTUP_MSG \
	"GCC builtin __sync_synchronize() is used for memory barrier"

#elif defined(HAVE_IB_MACHINE_BARRIER_SOLARIS)
# define HAVE_MEMORY_BARRIER
# include <mbarrier.h>
# define os_rmb	__machine_r_barrier()
# define os_wmb	__machine_w_barrier()
# define IB_MEMORY_BARRIER_STARTUP_MSG \
	"Solaris memory ordering functions are used for memory barrier"

#elif defined(HAVE_WINDOWS_MM_FENCE) && defined(_WIN64)
# define HAVE_MEMORY_BARRIER
# include <mmintrin.h>
# define os_rmb	_mm_lfence()
# define os_wmb	_mm_sfence()
# define IB_MEMORY_BARRIER_STARTUP_MSG \
	"_mm_lfence() and _mm_sfence() are used for memory barrier"

#else
# define os_rmb
# define os_wmb
# define IB_MEMORY_BARRIER_STARTUP_MSG \
	"Memory barrier is not used"
#endif

#ifndef UNIV_NONINL
#include "os0atomic.ic"
#endif /* UNIV_NOINL */

#endif /* !os0atomic_h */
