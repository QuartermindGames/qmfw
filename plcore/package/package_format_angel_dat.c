// SPDX-License-Identifier: MIT
// Hei Platform Library
// Copyright © 2017-2024 Mark E Sowden <hogsy@oldtimes-software.com>

#include "package_private.h"

#include "qmos/public/qm_os.h"
#include "qmos/public/qm_os_memory.h"

#define ANGEL_DAT_MAGIC  QM_OS_MAGIC_TO_NUM( 'D', 'A', 'V', 'E' ) /* oni2 */
#define ANGEL_DAT_MAGIC2 QM_OS_MAGIC_TO_NUM( 'D', 'a', 'v', 'e' ) /* tsnw, rdr, sh2 */

typedef struct AngelDATHeader {
	uint32_t magic;// 'DAVE'/'Dave'
	uint32_t tocIndices;
	uint32_t tocLength;
	uint32_t stringTableLength;
} AngelDATHeader;

typedef struct AngelDATIndex {
	uint32_t nameOffset;    // offset into string table
	uint32_t offset;        // absolute offset
	uint32_t size;          // size in memory
	uint32_t compressedSize;// size on disk - if same as memory, not compressed
} AngelDATIndex;

QmFsPackage *PlParseAngelDatPackage_( QmFsFile *file ) {
	AngelDATHeader header = {};

	header.magic = QM_FS_FILE_READ_UINT32( file, false, NULL );
	if ( header.magic != ANGEL_DAT_MAGIC && header.magic != ANGEL_DAT_MAGIC2 ) {
		PlReportBasicError( PL_RESULT_FILETYPE );
		return NULL;
	}

	header.tocIndices = QM_FS_FILE_READ_UINT32( file, false, NULL );
	header.tocLength = QM_FS_FILE_READ_UINT32( file, false, NULL );
	header.stringTableLength = QM_FS_FILE_READ_UINT32( file, false, NULL );

	size_t fileSize = qm_fs_file_get_size( file );
	if ( header.tocLength >= fileSize || header.stringTableLength >= fileSize ) {
		PlReportBasicError( PL_RESULT_FILEERR );
		return NULL;
	}

	/* toc is always padded to 2048 bytes */
	qm_fs_file_seek( file, 2048, QM_FS_SEEK_SET );

	AngelDATIndex *indices = QM_OS_MEMORY_NEW_( AngelDATIndex, header.tocIndices );
	for ( unsigned int i = 0; i < header.tocIndices; ++i ) {
		indices[ i ].nameOffset = QM_FS_FILE_READ_UINT32( file, false, NULL );
		indices[ i ].offset = QM_FS_FILE_READ_UINT32( file, false, NULL );
		indices[ i ].size = QM_FS_FILE_READ_UINT32( file, false, NULL );
		indices[ i ].compressedSize = QM_FS_FILE_READ_UINT32( file, false, NULL );
	}

	/* and now seek to the string table */
	qm_fs_file_seek( file, 2048 + header.tocLength, QM_FS_SEEK_SET );

	char *names = QM_OS_MEMORY_NEW_( char, header.stringTableLength );
	qm_file_read( file, names, sizeof( char ), header.stringTableLength );

	bool encodedStrings = false;
	if ( header.magic == ANGEL_DAT_MAGIC2 ) {
		encodedStrings = true;
	}

	const char *path = qm_fs_file_get_path( file );
	QmFsPackage *package = PlCreatePackageHandle( path, header.tocIndices, NULL );
	for ( unsigned int i = 0; i < package->maxFiles; ++i ) {
		package->files[ i ].offset = indices[ i ].offset;
		package->files[ i ].compressedSize = indices[ i ].compressedSize;
		package->files[ i ].size = indices[ i ].size;
		if ( indices[ i ].compressedSize != indices[ i ].size ) {
			package->files[ i ].compressionType = PL_COMPRESSION_GZIP;
		}

		/* no idea right now, so just spit by index */
		if ( encodedStrings ) {
			snprintf( package->files[ i ].name, sizeof( package->files[ i ].name ), "%d", i );
			continue;
		}

		strncpy( package->files[ i ].name, &names[ indices[ i ].nameOffset ], sizeof( package->files[ i ].name ) - 1 );
	}

	qm_os_memory_free( indices );
	qm_os_memory_free( names );

	return package;
}
