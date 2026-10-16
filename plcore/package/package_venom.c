// SPDX-License-Identifier: MIT
// Hei Platform Library
// Copyright © 2017-2025 Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Venom PAK loader

#include "package_private.h"

#include "qmparse/public/qm_parse.h"
#include "qmos/public/qm_os_memory.h"

// big-endian

static const char VENOM_PAK_IDENTIFIER[] = "VENOMBINPAK1.0\n";

QmFsPackage *PlParseVenomPakPackage( QmFsFile *file ) {
	char ident[ 15 ];
	qm_file_read( file, ident, sizeof( char ), sizeof( ident ) );
	if ( strncmp( VENOM_PAK_IDENTIFIER, ident, sizeof( ident ) ) != 0 ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "file is not a valid venom package" );
		return NULL;
	}

	uint32_t headerSize = QM_FS_FILE_READ_UINT32( file, true, NULL );
	if ( headerSize == 0 ) {
		PlReportErrorF( PL_RESULT_FILEERR, "invalid header size (%u)", headerSize );
		return NULL;
	}

	uint32_t numFiles = QM_FS_FILE_READ_UINT32( file, true, NULL );
	if ( numFiles == 0 ) {
		PlReportErrorF( PL_RESULT_FILEERR, "invalid number of files" );
		return NULL;
	}

	qm_fs_file_read_int32( file, true, NULL );// unk0

	size_t bufSize = headerSize - qm_fs_file_get_offset( file );
	char *buf = QM_OS_MEMORY_NEW_( char, bufSize + 1 );
	if ( qm_file_read( file, buf, sizeof( char ), bufSize ) != bufSize ) {
		qm_os_memory_free( buf );
		return NULL;
	}

	QmFsPackage *package = PlCreatePackageHandle( qm_fs_file_get_path( file ), numFiles, NULL );

	const char *p = buf;
	for ( unsigned int i = 0; i < numFiles; ++i ) {
		if ( *p == '\0' ) {
			break;
		}

		char line[ 512 ] = {};

		// name
		if ( qm_parse_line( &p, line, sizeof( line ) ) == NULL ) {
			PlReportErrorF( PL_RESULT_FILEREAD, "failed to pull name %u", i );
			break;
		}
		PlSetupPath( package->files[ i ].name, true, "%s", ( *line == 'd' && *( line + 1 ) == ':' ) ? line + 3 : line );

		// filesize
		if ( qm_parse_line( &p, line, sizeof( line ) ) == NULL ) {
			break;
		}
		package->files[ i ].size = strtoul( line, NULL, 10 );

		// offset
		if ( qm_parse_line( &p, line, sizeof( line ) ) == NULL ) {
			break;
		}
		package->files[ i ].offset = headerSize + ( PLFileOffset ) strtoul( line, NULL, 10 );

		// unknown...
		if ( qm_parse_line( &p, line, sizeof( line ) ) == NULL ) {
			break;
		}
	}

	qm_os_memory_free( buf );

	return package;
}
