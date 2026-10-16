// SPDX-License-Identifier: MIT
// Hei Platform Library
// Copyright © 2017-2024 Mark E Sowden <hogsy@oldtimes-software.com>

#include "qmos/public/qm_os.h"

#include "plcore/pl_math.h"

#include "../package_private.h"

/* Headhunter AHF package format
 *
 * Based on the packs found in the build below.
 * https://hiddenpalace.org/Headhunter_(Nov_18,_2001_prototype)
 */

QmFsPackage *PlParseAhfPackage_( QmFsFile *file ) {
	static const uint32_t MAGIC = QM_OS_MAGIC_TO_NUM( 'A', 'H', 'F', 'F' );
	uint32_t tag = QM_FS_FILE_READ_UINT32( file, false, NULL );
	if ( tag != MAGIC ) {
		PlReportErrorF( PL_RESULT_FILEERR, "invalid tag (%u != %u)", tag, MAGIC );
		return NULL;
	}

	uint32_t version = QM_FS_FILE_READ_UINT32( file, false, NULL );
	if ( version != 2 ) {
		PlReportErrorF( PL_RESULT_FILEERR, "invalid version (%u != 2)", version );
		return NULL;
	}

	uint32_t numFiles = QM_FS_FILE_READ_UINT32( file, false, NULL );
	if ( numFiles == 0 ) {
		PlReportErrorF( PL_RESULT_FILEERR, "invalid number of files in package" );
		return NULL;
	}

	size_t totalSize = qm_fs_file_get_size( file );
	uint32_t startOffset = QM_FS_FILE_READ_UINT32( file, false, NULL );
	if ( startOffset >= totalSize ) {
		PlReportErrorF( PL_RESULT_FILEERR, "invalid file size" );
		return NULL;
	}

	QmFsPackage *package = PlCreatePackageHandle( qm_fs_file_get_path( file ), numFiles, NULL );
	for ( unsigned int i = 0; i < numFiles; ++i ) {
		package->files[ i ].offset = QM_FS_FILE_READ_UINT32( file, false, NULL );
		package->files[ i ].size = QM_FS_FILE_READ_UINT32( file, false, NULL );
		QM_FS_FILE_READ_UINT32( file, false, NULL );// maybe was meant to be string size? seems unused

		unsigned int j;
		for ( j = 0; j < ( sizeof( package->files[ i ].name ) - 1 ); ++j ) {
			package->files[ i ].name[ j ] = qm_fs_file_read_int8( file, NULL );
			if ( package->files[ i ].name[ j ] == '\0' ) {
				break;
			}
		}

		PLFileOffset padding = PlRoundUp( ( int ) ( j + 1 ), 4 ) - ( j + 1 );
		qm_fs_file_seek( file, padding, SEEK_CUR );

		qm_fs_normalize_path( package->files[ i ].name, sizeof( package->files[ i ].name ) );
	}

	return package;
}
