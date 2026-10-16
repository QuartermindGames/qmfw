// SPDX-License-Identifier: MIT
// Hei Platform Library
// Copyright © 2017-2025 Mark E Sowden <hogsy@oldtimes-software.com>

#include "package_private.h"

#include "qmos/public/qm_os_memory.h"
#include "qmparse/public/qm_parse.h"

#include <plcore/pl_hashtable.h>

// Loader for Free Radical Design's PAK format.

#define PAK5_MAGIC QM_OS_MAGIC_TO_NUM( 'P', '5', 'C', 'K' )

typedef struct Pak5Header {
	uint32_t magic;
	uint32_t tocOffset;
	uint32_t tocSize;
} Pak5Header;

typedef struct Pak5Index {
	uint32_t checksum;
	uint32_t offset;
	uint32_t size;
	uint32_t compressedSize;//?
} Pak5Index;

static PLHashTable *populate_name_table( const QmFsFile *file ) {
	PLPath path;
	PlSetupPath( path, true, qm_fs_file_get_path( file ) );
	char *c = strrchr( path, '.' );
	if ( c == NULL ) {
		return NULL;
	}

	*c = '\0';
	if ( PlAppendPath( path, ".c2n", false ) == NULL ) {
		return NULL;
	}

	QmFsFile *nameFile = qm_fs_file_open( path, false );
	if ( nameFile == NULL ) {
		return NULL;
	}

	PLHashTable *nameTable = qm_os_hash_table();
	while ( true ) {
		char line[ 1024 ];
		const char *p = qm_fs_file_read_string( nameFile, line, sizeof( line ) );
		if ( p == NULL ) {
			break;
		}

		char token[ 256 ];

		// checksum
		if ( qm_parse_token( &p, token, sizeof( token ) ) == NULL ) {
			break;
		}
		uint32_t checksum = strtoul( token, NULL, 16 );

		// name
		if ( qm_parse_token( &p, token, sizeof( token ) ) == NULL ) {
			break;
		}

		char *name = QM_OS_MEMORY_NEW_( char, strlen( token ) + 1 );
		strcpy( name, token );

		PlInsertHashTableNode( nameTable, &checksum, sizeof( checksum ), name );
	}

	return nameTable;
}

QmFsPackage *PlParseFrdPakPackage_( QmFsFile *file ) {
	Pak5Header header;
	if ( qm_file_read( file, &header, sizeof( Pak5Header ), 1 ) != 1 ) {
		return NULL;
	}

	if ( header.magic != PAK5_MAGIC ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "invalid magic number" );
		return NULL;
	}

	if ( header.tocOffset == 0 || header.tocOffset >= qm_fs_file_get_size( file ) ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "invalid TOC offset" );
		return NULL;
	}

	if ( !qm_fs_file_seek( file, header.tocOffset, SEEK_SET ) ) {
		return NULL;
	}

	unsigned int numFiles = header.tocSize / sizeof( Pak5Index );
	if ( numFiles == 0 ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "invalid number of files" );
		return NULL;
	}

	QmFsPackage *package = PlCreatePackageHandle( qm_fs_file_get_path( file ), numFiles, NULL );

	// see if there's a checksum to name file
	PLHashTable *nameTable = populate_name_table( file );

	unsigned int i;
	for ( i = 0; i < numFiles; ++i ) {
		Pak5Index index;
		if ( qm_file_read( file, &index, sizeof( Pak5Index ), 1 ) != 1 ) {
			break;
		}
		if ( index.offset == 0 || index.offset >= qm_fs_file_get_size( file ) ) {
			PlReportErrorF( PL_RESULT_FILETYPE, "invalid file (%u) offset (%u)", i, index.offset );
			break;
		}

		package->files[ i ].offset = index.offset;
		package->files[ i ].size = index.size;

		if ( nameTable != NULL ) {
			const char *name = PlLookupHashTableUserData( nameTable, &index.checksum, sizeof( index.checksum ) );
			if ( name != NULL ) {
				snprintf( package->files[ i ].name, sizeof( package->files[ i ].name ), "%s", name );
			}
		}
	}

	PlDestroyHashTableEx( nameTable, qm_os_memory_free );

	if ( i != numFiles ) {
		PlDestroyPackage( package );
		package = NULL;
	}

	return package;
}
