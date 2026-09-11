/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

#include <plcore/pl.h>

#define PlKilobytesToBytes( a ) ( ( double ) ( ( a ) ) * 1000 )
#define PlMegabytesToBytes( a ) ( PlKilobytesToBytes( a ) * 1000 )
#define PlGigabytesToBytes( a ) ( PlMegabytesToBytes( a ) * 1000 )

#define PlBytesToKilobytes( a ) ( ( double ) ( ( a ) ) / 1000 )
#define PlBytesToMegabytes( a ) ( PlBytesToKilobytes( a ) / 1000 )
#define PlBytesToGigabytes( a ) ( PlBytesToMegabytes( a ) / 1000 )

#define PlBytesToKibibytes( a ) ( ( double ) ( ( a ) ) / 1024 )
#define PlBytesToMebibytes( a ) ( PlBytesToKibibytes( a ) / 1024 )
#define PlBytesToGibibytes( a ) ( PlBytesToMebibytes( a ) / 1024 )

typedef struct QmFsFile QmFsFile;

typedef enum QmFsSeek
{
#if defined( SEEK_SET ) && defined( SEEK_CUR ) && defined( SEEK_END )
	QM_FS_SEEK_SET = SEEK_SET,// set the explicit location
	QM_FS_SEEK_CUR = SEEK_CUR,// seek from the current location
	QM_FS_SEEK_END = SEEK_END
#else
	QM_FS_SEEK_SET,
	QM_FS_SEEK_CUR,
	QM_FS_SEEK_END
#endif
} QmFsSeek;

typedef enum QmFsFileOwnershipType
{
	QM_FS_FILE_OWNERSHIP_TYPE_COPY,     // creates a copy of the given input and takes ownership
	QM_FS_FILE_OWNERSHIP_TYPE_OWNER,    // takes ownership of the input and frees on close
	QM_FS_FILE_OWNERSHIP_TYPE_UNMANAGED,// doesn't free on close
} QmFsFileOwnershipType;

typedef enum QmFsMountType
{
	QM_FS_MOUNT_TYPE_DIR,
	QM_FS_MOUNT_TYPE_PACKAGE,
} QmFsMountType;

typedef struct QmFsMount QmFsMount;

#ifdef PL_FILESYSTEM_64
typedef int64_t PLFileOffset;
#else
typedef int32_t PLFileOffset;
#endif

PL_EXTERN_C

#if !defined( PL_COMPILE_PLUGIN )

/////////////////////////////////////////////////////////////////////////////////////
// Low Level API

/**
 * Wrapper around fclose, automatically sets passed pointer to null on successful
 * close.
 *
 * @param file	File handle.
 * @return		Returns 0 if pointer is null, otherwise returns value from fclose.
 */
int qm_fs_fclose( FILE **file );

int64_t qm_fs_ftell( FILE *file );
int64_t qm_fs_fseek( FILE *file, uint64_t off, QmFsSeek wence );

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

/**
 * Constructs a formatted path string and stores it in the provided destination buffer.
 *
 * The function uses variable arguments to build a formatted string and stores the result in 'dst'.
 * If 'truncate' is set to `true`, the function will truncate the resulting string if it exceeds
 * the size of `PLPath`. Otherwise, the function will report an error if the size is exceeded.
 *
 * If the length of the constructed path is zero or the path exceeds the maximum allowed length and
 * 'truncate' is false, the function returns NULL.
 *
 * The resulting path is normalized by converting backslashes ('\') to forward slashes ('/'), and
 * ensuring that trailing slashes are removed.
 *
 * @param dst 		The destination buffer of type `PLPath` where the constructed path will be stored.
 * @param truncate 	A boolean indicating whether to truncate the resulting path if it exceeds the
 *        			size of `PLPath`.
 * @param msg 		A format string that specifies how to construct the path.
 * @param ... 		Additional arguments that will be used in the construction of the formatted string.
 * @return 			A pointer to the resulting path stored in 'dst', or NULL if an error occurs.
 */
const char *PlSetupPath( PLPath dst, bool truncate, const char *msg, ... );

/**
 * Append a source path to a destination path, with an option to truncate if necessary.
 *
 * This function appends the given source path to the destination path. If the combined
 * length of both paths exceeds the maximum length and truncation is allowed, it will truncate
 * the destination path. If truncation is not allowed and the combined length exceeds
 * the maximum length, it will report an error and return NULL.
 *
 * @param dst 		The destination path (of type PLPath).
 * @param src 		The source path to append to the destination path.
 * @param truncate 	Flag indicating whether to truncate the destination path if the combined length exceeds the limit.
 * @return 			The modified destination path, or NULL if an error occurred.
 */
