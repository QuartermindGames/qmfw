/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#if !defined( _MSC_VER )
#	include <unistd.h>
#	include <dirent.h>
#endif

#include <plcore/pl_package.h>

#include "filesystem_private.h"

#include "pl_private.h"

#include "qmos/public/qm_os_linked_list.h"
#include "qmos/public/qm_os_memory.h"
#include "qmos/public/qm_os_string.h"

#if defined( _WIN32 )

#	include "3rdparty/portable_endian.h"

/*  this is required by secext.h */
#	define SECURITY_WIN32

#	include <security.h>
#	include <shlobj.h>
#	include <direct.h>
#	include <io.h>

#	if defined( _MSC_VER )
#		if !defined( S_ISREG ) && defined( S_IFMT ) && defined( S_IFREG )
#			define S_ISREG( m ) ( ( ( m ) & S_IFMT ) == S_IFREG )
#		endif
#	endif
#else
#	if defined( __APPLE__ )
#		include "3rdparty/portable_endian.h"
#	endif
#	include <pwd.h>
#endif

/////////////////////////////////////////////////////////////////////////////////////
// Low Level API
/////////////////////////////////////////////////////////////////////////////////////

int qm_fs_fclose( FILE **file )
{
	if ( *file == nullptr )
	{
		return 0;
	}

	int r = fclose( *file );
	if ( r == 0 )
	{
		*file = nullptr;
	}

	return r;
}

/* this is gross... but provide a dumb interface so we can
 * use 64-bit file calls if they're available and we're on
 * a system that supports it. */

int64_t qm_fs_ftell( FILE *file )
{
#if QM_OS_SYSTEM == QM_OS_SYSTEM_WINDOWS
#	if defined( _MSC_VER )
	return _ftelli64( file );
#	else
	return ftello64( file );
#	endif
#else
	return ftello( file );
#endif
}

