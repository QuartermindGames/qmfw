// SPDX-License-Identifier: MIT
// Hei Platform Library
// Copyright © 2017-2023 Mark E Sowden <hogsy@oldtimes-software.com>

#include "package_private.h"

// GRP package format used by Duke Nukem 3D

QmFsPackage *PlParseGrpPackage_( QmFsFile *file ) {
	char buf[ 12 ];
	qm_file_read( file, buf, sizeof( char ), sizeof( buf ) );
	if ( strncmp( buf, "KenSilverman", sizeof( buf ) ) != 0 ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "not a grp package" );
		return NULL;
	}

	unsigned int numFiles = QM_FS_FILE_READ_UINT32( file, false, NULL );
	if ( numFiles == 0 ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "empty grp package" );
		return NULL;
	}

	QmFsPackage *package = PlCreatePackageHandle( qm_fs_file_get_path( file ), numFiles, NULL );
	if ( package == NULL ) {
		return NULL;
	}

	unsigned int baseOffset = qm_fs_file_get_offset( file ) + ( numFiles * 16 );
	for ( unsigned int i = 0; i < numFiles; ++i ) {
		qm_file_read( file, package->files[ i ].name, sizeof( char ), 12 );
		package->files[ i ].size = QM_FS_FILE_READ_UINT32( file, false, NULL );
		package->files[ i ].offset = baseOffset;
		baseOffset += package->files[ i ].size;
	}

	return package;
}
