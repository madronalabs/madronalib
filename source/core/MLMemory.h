//
//  MLMemory.h
//  madronalib
//
//  Created by Randy Jones on 3/3/17.
//
//

#pragma once

// macros to compile the Eigen functions we are using

#define EIGEN_DEVICE_FUNC
#define EIGEN_DEFAULT_ALIGN_BYTES 16

// Suppresses 'unused variable' warnings.
namespace Eigen {
	namespace internal {
		template<typename T> EIGEN_DEVICE_FUNC void ignore_unused_variable(const T&) {}
	}
}
#define EIGEN_UNUSED_VARIABLE(var) Eigen::internal::ignore_unused_variable(var);

#  define EIGEN_TRY if (true)
#  define EIGEN_CATCH(X) else
#    define EIGEN_THROW_X(X) std::abort()
#    define EIGEN_THROW std::abort()

#define EIGEN_ALWAYS_INLINE inline 

#ifdef _WIN32
	#define EIGEN_MALLOC_ALREADY_ALIGNED 0
#else
	#define EIGEN_MALLOC_ALREADY_ALIGNED 1
#endif


#ifdef NDEBUG
	#define eigen_assert(x)
#else
	namespace Eigen {
		namespace internal {
			inline bool copy_bool(bool b) { return b; }
		}
	}
	#define eigen_assert(x) assert(x)
#endif

// This file is part of Eigen, a lightweight C++ template library
// for linear algebra.
//
// Copyright (C) 2008-2015 Gael Guennebaud <gael.guennebaud@inria.fr>
// Copyright (C) 2008-2009 Benoit Jacob <jacob.benoit.1@gmail.com>
// Copyright (C) 2009 Kenneth Riddile <kfriddile@yahoo.com>
// Copyright (C) 2010 Hauke Heibel <hauke.heibel@gmail.com>
// Copyright (C) 2010 Thomas Capricelli <orzel@freehackers.org>
// Copyright (C) 2013 Pavel Holoborodko <pavel@holoborodko.com>
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.


namespace Eigen {
	
	namespace internal {
		
		EIGEN_DEVICE_FUNC 
		inline void throw_std_bad_alloc()
		{
#ifdef EIGEN_EXCEPTIONS
			throw std::bad_alloc();
#else
			std::size_t huge = static_cast<std::size_t>(-1);
			new int[huge];
#endif
		}
		
		/*****************************************************************************
		 *** Implementation of handmade aligned functions                           ***
		 *****************************************************************************/
		
		/* ----- Hand made implementations of aligned malloc/free and realloc ----- */
		
		/** \internal Like malloc, but the returned pointer is guaranteed to be 16-byte aligned.
		 * Fast, but wastes 16 additional bytes of memory. Does not throw any exception.
		 */
		inline void* handmade_aligned_malloc(std::size_t size)
		{
			void *original = std::malloc(size+EIGEN_DEFAULT_ALIGN_BYTES);
			if (original == 0) return 0;
			void *aligned = reinterpret_cast<void*>((reinterpret_cast<std::size_t>(original) & ~(std::size_t(EIGEN_DEFAULT_ALIGN_BYTES-1))) + EIGEN_DEFAULT_ALIGN_BYTES);
			*(reinterpret_cast<void**>(aligned) - 1) = original;
			return aligned;
		}
		
		/** \internal Frees memory allocated with handmade_aligned_malloc */
		inline void handmade_aligned_free(void *ptr)
		{
			if (ptr) std::free(*(reinterpret_cast<void**>(ptr) - 1));
		}
		
		/** \internal
		 * \brief Reallocates aligned memory.
		 * Since we know that our handmade version is based on std::malloc
		 * we can use std::realloc to implement efficient reallocation.
		 */
		inline void* handmade_aligned_realloc(void* ptr, std::size_t size, std::size_t = 0)
		{
			if (ptr == 0) return handmade_aligned_malloc(size);
			void *original = *(reinterpret_cast<void**>(ptr) - 1);
			std::ptrdiff_t previous_offset = static_cast<char *>(ptr)-static_cast<char *>(original);
			original = std::realloc(original,size+EIGEN_DEFAULT_ALIGN_BYTES);
			if (original == 0) return 0;
			void *aligned = reinterpret_cast<void*>((reinterpret_cast<std::size_t>(original) & ~(std::size_t(EIGEN_DEFAULT_ALIGN_BYTES-1))) + EIGEN_DEFAULT_ALIGN_BYTES);
			void *previous_aligned = static_cast<char *>(original)+previous_offset;
			if(aligned!=previous_aligned)
    std::memmove(aligned, previous_aligned, size);
			
			*(reinterpret_cast<void**>(aligned) - 1) = original;
			return aligned;
		}
		
