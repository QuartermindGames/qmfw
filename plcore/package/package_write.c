// SPDX-License-Identifier: MIT
// Copyright © 2017-2023 Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_hashtable.h>

#include "package_private.h"

#if defined( PL_PACKAGE_WRITE_ENABLED )

static PLHashTable *packageWriteFormats = NULL;//PackageWriter

void PlRegisterPackageWriter( const char *formatTag, PLWritePackageFunction writeFunction ) {
	if ( packageWriteFormats == NULL ) {
		packageWriteFormats = qm_os_hash_table();
	}

	PlInsertHashTableNode( packageWriteFormats, formatTag, strlen( formatTag ), writeFunction );
}

void PlRegisterStandardPackageWriters( unsigned int flags ) {
	typedef struct PackageWriter {
		unsigned int flag;
		const char *tag;
		PLWritePackageFunction writeFunction;
	} PackageWriter;

	static const PackageWriter writers[] = {
	        {PL_PACKAGE_WRITE_FORMAT_BIN_FRESH, PL_PACKAGE_FORMAT_TAG_BIN_FRESH, NULL},
	};

	for ( unsigned int i = 0; i < QM_OS_ARRAY_ELEMENTS( writers ); ++i ) {
		if ( flags != PL_PACKAGE_LOAD_FORMAT_ALL && !( flags & writers[ i ].flag ) ) {
			continue;
		}

		PlRegisterPackageWriter( writers[ i ].tag, writers[ i ].writeFunction );
	}
}

void PlClearPackageWriters( void ) {
	qm_os_memory_free( packageWriteFormats );
}

QmFsPackageFile *PlAppendPackageFromFile( QmFsPackage *package, const char *source, const char *filename, PLCompressionType compressionType ) {
	if ( package->numFiles >= package->maxFiles ) {
		static const unsigned int INC = 64;
		unsigned int newMaxSize = package->maxFiles + INC;
		package->files = qm_os_memory_realloc( package->files, newMaxSize );
		package->maxFiles = newMaxSize;
	}

	QmFsPackageFile *index = &package->files[ package->numFiles ];
	strncpy( index->name, filename, sizeof( index->name ) - 1 );
	index->compressionType = compressionType;

	package->numFiles++;
	return index;
}

bool PlWritePackage( QmFsPackage *package, const char *path, const char *formatTag ) {
	if ( packageWriteFormats == NULL ) {
		PlReportErrorF( PL_RESULT_UNSUPPORTED, "no package writers registered" );
		return false;
	}

	PLWritePackageFunction writeFunction = PlLookupHashTableUserData( packageWriteFormats, formatTag, strlen( formatTag ) );
	if ( writeFunction == NULL ) {
		return false;
	}

	return writeFunction( package, path );
}

#endif
