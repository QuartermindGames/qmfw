// SPDX-License-Identifier: MIT
// Hei Platform Library
// Copyright © 2017-2023 Mark E Sowden <hogsy@oldtimes-software.com>

#include "package_private.h"

// Package format as used by FreshEngine
/*	- Little endian
 *	- Fixed-length filenames (12 bytes)
 */

QmFsPackage *PlParseFreshBinPackage_( QmFsFile *file ) {
	// First check the funky identifier at the start
	char ident[ 8 ];
	qm_file_read( file, ident, sizeof( char ), sizeof( ident ) );
	if ( strncmp( ident, "DATA    ", 8 ) != 0 ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "not a freshengine bin package" );
		return NULL;
	}

	// First two bytes seem to just be zero, so we'll assume that's normal
	if ( qm_fs_file_read_int32( file, false, NULL ) != 0 || qm_fs_file_read_int32( file, false, NULL ) != 0 ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "invalid freshengine bin package" );
		return NULL;
	}

	unsigned int numFiles = QM_FS_FILE_READ_UINT32( file, false, NULL );
	if ( numFiles == 0 ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "empty freshengine bin package" );
		return NULL;
	}

	QmFsPackage *package = PlCreatePackageHandle( qm_fs_file_get_path( file ), numFiles, NULL );
	if ( package == NULL ) {
		return NULL;
	}

	for ( unsigned int i = 0; i < numFiles; ++i ) {
		char filename[ 12 ];
		qm_file_read( file, filename, sizeof( char ), sizeof( filename ) );
		// For some reason, so null-terminator but spaces instead??
		// Let's convert it...
		for ( unsigned int j = 0; j < sizeof( filename ); ++j ) {
			if ( filename[ j ] != ' ' )
				continue;

			filename[ j ] = '\0';
			break;
		}

		strncpy( package->files[ i ].name, filename, 12 );

		unsigned int blockOffs = QM_FS_FILE_READ_UINT32( file, false, NULL );
		package->files[ i ].offset = 2048 * blockOffs;
		unsigned int size = QM_FS_FILE_READ_UINT32( file, false, NULL );
		package->files[ i ].size = size;
	}

	return package;
}