		/*****************************************************************************
		 *** Implementation of portable aligned versions of malloc/free/realloc     ***
		 *****************************************************************************/
		
#ifdef EIGEN_NO_MALLOC
		EIGEN_DEVICE_FUNC inline void check_that_malloc_is_allowed()
		{
			eigen_assert(false && "heap allocation is forbidden (EIGEN_NO_MALLOC is defined)");
		}
#elif defined EIGEN_RUNTIME_NO_MALLOC
		EIGEN_DEVICE_FUNC inline bool is_malloc_allowed_impl(bool update, bool new_value = false)
		{
			static bool value = true;
			if (update == 1)
    value = new_value;
			return value;
		}
		EIGEN_DEVICE_FUNC inline bool is_malloc_allowed() { return is_malloc_allowed_impl(false); }
		EIGEN_DEVICE_FUNC inline bool set_is_malloc_allowed(bool new_value) { return is_malloc_allowed_impl(true, new_value); }
		EIGEN_DEVICE_FUNC inline void check_that_malloc_is_allowed()
		{
			eigen_assert(is_malloc_allowed() && "heap allocation is forbidden (EIGEN_RUNTIME_NO_MALLOC is defined and g_is_malloc_allowed is false)");
		}
#else 
		EIGEN_DEVICE_FUNC inline void check_that_malloc_is_allowed()
		{}
#endif
		
		/** \internal Allocates \a size bytes. The returned pointer is guaranteed to have 16 or 32 bytes alignment depending on the requirements.
		 * On allocation error, the returned pointer is null, and std::bad_alloc is thrown.
		 */
		EIGEN_DEVICE_FUNC inline void* aligned_malloc(std::size_t size)
		{
			check_that_malloc_is_allowed();
			
			void *result;
#if (EIGEN_DEFAULT_ALIGN_BYTES==0) || EIGEN_MALLOC_ALREADY_ALIGNED
			result = std::malloc(size);
#if EIGEN_DEFAULT_ALIGN_BYTES==16
			eigen_assert((size<16 || (std::size_t(result)%16)==0) && "System's malloc returned an unaligned pointer. Compile with EIGEN_MALLOC_ALREADY_ALIGNED=0 to fallback to handmade alignd memory allocator.");
#endif
#else
			result = handmade_aligned_malloc(size);
#endif
			
			if(!result && size)
    throw_std_bad_alloc();
			
			return result;
		}
		
		/** \internal Frees memory allocated with aligned_malloc. */
		EIGEN_DEVICE_FUNC inline void aligned_free(void *ptr)
		{
#if (EIGEN_DEFAULT_ALIGN_BYTES==0) || EIGEN_MALLOC_ALREADY_ALIGNED
			std::free(ptr);
#else
			handmade_aligned_free(ptr);
#endif
		}
		
		/**
		 * \internal
		 * \brief Reallocates an aligned block of memory.
		 * \throws std::bad_alloc on allocation failure
		 */
		inline void* aligned_realloc(void *ptr, std::size_t new_size, std::size_t old_size)
		{
			EIGEN_UNUSED_VARIABLE(old_size);
			
			void *result;
#if (EIGEN_DEFAULT_ALIGN_BYTES==0) || EIGEN_MALLOC_ALREADY_ALIGNED
			result = std::realloc(ptr,new_size);
#else
			result = handmade_aligned_realloc(ptr,new_size,old_size);
#endif
			
			if (!result && new_size)
    throw_std_bad_alloc();
			
			return result;
		}
		
		/*****************************************************************************
		 *** Implementation of conditionally aligned functions                      ***
		 *****************************************************************************/
		
		/** \internal Allocates \a size bytes. If Align is true, then the returned ptr is 16-byte-aligned.
		 * On allocation error, the returned pointer is null, and a std::bad_alloc is thrown.
		 */
		template<bool Align> EIGEN_DEVICE_FUNC inline void* conditional_aligned_malloc(std::size_t size)
		{
			return aligned_malloc(size);
		}
		
		template<> EIGEN_DEVICE_FUNC inline void* conditional_aligned_malloc<false>(std::size_t size)
		{
			check_that_malloc_is_allowed();
			
			void *result = std::malloc(size);
			if(!result && size)
    throw_std_bad_alloc();
			return result;
		}
		
		/** \internal Frees memory allocated with conditional_aligned_malloc */
		template<bool Align> EIGEN_DEVICE_FUNC inline void conditional_aligned_free(void *ptr)
		{
			aligned_free(ptr);
		}
		
		template<> EIGEN_DEVICE_FUNC inline void conditional_aligned_free<false>(void *ptr)
		{
			std::free(ptr);
		}
		
		template<bool Align> inline void* conditional_aligned_realloc(void* ptr, std::size_t new_size, std::size_t old_size)
		{
			return aligned_realloc(ptr, new_size, old_size);
		}
		
