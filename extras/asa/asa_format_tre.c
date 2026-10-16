/**
 * Hei Platform Library
 * Copyright (C) 2017-2022 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <plcore/pl_hashtable.h>
#include <plcore/pl_package.h>

#include "asa_vexx_files.h"

static PLHashTable *hashTable = NULL;

typedef struct TREIndex {
	uint32_t offset;
	uint32_t size;
	uint32_t nameCRC;
	uint32_t dataCRC;
} TREIndex;
PL_STATIC_ASSERT( sizeof( TREIndex ) == 16, "needs to be 16 bytes" );

static TREIndex *parse_tre_index( PLFile *file, TREIndex *out ) {
	bool status;
	out->offset = ( uint32_t ) PlReadInt32( file, false, &status );
	if ( !status ) {
		return NULL;
	}
	out->size = ( uint32_t ) PlReadInt32( file, false, &status );
	if ( !status ) {
		return NULL;
	}
	out->nameCRC = ( uint32_t ) PlReadInt32( file, false, &status );
	if ( !status ) {
		return NULL;
	}
	out->dataCRC = ( uint32_t ) PlReadInt32( file, false, &status );
	if ( !status ) {
		return NULL;
	}
	return out;
}

static bool validate_tre( PLFile *file ) {
	uint32_t numFiles = ( uint32_t ) PlReadInt32( file, false, NULL );
	if ( numFiles == 0 ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "invalid tre file" );
		return NULL;
	}

	size_t size = PlGetFileSize( file );
	size_t tocSize = sizeof( TREIndex ) * numFiles;
	if ( tocSize >= size ) {
		return false;
	}
	/* try loading in the first index and ensure that's all dandy too */
	TREIndex tmp;
	if ( parse_tre_index( file, &tmp ) == NULL ) {
		return false;
	}
	if ( tmp.offset < ( tocSize + 4 ) || tmp.offset >= size ) {
		return false;
	}

	/* maybe as additional validation we could check the dataCRC matches,
	 * but for now I think the above is good enough */

	PlRewindFile( file );

	return true;
}

static PLPackage *parse_tre_file( PLFile *file ) {
	if ( !validate_tre( file ) ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "invalid tre file" );
		return NULL;
	}

	uint32_t numFiles = ( uint32_t ) PlReadInt32( file, false, NULL );
	if ( numFiles == 0 ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "invalid tre file" );
		return NULL;
	}

	PLPackage *package = PlCreatePackageHandle( PlGetFilePath( file ), numFiles, NULL );
	if ( package == NULL ) {
		return NULL;
	}

	unsigned int numMatches = 0;
	for ( unsigned int i = 0; i < numFiles; ++i ) {
		TREIndex tmp;
		if ( parse_tre_index( file, &tmp ) == NULL ) {
			PlDestroyPackage( package );
			return NULL;
		}

		package->table[ i ].offset = tmp.offset;
		package->table[ i ].fileSize = tmp.size;

		const char *fileName = ( const char * ) PlLookupHashTableUserData( hashTable, &tmp.nameCRC, sizeof( uint32_t ) );
		if ( fileName != NULL ) {
			numMatches++;
			strncpy( package->table[ i ].fileName, fileName, sizeof( package->table[ i ].fileName ) - 1 );
			PlNormalizePath( package->table[ i ].fileName, sizeof( package->table[ i ].fileName ) );
		} else {
			snprintf( package->table[ i ].fileName, sizeof( package->table[ i ].fileSize ), "%X", tmp.nameCRC );
		}
	}

#if !defined( NDEBUG )
	const char *filePath = PlGetFilePath( file );
	const char *fileName = strrchr( filePath, '/' );
	if ( fileName == NULL ) {
		fileName = filePath;
	}
	printf( "%.2lf%% matched (%u/%u) for package \"%s\"\n", ( ( double ) numMatches / numFiles ) * 100.0,
	        numMatches, numFiles,
	        fileName );
#endif

	return package;
}

PLPackage *asa_format_tre_load( const char *path ) {
	if ( hashTable == NULL ) {
		hashTable = qm_os_hash_table();
		for ( unsigned int i = 0; i < numVexxStrings; ++i ) {
			uint32_t hash = pl_crc32( vexxStrings[ i ], strlen( vexxStrings[ i ] ), 0 );
			PlInsertHashTableNode( hashTable, &hash, sizeof( uint32_t ), ( void * ) vexxStrings[ i ] );
		}
	}

	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL ) {
		return NULL;
	}
	PLPackage *package = parse_tre_file( file );
	PlCloseFile( file );
	return package;
}