int64_t qm_fs_fseek( FILE *file, uint64_t off, QmFsSeek wence )
{
#if QM_OS_SYSTEM == QM_OS_SYSTEM_WINDOWS
#	if defined( _MSC_VER )
	return _fseeki64( file, off, wence );
#	else
	return fseeko64( file, off, wence );
#	endif
#else
	return fseeko( file, off, wence );
#endif
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

/*	File System	*/

const char *PlSetupPath( PLPath dst, bool truncate, const char *msg, ... )
{
	*dst = '\0';

	va_list args;
	va_start( args, msg );
	int length = pl_vscprintf( msg, args );
	va_end( args );

	if ( length <= 0 )
	{
		return nullptr;
	}

	if ( !truncate && ( length + 1 >= sizeof( PLPath ) ) )
	{
		PlReportErrorF( PL_RESULT_MEMORY_EOA, "source path is too long" );
		return nullptr;
	}

	va_start( args, msg );
	vsnprintf( dst, sizeof( PLPath ), msg, args );
	va_end( args );

	qm_fs_normalize_path( dst, sizeof( PLPath ) );

	return dst;
}

const char *PlAppendPath( PLPath dst, const char *src, bool truncate )
{
	size_t as = strlen( dst );
	if ( !truncate && ( ( as + strlen( src ) ) >= sizeof( PLPath ) ) )
	{
		PlReportErrorF( PL_RESULT_MEMORY_EOA, "source path is too long" );
		return nullptr;
	}
	snprintf( &dst[ as ], sizeof( PLPath ) - as, "%s", src );
	qm_fs_normalize_path( dst, sizeof( PLPath ) );
	return dst;
}

const char *PlAppendPathEx( PLPath dst, bool truncate, const char *msg, ... )
{
	va_list args;
	va_start( args, msg );
	int length = pl_vscprintf( msg, args );
	va_end( args );

	if ( length <= 0 )
	{
		return nullptr;
	}

	size_t dstSize = strnlen( dst, sizeof( PLPath ) - 1 );
	if ( !truncate && ( dstSize + ( length + 1 ) >= sizeof( PLPath ) ) )
	{
		PlReportErrorF( PL_RESULT_MEMORY_EOA, "source path is too long" );
		return nullptr;
	}

	va_start( args, msg );
	vsnprintf( &dst[ dstSize ], sizeof( PLPath ) - dstSize, msg, args );
	va_end( args );

	qm_fs_normalize_path( dst, sizeof( PLPath ) );

	return dst;
}

char *qm_fs_normalize_path( char *dst, size_t dstSize )
{
	for ( size_t i = 0; i < dstSize; ++i )
	{
		if ( dst[ i ] == '\0' )
		{
			if ( dst[ i - 1 ] == '/' )
			{
				dst[ i - 1 ] = '\0';
			}
			break;
		}

		if ( dst[ i ] != '\\' )
		{
			continue;
		}

		dst[ i ] = '/';
	}

	return dst;
}

char *PlGetFolderForPath( PLPath dst, PLPath src )
{
	char *c = strrchr( src, '/' );
	if ( c == nullptr )
	{
		return nullptr;
	}

	snprintf( dst, ( ( c - src ) + 1 ), "%s", src );
	return dst;
}

/** FS Mounting **/
/** Future
 * 		- Descriptor for locations; then can use <location>:// to mount from a specific location
 */

typedef struct QmFsMount
{
	QmFsMountType       type;
	QmOsLinkedListNode *listNode;
	union
	{
		QmFsPackage *pkg;  /* PL_FS_MOUNT_PACKAGE */
		char        *path; /* PL_FS_MOUNT_DIR */
	};
} QmFsMount;

static QmOsLinkedList *mounts;

static constexpr char VFS_LOCAL_HINT[] = "local://";
#define VFS_MAX_HINT 16
#define VFS_MAX_PATH ( ( PL_SYSTEM_MAX_PATH + VFS_MAX_HINT ) + 1 )

PL_STATIC_ASSERT( sizeof( VFS_LOCAL_HINT ) < VFS_MAX_HINT, "Local hint is larger than maximum hint length, please adjust limit!" );

void PlClearMountedLocation( QmFsMount *location )
{
	if ( location->type == QM_FS_MOUNT_TYPE_PACKAGE )
	{
		PlDestroyPackage( location->pkg );
		location->pkg = nullptr;
	}
	else
	{
		qm_os_memory_free( location->path );
	}

	qm_os_memory_free( location->listNode );
	qm_os_memory_free( location );
}

void qm_fs_clear_mounted_locations()
{
	if ( mounts == nullptr )
	{
		return;
	}

	QmFsMount *mount;
	QM_OS_LINKED_LIST_ITERATE( mount, mounts, i )
	{
		PlClearMountedLocation( mount );
	}

	qm_os_memory_free( mounts );
	mounts = nullptr;
}

static QmOsLinkedListNode *push_back_mount_location( QmFsMount *location )
{
	if ( mounts == nullptr )
	{
		mounts = qm_os_linked_list_create();
		if ( mounts == nullptr )
		{
			return nullptr;
		}
	}

	return qm_os_linked_list_push_back( mounts, location );
}

QmFsMount *qm_fs_mount_local_location( const char *path )
{
	if ( PlLocalPathExists( path ) )
	{
		// attempt to mount it as a path
		QmFsMount *location = QM_OS_MEMORY_NEW( QmFsMount );
		location->type      = QM_FS_MOUNT_TYPE_DIR;

		location->listNode = push_back_mount_location( location );
		if ( location->listNode == nullptr )
		{
			qm_os_memory_free( location );
			return nullptr;
		}

		location->path = qm_os_string_alloc( "%s", path );
		qm_fs_normalize_path( location->path, sizeof( location->path ) );

		return location;
	}

	QmFsPackage *pkg = PlLoadPackage( path );
	if ( pkg != nullptr )
	{
		QmFsMount *location = QM_OS_MEMORY_NEW( QmFsMount );
		if ( location == nullptr )
		{
			goto ABORT;
		}

		location->listNode = push_back_mount_location( location );
		if ( location->listNode == nullptr )
		{
			goto ABORT;
		}

		location->type = QM_FS_MOUNT_TYPE_PACKAGE;
		location->pkg  = pkg;

		return location;

	ABORT:
		PlDestroyPackage( pkg );
		qm_os_memory_free( location );
		return nullptr;
	}

	PlReportErrorF( 0, "failed to mount location, %s", path );
	return nullptr;
}

QmFsMount *qm_fs_mount_location( const char *path )
{
	char        buf[ VFS_MAX_PATH ];
	const char *vpath = qm_fs_resolve_virtual_path( path, buf, sizeof( buf ) );
	if ( vpath == nullptr )
	{
		PlReportErrorF( PL_RESULT_FILEPATH, "failed to resolve path, %s", path );
		return nullptr;
	}

	return qm_fs_mount_local_location( vpath );
}

QmFsMountType qm_fs_mount_get_type( const QmFsMount *self )
{
	return self->type;
}

const char *qm_fs_mount_get_path( const QmFsMount *self )
{
	if ( self->type != QM_FS_MOUNT_TYPE_DIR )
	{
		return nullptr;
	}

	return self->path;
}

QmFsMount *PlGetMountLocationForPath( const char *path )
{
	if ( mounts == nullptr )
	{
		return nullptr;
	}

	QmFsMount *match       = nullptr;
	size_t     matchLength = 0;

	QmFsMount *mount;
	QM_OS_LINKED_LIST_ITERATE( mount, mounts, i )
	{
		size_t mountLength = strlen( mount->path );
		if ( match != nullptr && matchLength >= mountLength )
		{
			continue;
		}

		if ( strncmp( mount->path, path, mountLength ) == 0 )
		{
			if ( path[ mountLength ] == '\0' || path[ mountLength ] == '/' )
			{
				match       = mount;
				matchLength = mountLength;
			}
		}
	}

	return match;
}

bool qm_fs_file_is_end( const QmFsFile *self )
{
	return ( size_t ) qm_fs_file_get_offset( self ) == qm_fs_file_get_size( self );
}

time_t qm_fs_get_local_file_timestamp( const char *path )
{
	struct stat attributes;
	if ( stat( path, &attributes ) == -1 )
	{
		PlReportErrorF( PL_RESULT_FILEERR, "failed to stat %s: %s", path, strerror( errno ) );
		return 0;
	}

	return attributes.st_mtime;
}

time_t qm_fs_file_get_timestamp( QmFsFile *self )
{
	// timestamp defaults to -1 for files loaded locally
	if ( self->timeStamp < 0 )
	{
		self->timeStamp = qm_fs_get_local_file_timestamp( self->path );
	}

	return self->timeStamp;
}

// Creates a folder at the given path.
bool PlCreateDirectory( const char *path )
{
	if ( PlLocalPathExists( path ) )
	{
		return true;
	}

	if ( _pl_mkdir( path ) == 0 )
	{
		return true;
	}

	PlReportErrorF( PL_RESULT_FILEERR, "%s", strerror( errno ) );

	return false;
}

bool PlCreatePath( const char *path )
{
	size_t length = strlen( path );
	if ( length >= PL_SYSTEM_MAX_PATH )
	{
		PlReportErrorF( PL_RESULT_INVALID_PARM1, "path is greater that maximum supported path length, %d vs %d",
		                length, PL_SYSTEM_MAX_PATH );
		return false;
	}

	char dir_path[ PL_SYSTEM_MAX_PATH ] = {};
	for ( size_t i = 0; i < length; ++i )
	{
		dir_path[ i ] = path[ i ];
		if ( i != 0 &&
		     ( path[ i ] == '\\' || path[ i ] == '/' ) &&
		     ( path[ i - 1 ] != '\\' && path[ i - 1 ] != '/' ) )
		{
			if ( !PlCreateDirectory( dir_path ) )
			{
				return false;
			}
		}
	}

	return PlCreateDirectory( dir_path );
}

// Returns the extension for the file.
const char *PlGetFileExtension( const char *in )
{
	const char *s = strrchr( in, '.' );
	return s != nullptr ? s + 1 : nullptr;
}

// Strips the extension from the filename.
void PlStripExtension( char *dest, size_t length, const char *in )
{
	const char *s = strrchr( in, '.' );
	while ( in < s )
	{
		if ( --length <= 1 )
		{
			break;
		}
		*dest++ = *in++;
	}
	*dest = 0;
}

const char *PlGetFileName( const char *path )
{
	const char *lslash;
	if ( ( lslash = strrchr( path, '/' ) ) == nullptr )
	{
		lslash = strrchr( path, '\\' );
	}

	if ( lslash != nullptr )
	{
		path = lslash + 1;
	}

	return path;
}

char *PlGetUserName( char *out, size_t n )
{
#ifdef _WIN32
	char  user_string[ PL_SYSTEM_MAX_USERNAME ];
	ULONG size = PL_SYSTEM_MAX_USERNAME;
	if ( GetUserNameEx( NameDisplay, user_string, &size ) == 0 )
	{
		snprintf( user_string, sizeof( user_string ), "user" );
	}
#else// Linux
	char *user_string = getenv( "LOGNAME" );
	if ( user_string == nullptr )
	{
		user_string = "user";
	}
#endif

	strncpy( out, user_string, n );
	return out;
}

char *PlGetApplicationDataDirectory( const char *appName, char *dst, size_t dstSize )
{
#if QM_OS_SYSTEM == QM_OS_SYSTEM_LINUX || QM_OS_SYSTEM == QM_OS_SYSTEM_MACOS
	const char *home;
	if ( ( home = getenv( "HOME" ) ) == nullptr )
	{
		struct passwd *pw = getpwuid( getuid() );
		home              = pw->pw_dir;
	}
	snprintf( dst, dstSize, "%s/.config/%s", home, appName );
#else
	char home[ MAX_PATH ];
	if ( SUCCEEDED( SHGetFolderPath( nullptr, CSIDL_APPDATA, nullptr, 0, home ) ) )
	{
		snprintf( dst, dstSize, "%s/%s", home, appName );
		return dst;
	}
	snprintf( home, sizeof( home ), "." );
#endif

	return dst;
}

bool PlPathEndsInSlash( const char *p )
{
	size_t l = strlen( p );
	return l > 0 && ( p[ l - 1 ] == '/' || p[ l - 1 ] == '\\' );
}

typedef struct FSScanInstance
{
	char                   path[ PL_SYSTEM_MAX_PATH ];
	struct FSScanInstance *next;
} FSScanInstance;

static void scan_local_directory( const QmFsMount *mount, FSScanInstance **fileList, const char *path,
                                  const char *extension, void ( *Function )( const char *, void * ), bool recursive, void *userData )
{
#if !defined( _MSC_VER )
	DIR *directory = opendir( path );
	if ( directory )
	{
		struct dirent *entry;
		while ( ( entry = readdir( directory ) ) )
		{
			if ( strcmp( entry->d_name, "." ) == 0 || strcmp( entry->d_name, ".." ) == 0 )
			{
				continue;
			}

			char filestring[ PL_SYSTEM_MAX_PATH + 1 ];
			snprintf( filestring, sizeof( filestring ), PlPathEndsInSlash( path ) ? "%s%s" : "%s/%s", path, entry->d_name );

			struct stat st;
			if ( stat( filestring, &st ) == 0 )
			{
				if ( S_ISREG( st.st_mode ) )
				{
					// We used to just compare against the end of the name relative to '.',
					// but an extension could be made up of multiple parts (.world.n),
					// so we'll do this instead
					bool match = false;
					if ( extension != nullptr )
					{
						size_t el = strlen( extension );
						size_t fl = strlen( entry->d_name );
						match     = ( el < fl ) && ( pl_strncasecmp( &entry->d_name[ fl - el ], extension, el ) == 0 );
					}

					if ( extension == nullptr || match )
					{
						if ( mount == nullptr )
						{
							Function( filestring, userData );
							continue;
						}

						size_t      pos      = strlen( mount->path ) + 1;
						const char *filePath = &filestring[ pos ];

						// Ensure it's not already in the list
						FSScanInstance *cur = *fileList;
						while ( cur != nullptr )
						{
							if ( strcmp( filePath, cur->path ) == 0 )
							{
								// File was already passed back
								break;
							}

							cur = cur->next;
						}

						// Jumped the list early, so abort here
						if ( cur != nullptr )
						{
							continue;
						}

						Function( filePath, userData );

						// Tack it onto the list
						cur = QM_OS_MEMORY_NEW( FSScanInstance );
						strncpy( cur->path, filePath, sizeof( cur->path ) );
						cur->next = *fileList;
						*fileList = cur;
					}
				}
				else if ( S_ISDIR( st.st_mode ) && recursive )
				{
					scan_local_directory( mount, fileList, filestring, extension, Function, recursive, userData );
				}
			}
		}

		closedir( directory );
	}
	else
	{
		PlReportErrorF( PL_RESULT_FILEPATH, "opendir failed: %s", GetLastError_strerror( GetLastError() ) );
	}
#else /* assumed win32 impl */
	if ( extension == nullptr )
	{
		extension = "*";
	}

	char selectorPath[ PL_SYSTEM_MAX_PATH ];
	snprintf( selectorPath, sizeof( selectorPath ), PlPathEndsInSlash( path ) ? "%s*.%s" : "%s/*.%s", path, extension );

	WIN32_FIND_DATA ffd;
	HANDLE          find = FindFirstFile( selectorPath, &ffd );
	if ( find == INVALID_HANDLE_VALUE )
	{
		return;
	}

	size_t ml;
	if ( mount != nullptr )
		ml = strlen( mount->path );

	do
	{
		snprintf( selectorPath, sizeof( selectorPath ), PlPathEndsInSlash( path ) ? "%s%s" : "%s/%s", path, ffd.cFileName );

		if ( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			if ( recursive && !( strcmp( ffd.cFileName, "." ) == 0 || strcmp( ffd.cFileName, ".." ) == 0 ) )
			{
				scan_local_directory( mount, fileList, selectorPath, extension, Function, recursive, userData );
			}
			continue;
		}

		const char *rp = selectorPath;
		if ( mount != nullptr )
		{
			rp = &selectorPath[ ml + 1 ];
		}

		Function( rp, userData );
	} while ( FindNextFile( find, &ffd ) != FALSE );

	FindClose( find );
#endif
}

/**
 * Scans the given directory.
 *
 * @param path path to directory.
 * @param extension the extension to scan for (exclude '.').
 * @param Function callback function to deal with the file.
 * @param recursive if true, also scans the contents of each sub-directory.
 */
void PlScanDirectory( const char *path, const char *extension, void ( *Function )( const char *, void * ), bool recursive, void *userData )
{
	PlClearError();

	size_t hintSize = strlen( VFS_LOCAL_HINT );
	if ( strncmp( VFS_LOCAL_HINT, path, hintSize ) == 0 )
	{
		const char *c = path + hintSize;
		if ( *c == ':' ) c++;
		scan_local_directory( nullptr, nullptr, c, extension, Function, recursive, userData );
		return;
	}

	// If no mounted locations, assume local scan
	if ( mounts == nullptr )
	{
		scan_local_directory( nullptr, nullptr, path, extension, Function, recursive, userData );
		return;
	}

	PLPath normPath;
	snprintf( normPath, sizeof( normPath ), "%s", path );
	qm_fs_normalize_path( normPath, sizeof( normPath ) );

	FSScanInstance *fileList = nullptr;
	QmFsMount      *location;
	QM_OS_LINKED_LIST_ITERATE( location, mounts, i )
	{
		if ( location->type == QM_FS_MOUNT_TYPE_PACKAGE )
		{
			QmFsPackage *package = location->pkg;
			for ( unsigned int j = 0; j < package->numFiles; ++j )
			{
				//HACK: urgh, packages don't have the concept of '.' or './', so let's work around that

				const char *subPath;
				if ( *path == '.' )
				{
					subPath = *( path + 1 ) == '/' ? path + 2 : path + 1;
				}
				else
				{
					subPath = path;
				}

				size_t l = strlen( subPath );
				if ( strncmp( package->files[ j ].name, subPath, l ) != 0 )
				{
					continue;
				}

				const char *indexExtension = PlGetFileExtension( package->files[ j ].name );
				if ( indexExtension == nullptr || strcmp( indexExtension, extension ) != 0 )
				{
					continue;
				}

				Function( package->files[ j ].name, userData );
			}
		}
		else if ( location->type == QM_FS_MOUNT_TYPE_DIR )
		{
			char mounted_path[ PL_SYSTEM_MAX_PATH * 2 ];
			snprintf( mounted_path, sizeof( mounted_path ), "%s/%s", location->path, normPath );
			scan_local_directory( location, &fileList, mounted_path, extension, Function, recursive, userData );
		}
	}

	// Clean up the list
	FSScanInstance *current = fileList;
	while ( current != nullptr )
	{
		FSScanInstance *prev = current;
		current              = current->next;
		qm_os_memory_free( prev );
	}
}

const char *PlGetWorkingDirectory( void )
{
	static char out[ PL_SYSTEM_MAX_PATH ] = { '\0' };
	if ( getcwd( out, PL_SYSTEM_MAX_PATH ) == nullptr )
	{
		/* The MSDN documentation for getcwd() is gone, but it proooobably uses
		 * errno and friends.
		 */
		PlReportErrorF( PL_RESULT_SYSERR, "%s", strerror( errno ) );
		return nullptr;
	}
	qm_fs_normalize_path( out, sizeof( out ) );
	return out;
}

void PlSetWorkingDirectory( const char *path )
{
	if ( chdir( path ) != 0 )
	{
		PlReportErrorF( PL_RESULT_SYSERR, "%s", strerror( errno ) );
	}
}

const char *PlGetExecutablePath( char *out, size_t outSize )
{
	QM_OS_ZERO( out, outSize );

#if QM_OS_SYSTEM == QM_OS_SYSTEM_LINUX
	if ( readlink( "/proc/self/exe", out, outSize ) == -1 )
	{
		return nullptr;
	}
#elif QM_OS_SYSTEM == QM_OS_SYSTEM_WINDOWS
	GetModuleFileName( nullptr, out, ( DWORD ) outSize );
#else
	PlReportBasicError( PL_RESULT_UNSUPPORTED );
#endif

	qm_fs_normalize_path( out, outSize );

	return out;
}

/**
 * Returns the directory the current executable is located within.
 */
const char *PlGetExecutableDirectory( char *out, size_t outSize )
{
	if ( PlGetExecutablePath( out, outSize ) == nullptr )
	{
		return nullptr;
	}

	char *c = strrchr( out, '/' );
	if ( c == nullptr )
	{
		PlReportErrorF( PL_RESULT_FILEPATH, "couldn't find path seperator" );
		return nullptr;
	}

	unsigned int i = c - out + 1;
	if ( i < outSize )
	{
		out[ i ] = '\0';
	}

	qm_fs_normalize_path( out, outSize );

	return out;
}

/////////////////////////////////////////////////////////////////////////////////////
// FILE I/O

/**
 * Transform the given path to the direct path
 * relative to anything mounted under the VFS.
 */
static QmFsMount *PlGetMountLocationForPath_( const char *path )
{
	if ( PlLocalPathExists( path ) || qm_fs_check_local_file_exists( path ) )
	{
		return nullptr;
	}

#if 0
	if ( strncmp( VFS_LOCAL_HINT, path, sizeof( VFS_LOCAL_HINT ) ) == 0 ) {
		return nullptr;
	}
#endif

	size_t sl = strlen( path );
	if ( mounts != nullptr )
	{
		QmFsMount *location;
		QM_OS_LINKED_LIST_ITERATE( location, mounts, i )
		{
			switch ( location->type )
			{
				default:
				{
					/* todo: don't allow path to search outside of mounted path */
					char buf[ VFS_MAX_PATH ];
					snprintf( buf, sizeof( buf ), "%s/%s", location->path, path );
					if ( PlLocalPathExists( buf ) || qm_fs_check_local_file_exists( buf ) )
					{
						return location;
					}
					break;
				}
				case QM_FS_MOUNT_TYPE_PACKAGE:
				{
					for ( unsigned int i = 0; i < location->pkg->numFiles; ++i )
					{
						/* packages don't necessarily have the concept of a directory
						 * the way we might expect (take Unreal packages for example),
						 * so down the line we might want to allow this to be handled
						 * by the specific package API (i.e. call to GetPackageDirectory?). */
						if ( strncmp( path, location->pkg->files[ i ].name, sl ) == 0 )
						{
							return location;
						}
					}
					break;
				}
			}
		}
	}

	return nullptr;
}

static const char *PlVirtualToLocalPath_( QmFsMount *mount, const char *path, char *dest, size_t size )
{
	if ( mount == nullptr || mount->type == QM_FS_MOUNT_TYPE_PACKAGE )
	{
		snprintf( dest, size, "%s", path );
	}
	else
	{
		const char *fmt;
		if ( *path == '\\' || *path == '/' )
			fmt = "%s%s";
		else
			fmt = "%s/%s";

		snprintf( dest, size, fmt, mount->path, path );
	}

	return dest;
}

/**
 * Transform the given path to the direct path
 * relative to anything mounted under the VFS.
 */
const char *qm_fs_resolve_virtual_path( const char *path, char *dest, size_t size )
{
	if ( strncmp( VFS_LOCAL_HINT, path, sizeof( VFS_LOCAL_HINT ) ) == 0 )
	{
		path += sizeof( VFS_LOCAL_HINT );
		return path;
	}

	QmFsMount *mount = PlGetMountLocationForPath_( path );
	if ( mount == nullptr || mount->type == QM_FS_MOUNT_TYPE_PACKAGE )
	{
		return path;
	}

	const char *fmt;
	if ( *path == '\\' || *path == '/' )
		fmt = "%s%s";
	else
		fmt = "%s/%s";

	snprintf( dest, size, fmt, mount->path, path );
	return dest;
}

bool qm_fs_check_local_file_exists( const char *path )
{
	struct stat buffer;
	return ( bool ) ( stat( path, &buffer ) == 0 );
}

bool qm_fs_check_file_exists( const char *path )
{
	QmFsFile *file = qm_fs_file_open( path, false );
	if ( file == nullptr )
	{
		return false;
	}

	PlCloseFile( file );
	return true;
}

bool PlLocalPathExists( const char *path )
{
#if defined( _MSC_VER )
	errno_t err = _access_s( path, 0 );
	if ( err != 0 )
		return false;

	struct _stat s;
	_stat( path, &s );
	return ( s.st_mode & S_IFDIR );
#else
	DIR *dir = opendir( path );
	if ( dir )
	{
		closedir( dir );
		return true;
	}

	return false;
#endif
}

bool PlPathExists( const char *path )
{
	QmFsMount *mount = PlGetMountLocationForPath_( path );
	if ( mount == nullptr )
		return PlLocalPathExists( path );

	return true;
}

QmFsPathType qm_fs_get_path_type( const char *path )
{
	struct stat buf;
	if ( stat( path, &buf ) != 0 )
	{
		PlReportErrorF( PL_RESULT_FILEPATH, "failed to stat (%s) (%u)", path, errno );
		return QM_FS_PATH_TYPE_INVALID;
	}

	if ( ( buf.st_mode & S_IFMT ) == S_IFDIR )
	{
		return QM_FS_PATH_TYPE_DIR;
	}
	if ( ( buf.st_mode & S_IFMT ) == S_IFREG )
	{
		return QM_FS_PATH_TYPE_FILE;
	}

	// it's not a file or a dir *shrug*
	return QM_FS_PATH_TYPE_INVALID;
}

bool qm_fs_delete_file( const char *path )
{
	if ( !qm_fs_check_local_file_exists( path ) )
	{
		return true;
	}

	int result = remove( path );
	if ( result == 0 )
	{
		return true;
	}

	PlReportErrorF( PL_RESULT_FILEREAD, strerror( errno ) );
	return false;
}

/**
 * Write the data to the specified location. Not VFS compatible.
 * @param path Path to the destination.
 * @param buf Data buffer for whatever you're writing.
 * @param length Length of the data buffer.
 * @return True on success and false on fail.
 */
bool PlWriteFile( const char *path, const void *buf, size_t length )
{
	FILE *fp = fopen( path, "wb" );
	if ( fp == nullptr )
	{
		PlReportErrorF( PL_RESULT_FILEREAD, "failed to open %s", path );
		return false;
	}

	bool result = true;
	if ( fwrite( buf, sizeof( char ), length, fp ) != length )
	{
		PlReportErrorF( PL_RESULT_FILEWRITE, "failed to write entirety of file" );
		result = false;
	}

	qm_fs_fclose( &fp );

	return result;
}

bool qm_fs_copy_file( const char *path, const char *dest )
{
	// read in the original
	QmFsFile *original = qm_fs_file_open( path, true );
	if ( original == nullptr )
	{
		PlReportErrorF( PL_RESULT_FILEREAD, "failed to open %s", path );
		return false;
	}

	// write out the copy
	FILE *copy = fopen( dest, "wb" );
	if ( copy == nullptr )
	{
		PlReportErrorF( PL_RESULT_FILEWRITE, "failed to open %s for write", dest );
		goto BAIL;
	}

	if ( fwrite( original->data, 1, original->size, copy ) != original->size )
	{
		PlReportErrorF( PL_RESULT_FILEWRITE, "failed to write out %d bytes for %s", original->size, path );
		goto BAIL;
	}

	qm_fs_fclose( &copy );
	PlCloseFile( original );
	return true;

BAIL:
	qm_fs_fclose( &copy );
	PlCloseFile( original );
	return false;
}

size_t qm_fs_get_local_file_size( const char *path )
{
	struct stat buf;
	if ( stat( path, &buf ) != 0 )
	{
		PlReportErrorF( PL_RESULT_FILEERR, "failed to stat %s: %s", path, strerror( errno ) );
		return 0;
	}

	return ( size_t ) buf.st_size;
}

///////////////////////////////////////////

/**
 * Creates a virtual file handle from the given buffer.
 * If 'isOwner' is true, the file handle takes ownership of the buffer and frees it.
 */
QmFsFile *qm_fs_file_from_memory( const char *path, void *buf, size_t bufSize, QmFsFileOwnershipType bufType )
{
	QmFsFile *file = QM_OS_MEMORY_NEW( QmFsFile );
	if ( file == nullptr )
	{
		return nullptr;
	}

	if ( bufType == QM_FS_FILE_OWNERSHIP_TYPE_COPY )
	{
		file->data = QM_OS_MEMORY_MALLOC_( bufSize );
		memcpy( file->data, buf, bufSize );
	}
	else
	{
		if ( bufType == QM_FS_FILE_OWNERSHIP_TYPE_UNMANAGED )
		{
			file->isUnmanaged = true;
		}

		file->data = buf;
	}

	file->pos  = file->data;
	file->size = bufSize;
	file->path = qm_os_string_alloc( "%s", path );

	return file;
}

QmFsFile *qm_fs_file_from_stdio( FILE *stdio, const char *source )
{
	PLFileOffset size   = 0;
	PLFileOffset offset = qm_fs_ftell( stdio );
	if ( qm_fs_fseek( stdio, 0, SEEK_END ) == 0 )
	{
		size = qm_fs_ftell( stdio );
	}
	qm_fs_fseek( stdio, offset, SEEK_SET );

	if ( size == 0 )
	{
		PlReportErrorF( PL_RESULT_FILEREAD, "failed to determine file size" );
		return nullptr;
	}

	QmFsFile *file = QM_OS_MEMORY_NEW( QmFsFile );
	if ( file == nullptr )
	{
		return nullptr;
	}

	file->size = size;
	file->fptr = stdio;

	if ( source != nullptr )
	{
		file->path = qm_os_string_alloc( "%s", source );
	}

	return file;
}

QmFsFile *qm_fs_file_open_local( const char *path, bool cache )
{
	FILE *sysFile = fopen( path, "rb" );
	if ( sysFile == nullptr )
	{
		PlReportErrorF( PL_RESULT_FILEREAD, strerror( errno ) );
		return nullptr;
	}

	QmFsFile *file = QM_OS_MEMORY_NEW( QmFsFile );

	file->path = qm_os_string_alloc( "%s", path );
	file->size = qm_fs_get_local_file_size( path );

	if ( cache )
	{
		file->data = QM_OS_MEMORY_NEW_( uint8_t, file->size );
		file->pos  = file->data;
		size_t r   = fread( file->data, sizeof( uint8_t ), file->size, sysFile );

		qm_fs_fclose( &sysFile );

		if ( r != file->size )
		{
			qm_os_memory_free( file->data );
			qm_os_memory_free( file );
			PlReportErrorF( PL_RESULT_FILEREAD, "failed to read complete file (%lu != %lu)", r, file->size );
			return nullptr;
		}
	}
	else
	{
		file->fptr = sysFile;
	}

	file->timeStamp = qm_fs_get_local_file_timestamp( path );

	return file;
}

QmFsFile *qm_fs_file_open( const char *path, bool cache )
{
	const char *p = PlGetPathForAlias( path );
	if ( p != nullptr )
	{
		path = p;
	}

	QmFsMount *mount = PlGetMountLocationForPath_( path );
	if ( mount != nullptr && mount->type == QM_FS_MOUNT_TYPE_PACKAGE )
	{
		return PlLoadPackageFile( mount->pkg, path );
	}

	char buf[ PL_SYSTEM_MAX_PATH ];
	PlVirtualToLocalPath_( mount, path, buf, sizeof( buf ) );

	return qm_fs_file_open_local( buf, cache );
}

void PlCloseFile( QmFsFile *ptr )
{
	if ( ptr == nullptr )
	{
		return;
	}

	qm_os_memory_free( ptr->path );

	if ( !ptr->isUnmanaged )
	{
		qm_os_memory_free( ptr->data );
	}

	qm_fs_fclose( &ptr->fptr );

	qm_os_memory_free( ptr );
}

const char *qm_fs_file_get_path( const QmFsFile *self )
{
	return self->path;
}

const void *qm_fs_file_get_data( const QmFsFile *self )
{
	return self->data;
}

size_t qm_fs_file_get_size( const QmFsFile *ptr )
{
	return ptr->size;
}

int64_t qm_fs_file_get_offset( const QmFsFile *self )
{
	if ( self->fptr != nullptr )
	{
		return qm_fs_ftell( self->fptr );
	}

	return ( char * ) self->pos - ( char * ) self->data;
}

size_t qm_file_read( QmFsFile *ptr, void *dest, size_t size, size_t count )
{
	// bail early if size is 0 to avoid division by 0
	if ( size == 0 )
	{
		PlReportBasicError( PL_RESULT_FILESIZE );
		return 0;
	}

	if ( ptr->fptr != nullptr )
	{
		size_t r = fread( dest, size, count, ptr->fptr );
		if ( r != count )
		{
			PlReportErrorF( PL_RESULT_FILEREAD, "read failed on %u (%u read)", count, r );
		}
		return r;
	}

	// ensure that the read is valid
	size_t       length = size * count;
	PLFileOffset posn   = qm_fs_file_get_offset( ptr );
	if ( posn + length >= ptr->size )
	{
		// out of bounds, truncate it
		length = ptr->size - posn;
	}

	memcpy( dest, ptr->pos, length );
	ptr->pos = ( void * ) ( ( char * ) ptr->pos + length );
	return length / size;
}

int8_t qm_fs_file_read_int8( QmFsFile *self, bool *status )
{
	if ( ( size_t ) qm_fs_file_get_offset( self ) >= self->size )
	{
		if ( status != nullptr )
		{
			*status = false;
		}
		return 0;
	}

	if ( status != nullptr )
	{
		*status = true;
	}

	if ( self->fptr != nullptr )
	{
		return ( int8_t ) fgetc( self->fptr );
	}

	int8_t value = *( int8_t * ) self->pos;
	self->pos    = ( void * ) ( ( char * ) self->pos + 1 );
	return value;
}

static int64_t file_read_sized_int( QmFsFile *ptr, size_t size, bool big_endian, bool *status )
{
	int64_t n;
	if ( qm_file_read( ptr, &n, size, 1 ) != 1 )
	{
		if ( status != nullptr )
		{
			*status = false;
		}
		return 0;
	}

	if ( status != nullptr )
	{
		*status = true;
	}

	if ( big_endian )
	{
		if ( size == sizeof( int16_t ) )
		{
			return be16toh( ( int16_t ) n );
		}

		if ( size == sizeof( int32_t ) )
		{
			return be32toh( ( int32_t ) n );
		}

		return be64toh( n );
	}

	return n;
}

int16_t qm_fs_file_read_int16( QmFsFile *self, bool big_endian, bool *status )
{
	return ( int16_t ) file_read_sized_int( self, sizeof( int16_t ), big_endian, status );
}

int32_t qm_fs_file_read_int32( QmFsFile *self, bool big_endian, bool *status )
{
	return ( int32_t ) file_read_sized_int( self, sizeof( int32_t ), big_endian, status );
}

int64_t qm_fs_file_read_int64( QmFsFile *self, bool big_endian, bool *status )
{
	return file_read_sized_int( self, sizeof( int64_t ), big_endian, status );
}

float qm_fs_file_read_float( QmFsFile *ptr, bool big_endian, bool *status )
{
	float f;
	if ( qm_file_read( ptr, &f, sizeof( float ), 1 ) != 1 )
	{
		if ( status != nullptr ) *status = false;
		return 0.0f;
	}

	// not entirely sure how well this'd work, if at all
	if ( big_endian )
	{
		f = ( float ) be32toh( ( uint32_t ) f );
	}

	if ( status != nullptr ) *status = true;

	return f;
}

double qm_fs_file_read_double( QmFsFile *ptr, bool big_endian, bool *status )
{
	double d;
	if ( qm_file_read( ptr, &d, sizeof( double ), 1 ) != 1 )
	{
		if ( status != nullptr ) *status = false;
		return 0.0;
	}

	// not entirely sure how well this'd work, if at all
	if ( big_endian )
	{
		d = ( double ) be64toh( ( uint64_t ) d );
	}

	if ( status != nullptr ) *status = true;

	return d;
}

char *qm_fs_file_read_string( QmFsFile *ptr, char *str, size_t size )
{
	if ( size == 0 )
	{
		PlReportBasicError( PL_RESULT_INVALID_PARM3 );
		return nullptr;
	}

	char *result;
	if ( ptr->fptr != nullptr )
	{
		result = fgets( str, ( int ) size, ptr->fptr );
	}
	else
	{
		if ( ( char * ) ptr->pos >= ( char * ) ptr->data + ptr->size )
		{
			PlReportBasicError( PL_RESULT_FILEREAD );
			return nullptr;
		}

		char *nl = memchr( ptr->pos, '\n', ptr->size - ( ( char * ) ptr->pos - ( char * ) ptr->data ) );
		if ( nl == nullptr )
		{
			nl = ( char * ) ptr->data + ptr->size - 1;
		}

		size_t bytesToCopy = nl - ( char * ) ptr->pos + 1;
		if ( bytesToCopy >= size )
		{
			bytesToCopy = size - 1;
			nl          = ( char * ) ptr->pos + bytesToCopy - 1;
		}

		memcpy( str, ptr->pos, bytesToCopy );
		str[ bytesToCopy ] = '\0';
		ptr->pos           = ( uint8_t * ) ( nl + 1 );
		result             = str;
	}

	return result;
}

bool qm_fs_file_seek( QmFsFile *ptr, PLFileOffset pos, QmFsSeek seek )
{
	if ( ptr->fptr != nullptr )
	{
		int err = qm_fs_fseek( ptr->fptr, pos, seek );
		if ( err != 0 )
		{
			PlReportErrorF( PL_RESULT_FILEREAD, "failed to seek file (%s)", GetLastError_strerror( GetLastError() ) );
			return false;
		}

		return true;
	}

	PLFileOffset posn = qm_fs_file_get_offset( ptr );
	switch ( seek )
	{
		case QM_FS_SEEK_CUR:
			if ( ( size_t ) ( posn + pos ) > ptr->size || pos < -( ( signed long ) posn ) )
			{
				PlReportBasicError( PL_RESULT_INVALID_PARM2 );
				return false;
			}
			ptr->pos = ( ( char * ) ptr->pos + pos );
			break;

		case QM_FS_SEEK_SET:
			if ( pos > ( signed long ) ptr->size || pos < 0 )
			{
				PlReportBasicError( PL_RESULT_INVALID_PARM2 );
				return false;
			}
			ptr->pos = ( ( char * ) ptr->data + pos );
			break;

		case QM_FS_SEEK_END:
			if ( pos <= -( ( signed long ) ptr->size ) )
			{
				PlReportBasicError( PL_RESULT_INVALID_PARM2 );
				return false;
			}
			ptr->pos = ( ( char * ) ptr->data + ( ptr->size - pos ) );
			break;

		default:
			PlReportBasicError( PL_RESULT_INVALID_PARM3 );
			return false;
	}

	return true;
}

void qm_fs_file_rewind( QmFsFile *ptr )
{
	if ( ptr->fptr != nullptr )
	{
		rewind( ptr->fptr );
		return;
	}

	ptr->pos = ptr->data;
}

/**
 * If the file is being streamed from disk, cache
 * it into memory. This will also attempt to retain
 * the original offset into the file.
 * Returns pointer to position relative to read
 * location, or null on fail.
 */
const void *PlCacheFile( QmFsFile *file )
{
	/* make sure it's not already cached */
	if ( file->fptr == nullptr )
	{
		return nullptr;
	}

	PLFileOffset p = qm_fs_file_get_offset( file );
	size_t       s = qm_fs_file_get_size( file );

	/* jump back to the start */
	qm_fs_file_rewind( file );

	/* allocate the new buffer and attempt to read in the whole thing */
	file->data = QM_OS_MEMORY_MALLOC_( s );
	if ( qm_file_read( file, file->data, sizeof( char ), s ) != s )
	{
		/* seek back and restore where we were */
		qm_fs_file_seek( file, ( long ) p, QM_FS_SEEK_SET );
		qm_os_memory_free( file->data );
		return nullptr;
	}

	/* close the original file handle we had */
	qm_fs_fclose( &file->fptr );

	/* match pos with where we originally were, so it's like nothing changed */
	file->pos = ( char * ) file->data + p;

	return file->pos;
}

/****************************************
 * File Aliases
 * This system allows you to register an alias that can be caught to load a
 * different file in it's place. Useful when dealing with older titles that
 * didn't utilise full paths and just single file names instead (i.e. mapping
 * between a WAD and local file).
 ****************************************/

#define MAX_ALIASES 4096

typedef struct FileAlias
{
	PLPath   alias;
	PLPath   target;
	uint32_t hash;
} FileAlias;
static FileAlias    fileAliases[ MAX_ALIASES ];
static unsigned int numFileAliases = 0;

void PlClearFileAliases( void )
{
	numFileAliases = 0;
}

/**
 * Adds a new alias to the list.
 */
void PlAddFileAlias( const char *alias, const char *target )
{
	if ( numFileAliases >= MAX_ALIASES )
	{
		PlReportBasicError( PL_RESULT_MEMORY_EOA );
		return;
	}

	const char *p = PlGetPathForAlias( alias );
	if ( p != nullptr )
	{
		PlReportErrorF( PL_RESULT_INVALID_PARM1, "duplicate alias" );
		return;
	}

	snprintf( fileAliases[ numFileAliases ].alias, sizeof( PLPath ), "%s", alias );
	fileAliases[ numFileAliases ].hash = PlGenerateHashSDBM( fileAliases[ numFileAliases ].alias );
	snprintf( fileAliases[ numFileAliases ].target, sizeof( PLPath ), "%s", target );
	numFileAliases++;
}

const char *PlGetPathForAlias( const char *alias )
{
	if ( numFileAliases == 0 )
	{
		return nullptr;
	}

	uint32_t hash = PlGenerateHashSDBM( alias );
	for ( unsigned int i = 0; i < numFileAliases; ++i )
	{
		if ( hash != fileAliases[ i ].hash )
		{
			continue;
		}

		return fileAliases[ i ].target;
	}

	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////
// New API
// TODO: move these under an entirely new module...
/////////////////////////////////////////////////////////////////////////////////////

char *hei_fs_get_temp_path( char *dst, size_t dstSize )
{
#if ( PL_SYSTEM_OS == PL_SYSTEM_OS_LINUX )

	char dir[] = "/tmp/heiXXXXXX";
	if ( mkdtemp( dir ) == nullptr )
	{
		PlReportErrorF( PL_RESULT_FILETYPE, "failed to set temporary write location" );
		return nullptr;
	}

	snprintf( dst, dstSize, "%s", dir );

#elif ( PL_SYSTEM_OS == PL_SYSTEM_OS_WINDOWS )

	//TODO: this is untested...

	char dir[ 128 ];
	GetTempPath( sizeof( dir ), dir );
	if ( !PlCreateDirectory( dir ) )
	{
		return nullptr;
	}

	snprintf( dst, dstSize, "%s", dir );

#endif

	return dst;
}
