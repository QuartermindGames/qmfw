// SPDX-License-Identifier: MIT
// Hei Platform Library
// Copyright © 2017-2024 Mark E Sowden <hogsy@oldtimes-software.com>

#include "package_private.h"

QmFsPackage *PlParseAllPackage_( QmFsFile *file ) {
	uint32_t numFiles = QM_FS_FILE_READ_UINT32( file, false, NULL );
	if ( numFiles == 0 ) {
		PlReportErrorF( PL_RESULT_FILEERR, "invalid number of files in package" );
		return NULL;
	}

	// try to validate...
	for ( unsigned int i = 0; i < 3; ++i ) {
		bool status;
		if ( QM_FS_FILE_READ_UINT32( file, false, &status ) == 0 && status ) {
			continue;
		}

		if ( !status ) {
			return NULL;
		}

		PlReportErrorF( PL_RESULT_FILEERR, "invalid header for package" );
		return NULL;
	}

	QmFsPackage *package = PlCreatePackageHandle( qm_fs_file_get_path( file ), numFiles, NULL );
	if ( package == NULL ) {
		return NULL;
	}

	// this seems to be good enough, but not entirely correct?
	static const unsigned int ENTRY_SIZE = 72;
	unsigned int offset = qm_fs_file_get_offset( file ) + ( ENTRY_SIZE * numFiles );
	for ( unsigned int i = 0; i < numFiles; offset += package->files[ i ].size, ++i ) {
		qm_file_read( file, package->files[ i ].name, sizeof( char ), 64 );
		package->files[ i ].size = QM_FS_FILE_READ_UINT32( file, false, NULL );
		package->files[ i ].offset = offset;
		qm_fs_file_read_int32( file, false, NULL );
	}

	return package;
}