const char *PlAppendPath( PLPath dst, const char *src, bool truncate );

/**
 * @brief Appends a formatted string to a destination path with optional truncation.
 *
 * This function appends a formatted string, specified by a format string and
 * optional additional arguments, to an existing destination path.
 * The resulting path is normalized, converting backslashes ('\\') to forward slashes ('/').
 *
 * @param dst 		The destination path buffer where the formatted string should be appended.
 * @param truncate 	A boolean flag to allow truncation of the resulting path if it exceeds the maximum path length.
 * @param msg 		The format string that specifies how subsequent arguments are converted for output.
 * @param ... 		Additional arguments for the format string.
 * @return 			A pointer to the destination buffer containing the combined and normalized path,
 * 					or NULL if the operation fails due to memory constraints or improper formatting.
 */
const char *PlAppendPathEx( PLPath dst, bool truncate, const char *msg, ... );

char *qm_fs_normalize_path( char *dst, size_t dstSize );

char *PlGetFolderForPath( PLPath dst, PLPath src );

/**
 * Returns the name of the systems current user.
 *
 * @param out
 */
char *PlGetUserName( char *out, size_t n );

const char *PlGetWorkingDirectory( void );
void        PlSetWorkingDirectory( const char *path );

const char *PlGetExecutablePath( char *out, size_t outSize );
const char *PlGetExecutableDirectory( char *out, size_t outSize );

/**
 * Returns directory for saving application data.
 *
 * @param appName 	Name of your application.
 * @param dst 		Buffer we'll be storing the path to.
 * @param dstSize 		Length of the buffer.
 * @return 			Pointer to the output, will return NULL on error.
 */
char *PlGetApplicationDataDirectory( const char *appName, char *dst, size_t dstSize );

/**
 * Checks if the given path ends with a slash character.
 *
 * This function determines whether the provided path string ends with either
 * a forward slash ('/') or a backslash ('\\'). It first calculates the
 * length of the string and then checks the last character.
 *
 * @param p A null-terminated string representing the path to check.
 * 			It should be a valid string.
 * @return 	A boolean value:
 *         	- true: if the path ends with a slash or backslash.
 *         	- false: otherwise.
 */
bool PlPathEndsInSlash( const char *p );

void PlStripExtension( char *dest, size_t length, const char *in );

const char *PlGetFileExtension( const char *in );

/**
 * Returns pointer to the last component in the given filename.
 *
 * @param path
 * @return
 */
const char *PlGetFileName( const char *path );

const char *qm_fs_resolve_virtual_path( const char *path, char *dest, size_t size );

bool qm_fs_check_local_file_exists( const char *path );

/**
 * Checks whether or not the given file is accessible or exists.
 *
 * @param path	Path to file we're checking for.
 * @return		False if the file wasn't accessible.
 */
bool qm_fs_check_file_exists( const char *path );

bool PlLocalPathExists( const char *path );
bool PlPathExists( const char *path );

typedef enum QmFsPathType : int8_t
{
	QM_FS_PATH_TYPE_INVALID = -1,
	QM_FS_PATH_TYPE_DIR,
	QM_FS_PATH_TYPE_FILE,
} QmFsPathType;

/**
 * Attempts to determine if the given path is a directory or file.
 * If it returns QM_FS_PATH_TYPE_INVALID, this indicates it's neither (probably invalid),
 * otherwise QM_FS_PATH_TYPE_DIR indicates a dir and QM_FS_PATH_TYPE_FILE a file.
 */
QmFsPathType qm_fs_get_path_type( const char *path );

void PlScanDirectory( const char *path, const char *extension, void ( *Function )( const char *, void * ), bool recursive, void *userData );

bool PlCreateDirectory( const char *path );
bool PlCreatePath( const char *path );

// File I/O ...

QmFsFile *qm_fs_file_from_memory( const char *path, void *buf, size_t bufSize, QmFsFileOwnershipType bufType );

/**
 * @brief Creates a PLFile structure from a standard C FILE pointer.
 *
 * This function initializes a PLFile structure, setting its file pointer to
 * the given stdio FILE pointer and determining the size of the file. If a
 * source path is provided, it will be copied into the PLFile's path attribute.
 * The function does not manage the opening or closing of the FILE pointer
 * provided.
 *
 * @param stdio 	Pointer to a standard C FILE object.
 * @param source 	Optional string representing the source path of the FILE object.
 *               	If provided, it will be stored in the PLFile's path attribute.
 * @return 			A pointer to the newly created PLFile structure.
 */
QmFsFile *qm_fs_file_from_stdio( FILE *stdio, const char *source );

QmFsFile *qm_fs_file_open_local( const char *path, bool cache );

