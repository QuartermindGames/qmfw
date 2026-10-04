/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include "qmos/public/qm_os_memory.h"
#include "qmparse/public/qm_parse.h"

#include <plcore/pl_filesystem.h>
#include <plcore/pl_linkedlist.h>
#include <plcore/pl_image.h>

#if defined( _WIN32 )
#	include <windows.h>
#	ifdef CreateDirectory
#		undef CreateDirectory
#	endif
#else
#	include <fcntl.h>
#endif

#if !defined( _MSC_VER )
#	include <sys/time.h>
#endif

#include <errno.h>

#include "pl_private.h"

typedef struct PLArguments {
	const char *exe_name;
	const char *arguments[ 256 ];

	unsigned int num_arguments;
} PLArguments;

PLArguments pl_arguments;

void PlInitializeMatrixStacks_( void );

PLFunctionResult PlInitialize( int argc, char **argv ) {
	memset( &pl_arguments, 0, sizeof( PLArguments ) );
	pl_arguments.num_arguments = ( unsigned int ) argc;
	if ( pl_arguments.num_arguments > 0 ) {
		pl_arguments.exe_name = PlGetFileName( argv[ 0 ] );

		for ( unsigned int i = 0; i < pl_arguments.num_arguments; i++ ) {
			pl_arguments.arguments[ i ] = argv[ i ];
		}
	}

	PlInitPackageSubSystem();
	PlInitializeMatrixStacks_();

	return PL_RESULT_SUCCESS;
}

// Returns the name of the current executable.
const char *plGetExecutableName( void ) {
	return pl_arguments.exe_name;
}

bool PlHasCommandLineArgument( const char *arg ) {
	if ( pl_arguments.num_arguments < 1 ) {
		return false;
	}

	for ( unsigned int i = 0; i < pl_arguments.num_arguments; i++ ) {
		if ( pl_arguments.arguments[ i ] == NULL ) {
			continue;
		}

		if ( strcmp( pl_arguments.arguments[ i ], arg ) == 0 ) {
			return true;
		}
	}

	return false;
}

// Returns result for a single command line argument.
const char *qm_os_cl_get_argument_value( const char *arg ) {
	if ( arg == NULL || *arg == '\0' ) {
		return NULL;
	}

	if ( pl_arguments.num_arguments < 2 ) {
		return NULL;
	}

	for ( unsigned int i = 0; i < pl_arguments.num_arguments; i++ ) {
		if ( strcmp( pl_arguments.arguments[ i ], arg ) == 0 ) {
			if ( i + 1 >= pl_arguments.num_arguments ) {
				return NULL;
			}

			return pl_arguments.arguments[ i + 1 ];
		}
	}

	return NULL;
}

const char *PlGetCommandLineArgumentValueByIndex( unsigned int index ) {
	if ( index >= pl_arguments.num_arguments ) {
		PlReportErrorF( PL_RESULT_INVALID_PARM1, "invalid argument index" );
		return NULL;
	}

	return pl_arguments.arguments[ index ];
}

void PlShutdown( void ) {
	qm_fs_clear_mounted_locations();
}

/*-------------------------------------------------------------------
 * ERROR HANDLING
 *-----------------------------------------------------------------*/

#if defined( _WIN32 )

const char *GetLastError_strerror( uint32_t errnum ) {
	/* TODO: Make this buffer per-thread */
	static char buf[ 1024 ] = { '\0' };

	if ( !FormatMessage(
	             FORMAT_MESSAGE_FROM_SYSTEM,
	             NULL,
	             errnum,
	             MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
	             buf,
	             sizeof( buf ),
	             NULL ) ) {
		return "INTERNAL ERROR: Could not resolve error message";
	}

	/* Microsoft like to end some of their errors with newlines... */

	char *nl = strrchr( buf, '\n' );
	char *cr = strrchr( buf, '\r' );

	if ( nl != NULL && nl[ 1 ] == '\0' ) { *nl = '\0'; }
	if ( cr != NULL && cr[ 1 ] == '\0' ) { *cr = '\0'; }

	return buf;
}

#else

int GetLastError( void ) {
	return errno;
}

const char *GetLastError_strerror( int errnum ) {
	return strerror( errnum );
}

#endif

#define MAX_FUNCTION_LENGTH 64
#define MAX_ERROR_LENGTH    2048

char
        loc_error[ MAX_ERROR_LENGTH ] = { '\0' },
                                    loc_function[ MAX_FUNCTION_LENGTH ] = { '\0' };

PLFunctionResult global_result = PL_RESULT_SUCCESS;

// Returns locally generated error message.
const char *PlGetError( void ) {
	return loc_error;
}

void PlReportError( PLFunctionResult result, const char *function, const char *message, ... ) {
	va_list args;
	va_start( args, message );

	char buf[ MAX_ERROR_LENGTH ];
	vsnprintf( buf, sizeof( buf ), message, args );
	strncpy( loc_error, buf, sizeof( loc_error ) );

	va_end( args );

	snprintf( loc_function, sizeof( loc_function ), "%s", function );

	global_result = result;
}

/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC

PLFunctionResult PlGetFunctionResult( void ) {
	return global_result;
}

