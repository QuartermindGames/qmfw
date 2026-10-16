// SPDX-License-Identifier: MIT
// Copyright © 2017-2023 Mark E Sowden <hogsy@oldtimes-software.com>

#include "package_private.h"

#include "qmos/public/qm_os_memory.h"
#include "qmos/public/qm_os_string.h"

#define DFS_MAGIC   QM_OS_MAGIC_TO_NUM( 'S', 'F', 'D', 'X' )
#define DFS_VERSION 1// Area 51 is 3 - will see about adding support later

/**
 * We actually need to load the data from a different file
 */
static void *LoadPackageFile( QmFsFile *file, QmFsPackageFile *index ) {
	const char *dfsPath = qm_fs_file_get_path( file );
	size_t dfsPathSize = strlen( dfsPath );

	PLPath dataPath = {};
	strncpy( dataPath, dfsPath, dfsPathSize - 3 );
	strcat( dataPath, "000" );

	void *data = NULL;
	QmFsFile *dataFile = qm_fs_file_open( dataPath, false );
	if ( qm_fs_file_seek( dataFile, ( signed ) index->offset, QM_FS_SEEK_SET ) ) {
		data = QM_OS_MEMORY_NEW_( uint8_t, index->size );
		if ( qm_file_read( dataFile, data, sizeof( char ), index->size ) != index->size ) {
			qm_os_memory_free( data );
			data = NULL;
		}
	}

	PlCloseFile( dataFile );

	return data;
}

static int ValidateDFSFile( QmFsFile *file ) {
	int magic = qm_fs_file_read_int32( file, false, NULL );
	if ( magic != DFS_MAGIC ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "invalid magic" );
		return -1;
	}

	int version = qm_fs_file_read_int32( file, false, NULL );
	if ( version <= 0 || version > DFS_VERSION ) {
		PlReportErrorF( PL_RESULT_FILEVERSION, "invalid version" );
		return -1;
	}

	return version;
}

static char *ReadString( QmFsFile *file, char **dst, uint32_t offset ) {
	PLFileOffset store = qm_fs_file_get_offset( file );
	qm_fs_file_seek( file, offset, QM_FS_SEEK_SET );
	for ( ;; ) {
		bool status;
		char c = ( char ) qm_fs_file_read_int8( file, &status );
		if ( !status || c == '\0' ) {
			break;
		}
		if ( c == '\\' ) {
			c = '/';
		}
		*( *dst )++ = c;
	}
	qm_fs_file_seek( file, store, QM_FS_SEEK_SET );
	return *dst;
}

QmFsPackage *PlParseDfsPackage_( QmFsFile *file ) {
	size_t fileSize = qm_fs_file_get_size( file );
	if ( fileSize == 0 ) {
		PlReportErrorF( PL_RESULT_FILESIZE, "empty file" );
		return NULL;
	}

	int version = ValidateDFSFile( file );
	if ( version == -1 ) {
		return NULL;
	}

	qm_fs_file_read_int32( file, false, NULL );// unknown
	qm_fs_file_read_int32( file, false, NULL );// unknown

	uint32_t numFiles = QM_FS_FILE_READ_UINT32( file, false, NULL );
	if ( numFiles <= 0 ) {
		PlReportErrorF( PL_RESULT_FILEERR, "invalid number of files" );
		return NULL;
	}

	qm_fs_file_read_int32( file, false, NULL );// unknown

	uint32_t stringTableSize = QM_FS_FILE_READ_UINT32( file, false, NULL );
	if ( stringTableSize <= 0 || stringTableSize >= fileSize ) {
		PlReportErrorF( PL_RESULT_FILEERR, "invalid string table size" );
		return NULL;
	}

	qm_fs_file_read_int32( file, false, NULL );// unknown
	qm_fs_file_read_int32( file, false, NULL );// unknown

	uint32_t stringTableOffset = QM_FS_FILE_READ_UINT32( file, false, NULL );
	if ( stringTableOffset <= 0 || stringTableOffset >= fileSize ) {
		PlReportErrorF( PL_RESULT_FILEERR, "invalid string table offset" );
		return NULL;
	}

	qm_fs_file_read_int32( file, false, NULL );// unknown

	QmFsPackage *package = PlCreatePackageHandle( qm_fs_file_get_path( file ), numFiles, LoadPackageFile );

	for ( uint32_t i = 0; i < numFiles; ++i ) {
		// urgh...
		uint32_t nameOffset = QM_FS_FILE_READ_UINT32( file, false, NULL );
		uint32_t nameExtendedOffset = QM_FS_FILE_READ_UINT32( file, false, NULL );
		uint32_t dirOffset = QM_FS_FILE_READ_UINT32( file, false, NULL );
		uint32_t extensionOffset = QM_FS_FILE_READ_UINT32( file, false, NULL );

		char *c = package->files[ i ].name;
		ReadString( file, &c, stringTableOffset + dirOffset );
		ReadString( file, &c, stringTableOffset + nameOffset );
		ReadString( file, &c, stringTableOffset + nameExtendedOffset );
		ReadString( file, &c, stringTableOffset + extensionOffset );
		qm_os_string_to_lower( package->files[ i ].name, sizeof( package->files[ i ].name ) );

		package->files[ i ].offset = QM_FS_FILE_READ_UINT32( file, false, NULL );
		package->files[ i ].size = QM_FS_FILE_READ_UINT32( file, false, NULL );
	}

	return package;
}
