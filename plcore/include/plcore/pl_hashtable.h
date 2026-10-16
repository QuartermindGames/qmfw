// SPDX-License-Identifier: MIT
// Hei Platform Library
// Copyright © 2017-2024 Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <plcore/pl.h>

PL_EXTERN_C

typedef struct PLHashTable PLHashTable;
typedef struct PLHashTableNode PLHashTableNode;

PLHashTable *qm_os_hash_table( void );
void PlDestroyHashTableEx( PLHashTable *hashTable, void ( *elementDeleter )( void *user ) );
void PlClearHashTable( PLHashTable *hashTable );

PLHashTableNode *PlLookupHashTableNode( const PLHashTable *hashTable, const void *key, size_t keySize );
void *PlLookupHashTableUserData( const PLHashTable *hashTable, const void *key, size_t keySize );

PLHashTableNode *PlInsertHashTableNode( PLHashTable *hashTable, const void *key, size_t keySize, void *value );
void PlDestroyHashTableNode( PLHashTableNode *hashTableNode );
unsigned int PlGetNumHashTableNodes( const PLHashTable *hashTable );

PLHashTableNode *PlGetFirstHashTableNode( PLHashTable *hashTable );
PLHashTableNode *PlGetNextHashTableNode( PLHashTableNode *hashTableNode );
void *PlGetHashTableNodeUserData( PLHashTableNode *hashTableNode );
void PlSetHashTableNodeUserData( PLHashTableNode *hashTableNode, void *value );

#define PL_ITERATE_HASHED_LIST( VAR, TYPE, LIST, ITR )                                               \
	for ( PLHashTableNode * ( ITR ) = PlGetFirstHashTableNode( LIST );                               \
	      ( ITR ) != NULL && ( ( ( VAR ) = ( TYPE * ) PlGetHashTableNodeUserData( ITR ) ) != NULL ); \
	      ( ITR ) = PlGetNextHashTableNode( ITR ) )

PL_EXTERN_C_END
