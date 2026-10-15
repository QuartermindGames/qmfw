/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

/* This document provides platform-specific
 * headers and any additional system information.
 */

// Operating System

#if defined( _WIN32 )
#	if 0
#		define PL_SYSTEM_MAX_PATH 259
#	else
#		define PL_SYSTEM_MAX_PATH 256
#	endif
#	define PL_SYSTEM_MAX_USERNAME 128

#	ifdef _MSC_VER
#		pragma warning( disable : 4152 )
#		pragma warning( disable : 4800 )// 'type' : forcing value to bool 'true' or 'false' (performance warning)
#		pragma warning( disable : 4204 )// nonstandard extension used: non-constant aggregate initializer
#		pragma warning( disable : 4201 )// nonstandard extension used: nameless struct/union
#		pragma warning( disable : 4996 )// The POSIX name for this item is deprecated.

#		ifndef itoa
#			define itoa _itoa
#		endif
#		ifndef getcwd
#			define getcwd _getcwd
#		endif
#		ifndef snprintf
#			define snprintf _snprintf
#		endif
#		ifndef unlink
#			define unlink _unlink
#		endif
#		ifndef strcasecmp
#			define strcasecmp _stricmp
#		endif
#		ifndef mkdir
#			define mkdir _mkdir
#		endif
#		ifndef strncasecmp
#			define strncasecmp _str
#		endif
#		ifdef __cplusplus
#			ifndef nothrow
//#				define nothrow __nothrow
#			endif
#		endif
#	endif
#elif defined( __linux__ )// Linux
#	ifndef PL_IGNORE_SYSTEM_HEADERS
#		include <dirent.h>
#		include <unistd.h>
#		include <dlfcn.h>
#		include <strings.h>
#	endif

#	define PL_SYSTEM_MAX_PATH     256
#	define PL_SYSTEM_MAX_USERNAME 32

#elif defined( __FreeBSD__ )
#	ifndef PL_IGNORE_SYSTEM_HEADERS
#		include <dirent.h>
#		include <unistd.h>
#		include <dlfcn.h>
#		include <strings.h>
#	endif

#	define PL_SYSTEM_MAX_PATH     256
#	define PL_SYSTEM_MAX_USERNAME 32

#elif defined( __APPLE__ )

#	ifndef PL_IGNORE_SYSTEM_HEADERS
#		include <zconf.h>
#		include <dirent.h>
#		include <dlfcn.h>
#	endif

#	define PL_SYSTEM_MAX_PATH     512
#	define PL_SYSTEM_MAX_USERNAME 32
#else
#	error "Unsupported system type."
#endif

// Compiler

#if ( __STDC_VERSION__ == 202311L )//C23
#	define PL_DEPRECATED( function ) [[deprecated]] function
#endif

#if defined( _MSC_VER )
#	define PL_EXTERN extern

#	ifndef PL_DEPRECATED
#		define PL_DEPRECATED( function ) __declspec( deprecated ) function
#	endif

#	define PL_STATIC_ASSERT( a, b ) static_assert( ( a ), b )

#	define PL_PACKED_STRUCT_START( a )              \
		__pragma( pack( push, 1 ) ) typedef struct a \
		{
#	define PL_PACKED_STRUCT_END( a ) \
		}                             \
		( a );                        \
		__pragma( pack( pop ) )
#elif defined( __GNUC__ ) || defined( __GNUG__ )
#	define PL_EXTERN extern

#	ifndef PL_DEPRECATED
#		define PL_DEPRECATED( function ) function __attribute__( ( deprecated ) )
#	endif

#	define PL_STATIC_ASSERT( a, b ) _Static_assert( ( a ), b )

#	define PL_PACKED_STRUCT_START( a )              \
		typedef struct __attribute__( ( packed ) ) a \
		{
#	define PL_PACKED_STRUCT_END( a ) \
		}                             \
		( a );
#else
#	error "Unsupported compiler."
#endif

///////////////////////////////////////////////////////////////
//
// System specific functions -
// using them on other platforms may be undefined!
//