/**
 * Opens the specified file via the VFS.
 *
 * @param path		Path to the file you want to open.
 * @param cache		Whether or not to cache the entire file into memory.
 * @return			Returns handle to the file instance.
 */
QmFsFile *qm_fs_file_open( const char *path, bool cache );

void PlCloseFile( QmFsFile *ptr );

bool qm_fs_copy_file( const char *path, const char *dest );
bool PlWriteFile( const char *path, const void *buf, size_t length );

/**
 * Deletes the specified file. Not VFS compatible.
 *
 * @param path	Path to the file you want to delete.
 * @return		True on success and false on fail.
 */
bool qm_fs_delete_file( const char *path );

bool qm_fs_file_is_end( const QmFsFile *self );

/**
 * Returns the modified time of the given file.
 *
 * @param path	Path to the file you want the timestamp for.
 * @return		Modification time in seconds. returns 0 upon fail.
 */
time_t qm_fs_get_local_file_timestamp( const char *path );

size_t qm_fs_get_local_file_size( const char *path );

/**
 * Returns the path of the current open file.
 *
 * @param self	Pointer to file handle.
 * @return		Full path to the current file.
 */
const char *qm_fs_file_get_path( const QmFsFile *self );

const void *qm_fs_file_get_data( const QmFsFile *self );
time_t      qm_fs_file_get_timestamp( QmFsFile *self );

/**
 * Returns file size in bytes.
 *
 * @param ptr Pointer to file handle.
 * @return Number of bytes within file.
 */
size_t qm_fs_file_get_size( const QmFsFile *ptr );

/**
 * Returns the current position within the file handle (ftell).
 *
 * @param self Pointer to the file handle.
 * @return Number of bytes into the file.
 */
int64_t qm_fs_file_get_offset( const QmFsFile *self );

size_t qm_file_read( QmFsFile *ptr, void *dest, size_t size, size_t count );

int8_t  qm_fs_file_read_int8( QmFsFile *self, bool *status );
int16_t qm_fs_file_read_int16( QmFsFile *self, bool big_endian, bool *status );
int32_t qm_fs_file_read_int32( QmFsFile *self, bool big_endian, bool *status );
int64_t qm_fs_file_read_int64( QmFsFile *self, bool big_endian, bool *status );

#	define PL_READUINT8( FILE, STATUS )          ( uint8_t ) qm_fs_file_read_int8( ( FILE ), ( STATUS ) )
#	define PL_READUINT16( FILE, ENDIAN, STATUS ) ( uint16_t ) qm_fs_file_read_int16( ( FILE ), ( ENDIAN ), ( STATUS ) )
#	define PL_READUINT32( FILE, ENDIAN, STATUS ) ( uint32_t ) qm_fs_file_read_int32( ( FILE ), ( ENDIAN ), ( STATUS ) )
#	define PL_READUINT64( FILE, ENDIAN, STATUS ) ( uint64_t ) qm_fs_file_read_int64( ( FILE ), ( ENDIAN ), ( STATUS ) )

float  qm_fs_file_read_float( QmFsFile *ptr, bool big_endian, bool *status );
double qm_fs_file_read_double( QmFsFile *ptr, bool big_endian, bool *status );

char *qm_fs_file_read_string( QmFsFile *ptr, char *str, size_t size );

bool qm_fs_file_seek( QmFsFile *ptr, PLFileOffset pos, QmFsSeek seek );
void qm_fs_file_rewind( QmFsFile *ptr );

const void *PlCacheFile( QmFsFile *file );

/** FS Mounting **/

QmFsMount *qm_fs_mount_local_location( const char *path );

/**
 * Mount the given location. On failure returns -1.
 */
QmFsMount *qm_fs_mount_location( const char *path );

void PlClearMountedLocation( QmFsMount *location );

/**
 * Clear all of the mounted locations
 */
void qm_fs_clear_mounted_locations();

QmFsMountType qm_fs_mount_get_type( const QmFsMount *self );
const char   *qm_fs_mount_get_path( const QmFsMount *self );

/**
 * Attempts to find a mount that matches closely to the given path.
 * This is unreliable, so probably avoid it!
 *
 * @param path	Path to match.
 * @return		Pointer to a filesystem mount instance on success, otherwise null.
 */
QmFsMount *PlGetMountLocationForPath( const char *path );

/****/

void        PlClearFileAliases( void );
void        PlAddFileAlias( const char *alias, const char *target );
const char *PlGetPathForAlias( const char *alias );

/////////////////////////////////////////////////////////////////////////////////////
// New API
// TODO: move these under an entirely new module...
/////////////////////////////////////////////////////////////////////////////////////

char *hei_fs_get_temp_path( char *dst, size_t dstSize );

#endif

PL_EXTERN_C_END