		template<> inline void* conditional_aligned_realloc<false>(void* ptr, std::size_t new_size, std::size_t)
		{
			return std::realloc(ptr, new_size);
		}
		
		/*****************************************************************************
		 *** Construction/destruction of array elements                             ***
		 *****************************************************************************/
		
		/** \internal Destructs the elements of an array.
		 * The \a size parameters tells on how many objects to call the destructor of T.
		 */
		template<typename T> EIGEN_DEVICE_FUNC inline void destruct_elements_of_array(T *ptr, std::size_t size)
		{
			// always destruct an array starting from the end.
			if(ptr)
    while(size) ptr[--size].~T();
		}
		
		/** \internal Constructs the elements of an array.
		 * The \a size parameter tells on how many objects to call the constructor of T.
		 */
		template<typename T> EIGEN_DEVICE_FUNC inline T* construct_elements_of_array(T *ptr, std::size_t size)
		{
			std::size_t i;
			EIGEN_TRY
			{
				for (i = 0; i < size; ++i) ::new (ptr + i) T;
				return ptr;
			}
			EIGEN_CATCH(...)
			{
    destruct_elements_of_array(ptr, i);
    EIGEN_THROW;
			}
			return NULL;
		}
				
	} // end namespace internal
	
	
	
	
	/*****************************************************************************
	 *** Implementation of EIGEN_MAKE_ALIGNED_OPERATOR_NEW [_IF]                ***
	 *****************************************************************************/
	
#if EIGEN_MAX_ALIGN_BYTES!=0
#define EIGEN_MAKE_ALIGNED_OPERATOR_NEW_NOTHROW(NeedsToAlign) \
void* operator new(std::size_t size, const std::nothrow_t&) EIGEN_NO_THROW { \
EIGEN_TRY { return Eigen::internal::conditional_aligned_malloc<NeedsToAlign>(size); } \
EIGEN_CATCH (...) { return 0; } \
}
#define EIGEN_MAKE_ALIGNED_OPERATOR_NEW_IF(NeedsToAlign) \
void *operator new(std::size_t size) { \
return Eigen::internal::conditional_aligned_malloc<NeedsToAlign>(size); \
} \
void *operator new[](std::size_t size) { \
return Eigen::internal::conditional_aligned_malloc<NeedsToAlign>(size); \
} \
void operator delete(void * ptr) EIGEN_NO_THROW { Eigen::internal::conditional_aligned_free<NeedsToAlign>(ptr); } \
void operator delete[](void * ptr) EIGEN_NO_THROW { Eigen::internal::conditional_aligned_free<NeedsToAlign>(ptr); } \
void operator delete(void * ptr, std::size_t /* sz */) EIGEN_NO_THROW { Eigen::internal::conditional_aligned_free<NeedsToAlign>(ptr); } \
void operator delete[](void * ptr, std::size_t /* sz */) EIGEN_NO_THROW { Eigen::internal::conditional_aligned_free<NeedsToAlign>(ptr); } \
/* in-place new and delete. since (at least afaik) there is no actual   */ \
/* memory allocated we can safely let the default implementation handle */ \
/* this particular case. */ \
static void *operator new(std::size_t size, void *ptr) { return ::operator new(size,ptr); } \
static void *operator new[](std::size_t size, void* ptr) { return ::operator new[](size,ptr); } \
void operator delete(void * memory, void *ptr) EIGEN_NO_THROW { return ::operator delete(memory,ptr); } \
void operator delete[](void * memory, void *ptr) EIGEN_NO_THROW { return ::operator delete[](memory,ptr); } \
/* nothrow-new (returns zero instead of std::bad_alloc) */ \
EIGEN_MAKE_ALIGNED_OPERATOR_NEW_NOTHROW(NeedsToAlign) \
void operator delete(void *ptr, const std::nothrow_t&) EIGEN_NO_THROW { \
Eigen::internal::conditional_aligned_free<NeedsToAlign>(ptr); \
} \
typedef void eigen_aligned_operator_new_marker_type;
#else
#define EIGEN_MAKE_ALIGNED_OPERATOR_NEW_IF(NeedsToAlign)
#endif
	
#define EIGEN_MAKE_ALIGNED_OPERATOR_NEW EIGEN_MAKE_ALIGNED_OPERATOR_NEW_IF(true)
#define EIGEN_MAKE_ALIGNED_OPERATOR_NEW_IF_VECTORIZABLE_FIXED_SIZE(Scalar,Size) \
EIGEN_MAKE_ALIGNED_OPERATOR_NEW_IF(bool(((Size)!=Eigen::Dynamic) && ((sizeof(Scalar)*(Size))%EIGEN_MAX_ALIGN_BYTES==0)))
	
	
} // end namespace Eigen


