// SPDX-License-Identifier: MIT
// Hei Platform Library
// Copyright © 2017-2024 Mark E Sowden <hogsy@oldtimes-software.com>

#include "../package_private.h"

/* Headhunter AFS package format
 *
 * There are two types of AFS packages it seems?
 * One that starts with AHV which doesn't have a TOC and may be compressed,
 * and another which has a TOC and no magic.
 *
 * Based on the packs found in the build below.
 * https://hiddenpalace.org/Headhunter_(Nov_18,_2001_prototype)
 *
 * Will check others some other time.
 */

QmFsPackage *PlParseAfsPackage_( QmFsFile *file ) {
	uint32_t numFiles = QM_FS_FILE_READ_UINT32( file, false, NULL );
	if ( numFiles == 0 ) {
		PlReportErrorF( PL_RESULT_FILEERR, "invalid number of files in package" );
		return NULL;
	}

	uint32_t version = QM_FS_FILE_READ_UINT32( file, false, NULL );
	if ( version != 2 ) {
		PlReportErrorF( PL_RESULT_FILEERR, "invalid version (%u != 2)", version );
		return NULL;
	}

#define SECTOR_SIZE 2048
	if ( !qm_fs_file_seek( file, SECTOR_SIZE, QM_FS_SEEK_SET ) ) {
		return NULL;
	}

	QmFsPackage *package = PlCreatePackageHandle( qm_fs_file_get_path( file ), numFiles, NULL );
	if ( package == NULL ) {
		return NULL;
	}

	for ( uint32_t i = 0; i < numFiles; ++i ) {
		package->files[ i ].offset = QM_FS_FILE_READ_UINT32( file, false, NULL );
		package->files[ i ].size = QM_FS_FILE_READ_UINT32( file, false, NULL );
		QM_FS_FILE_READ_UINT32( file, false, NULL );//?
		QM_FS_FILE_READ_UINT32( file, false, NULL );//?
		QM_FS_FILE_READ_UINT8( file, NULL );        // maybe this was supposed to be string length?? unused if so
		qm_file_read( file, package->files[ i ].name, sizeof( char ), 239 );
		qm_fs_normalize_path( package->files[ i ].name, sizeof( package->files[ i ].name ) );
	}

	return package;
}