const char *PlGetResultString( PLFunctionResult result ) {
	switch ( result ) {
		case PL_RESULT_SUCCESS:
			return "success";
		case PL_RESULT_UNSUPPORTED:
			return "unsupported";
		case PL_RESULT_SYSERR:
			return "system error";

		case PL_RESULT_INVALID_PARM1:
			return "invalid function parameter 1";
		case PL_RESULT_INVALID_PARM2:
			return "invalid function parameter 2";
		case PL_RESULT_INVALID_PARM3:
			return "invalid function parameter 3";
		case PL_RESULT_INVALID_PARM4:
			return "invalid function parameter 4";

		// FILE I/O
		case PL_RESULT_FILEREAD:
			return "failed to read complete file!";
		case PL_RESULT_FILESIZE:
			return "failed to get valid file size!";
		case PL_RESULT_FILETYPE:
			return "invalid file type!";
		case PL_RESULT_FILEVERSION:
			return "unsupported file version!";
		case PL_RESULT_FILEPATH:
			return "invalid file path!";
		case PL_RESULT_FILEERR:
			return "filesystem error";

		// GRAPHICS
		case PL_RESULT_GRAPHICSINIT:
			return "failed to initialize graphics!";
		case PL_RESULT_INVALID_SHADER_TYPE:
			return "unsupported shader type!";
		case PL_RESULT_SHADER_COMPILE:
			return "failed to compile shader!";

		// IMAGE
		case PL_RESULT_IMAGERESOLUTION:
			return "invalid image resolution!";
		case PL_RESULT_IMAGEFORMAT:
			return "unsupported image format!";

		// MEMORY
		case PL_RESULT_MEMORY_ALLOCATION:
			return "failed to allocate memory!";
		case PL_RESULT_MEMORY_UNDERFLOW:
			return "underflow in array";

		default:
			return "an unknown error occurred";
	}
}

void PlClearError( void ) {
	loc_function[ 0 ] = loc_error[ 0 ] = '\0';
	global_result = PL_RESULT_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////////
// Time

#if defined( _MSC_VER )

// https://stackoverflow.com/a/26085827
int gettimeofday( struct timeval *tp, struct timezone *tzp ) {
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	// This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
	// until 00:00:00 January 1, 1970
	static const uint64_t EPOCH = ( ( uint64_t ) 116444736000000000ULL );

	SYSTEMTIME system_time;
	FILETIME file_time;
	uint64_t time;

	GetSystemTime( &system_time );
	SystemTimeToFileTime( &system_time, &file_time );
	time = ( ( uint64_t ) file_time.dwLowDateTime );
	time += ( ( uint64_t ) file_time.dwHighDateTime ) << 32;

	tp->tv_sec = ( long ) ( ( time - EPOCH ) / 10000000L );
	tp->tv_usec = ( long ) ( system_time.wMilliseconds * 1000 );
	return 0;
}

#endif

const char *PlGetFormattedTimeForTimestamp( uint64_t timestamp ) {
	time_t time = ( time_t ) timestamp;

	struct tm *tmPtr;
	tmPtr = gmtime( &time );

	static char timeOut[ 32 ];
	strftime( timeOut, sizeof( timeOut ), "%x %X", tmPtr );

	return timeOut;
}

const char *PlGetFormattedTime( const char *format, char *buf, size_t bufSize ) {
	struct timeval time;
	if ( gettimeofday( &time, NULL ) != 0 ) {
		return "unknown";
	}

	time_t sec = time.tv_sec;
	strftime( buf, bufSize, format, localtime( &sec ) );
	return buf;
}

/**
 * Converts the given string to time.
 * http://stackoverflow.com/questions/1765014/convert-string-from-date-into-a-time-t
 */
time_t PlStringToTime( const char *ts ) {
	char s_month[ 5 ];
	int day, year;
	sscanf( ts, "%s %d %d", s_month, &day, &year );

	static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
	int month = ( int ) ( ( strstr( months, s_month ) - months ) / 3 );
	struct tm time = { 0 };
	time.tm_mon = month;
	time.tm_mday = day;
	time.tm_year = year - 1900;
	time.tm_isdst = -1;

	return mktime( &time );
}

/**********************************************************/
/** Plugin Interface **/

#include <plcore/pl_plugin_interface.h>

static PLPluginExportTable exportTable = {
        .ReportError = PlReportError,
        .GetError = PlGetError,

        /* pl_filesystem.h */
        .OpenFile = qm_fs_file_open,
        .CloseFile = PlCloseFile,
        .GetFileData = qm_fs_file_get_data,
        .GetFileSize = qm_fs_file_get_size,

        /* pl_image.h */
        .GetImageSize = PlGetImageSize,

        .SkipWhitespace = qm_parse_skip_whitespace,
        .SkipLine = qm_parse_skip_line,
        .ParseEnclosedString = qm_parse_enclosed,
        .ParseToken = qm_parse_token,

        .SetupPath = PlSetupPath,
};

static void *p_malloc( size_t size, bool die )
{
	void *p = qm_os_memory_alloc( size, sizeof( char ), NULL );
	if ( p == NULL && die )
	{
		abort();
	}

	return p;
}

static void *p_calloc( size_t num, size_t size, bool die )
{
	void *p = qm_os_memory_alloc( num, size, NULL );
	if ( p == NULL && die )
	{
		abort();
	}

	return p;
}

static void *p_realloc( void *ptr, size_t size, bool die )
{
	void *p = qm_os_memory_realloc( ptr, size );
	if ( p == NULL && die )
	{
		abort();
	}

	return p;
}

const PLPluginExportTable *PlGetExportTable( void )
{
	exportTable.MAlloc  = p_malloc;
	exportTable.CAlloc  = p_calloc;
	exportTable.ReAlloc = p_realloc;
	exportTable.Free    = qm_os_memory_free;
	return &exportTable;
}
