// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Tests for QmOs API.
// Author:  Mark E. Sowden

#include "qmos/public/qm_os.h"
#include "qmos/public/qm_os_memory.h"
#include "qmos/public/qm_os_shared_ptr.h"
#include "qmos/public/qm_os_string.h"
#include "qmos/public/qm_os_time.h"
#include "qmos/public/qm_os_random.h"
#include "qmos/public/qm_os_linked_list.h"

#include "qmtest/public/qm_test.h"

QM_TEST_FUNC( linked_list )
{
	QmOsLinkedList *list = qm_os_linked_list_create();
	QM_TEST_ASSERT( list != nullptr );

	typedef struct MyItem
	{
		const char         *str;
		QmOsLinkedListNode *listNode;
	} MyItem;

	MyItem *itemA   = QM_OS_MEMORY_NEW( MyItem );
	itemA->str      = "First!";
	itemA->listNode = qm_os_linked_list_push_back( list, itemA );

	MyItem *itemB   = QM_OS_MEMORY_NEW( MyItem );
	itemB->str      = "Second!";
	itemB->listNode = qm_os_linked_list_push_back( list, itemB );

	const QmOsLinkedListNode *front = qm_os_linked_list_get_front( list );
	QM_TEST_ASSERT( front != nullptr );
	MyItem *item = qm_os_linked_list_node_get_data( front );
	QM_TEST_ASSERT( item != nullptr );
	QM_TEST_ASSERT( item == itemA );

	const QmOsLinkedListNode *back = qm_os_linked_list_get_back( list );
	QM_TEST_ASSERT( back != nullptr );
	item = qm_os_linked_list_node_get_data( back );
	QM_TEST_ASSERT( item != nullptr );
	QM_TEST_ASSERT( item == itemB );

	QM_OS_LINKED_LIST_ITERATE( item, list, i )
	{
		QM_TEST_ASSERT( item != nullptr );
		qm_os_memory_free( item );
	}

	qm_os_linked_list_clear( list );
	QM_TEST_ASSERT( qm_os_linked_list_get_size( list ) == 0 );

	qm_os_memory_free( list );
}
QM_TEST_FUNC_END()

typedef struct MyThing
{
	char *thing;
	char *thing2;
} MyThing;

static bool thingDestructorCalled;
static void destroy_thing( void *ptr )
{
	MyThing *thing = ptr;
	qm_os_memory_free( thing->thing );
	qm_os_memory_free( thing->thing2 );

	thingDestructorCalled = true;
}

QM_TEST_FUNC( memory )
{
	MyThing *thing = qm_os_memory_alloc( 1, sizeof( MyThing ), destroy_thing );
	QM_TEST_ASSERT( thing != nullptr );

	static constexpr size_t THING_SIZE = sizeof( MyThing );
	QM_TEST_ASSERT( THING_SIZE == qm_os_memory_get_block_size( thing ) );

	thing->thing = QM_OS_MEMORY_NEW_( char, 128 );
	QM_TEST_ASSERT( thing->thing != nullptr );

	thing->thing2 = QM_OS_MEMORY_NEW_( char, 256 );
	QM_TEST_ASSERT( thing->thing2 != nullptr );

	qm_os_memory_free( thing );
	QM_TEST_ASSERT( thingDestructorCalled );

	QM_TEST_ASSERT( qm_os_memory_get_total() > 0 );
	QM_TEST_ASSERT( qm_os_memory_get_total_available() > 0 );
	QM_TEST_ASSERT( qm_os_memory_get_usage() > 0 );
}
QM_TEST_FUNC_END()

QM_TEST_FUNC( random )
{
	unsigned int seed = qm_os_random_seed_initialize();
	QM_TEST_ASSERT( seed != 0 );

	const int r = qm_os_random_int( &seed ) % 128 + 1;
	QM_TEST_ASSERT( r > 0 && r < 129 );
	QM_TEST_ASSERT( r != qm_os_random_int( &seed ) % 128 + 1 );

	const float rf = qm_os_random_float( &seed, 16.0f ) + 1.0f;
	QM_TEST_ASSERT( rf != 0.f && rf < 17.f );
	QM_TEST_ASSERT( rf != qm_os_random_float( &seed, 16.0f ) + 1.0f );

	const float urf = qm_os_random_uniform_float( &seed, 16.0f );
	QM_TEST_ASSERT( urf > -17.f && urf < 17.f );
	QM_TEST_ASSERT( urf != qm_os_random_uniform_float( &seed, 16.0f ) );

	QM_TEST_ASSERT( seed != qm_os_random_seed_initialize() );
}
QM_TEST_FUNC_END()

QM_TEST_FUNC( shared_ptr )
{
	static char MSG[] = "Hello to you!";

	QmOsSharedPtr *ptr = qm_os_shared_ptr_create( MSG );
	QM_TEST_ASSERT( ptr != nullptr );

	const char *str = qm_os_shared_ptr_get( ptr );
	QM_TEST_ASSERT( str != nullptr );
	QM_TEST_ASSERT( strcmp( str, MSG ) == 0 );

	qm_os_shared_ptr_add( ptr );
	qm_os_shared_ptr_release( ptr );
	QM_TEST_ASSERT( qm_os_shared_ptr_get( ptr ) != nullptr );

	qm_os_shared_ptr_set( ptr, nullptr );
	str = qm_os_shared_ptr_get( ptr );
	QM_TEST_ASSERT( str == nullptr );

	qm_os_shared_ptr_release( ptr );
}
QM_TEST_FUNC_END()

QM_TEST_FUNC( string )
{
	static constexpr char MSG[] = "Hello to you!";

	char *buf = qm_os_string_alloc( "Hello World! %s", MSG );
	QM_TEST_ASSERT( buf != nullptr );
	QM_TEST_ASSERT( strcmp( buf, "Hello World! Hello to you!" ) == 0 );

	char tmp[ 8 ];
	qm_os_string_convert_int( 128, tmp, sizeof( tmp ), 10 );
	QM_TEST_ASSERT( strcmp( tmp, "128" ) == 0 );

	size_t size = strlen( buf );
	if ( qm_os_string_count( buf, 'H', size ) != 2 )
	{
		QM_TEST_FAIL( "Failed on count\n" );
	}

	qm_os_string_reverse( buf, size );
	QM_TEST_ASSERT( strcmp( buf, "!uoy ot olleH !dlroW olleH" ) == 0 );

	QM_TEST_ASSERT( qm_os_string_alpha( "abdc", 4 ) != -1 );
	QM_TEST_ASSERT( qm_os_string_alpha( "1234", 4 ) == -1 );

	QM_TEST_ASSERT( qm_os_string_digit( "-283", 4 ) != -1 );
	QM_TEST_ASSERT( qm_os_string_digit( "abcd", 4 ) == -1 );

	qm_os_string_to_upper( buf, size );
	QM_TEST_ASSERT( strcmp( buf, "!UOY OT OLLEH !DLROW OLLEH" ) == 0 );

	qm_os_string_to_lower( buf, size );
	QM_TEST_ASSERT( strncmp( buf, "!uoy ot olleh !dlrow olleh", 12 ) == 0 );

	QM_TEST_ASSERT( qm_os_string_copy( tmp, "Hello world, how're you?", sizeof( tmp ) ) == 7 );
	QM_TEST_ASSERT( strcmp( tmp, "Hello w" ) == 0 );
	QM_TEST_ASSERT( qm_os_string_copy( buf, "Hello world, how're you?", size ) == 24 );
	QM_TEST_ASSERT( strcmp( buf, "Hello world, how're you?" ) == 0 );
	QM_TEST_ASSERT( qm_os_string_copy( buf, "Hello world, how're you?", 6 ) == 5 );
	QM_TEST_ASSERT( strcmp( buf, "Hello" ) == 0 );
	QM_TEST_ASSERT( qm_os_string_copy( buf, "Hello world, how're you?", 2 ) == 1 );
	QM_TEST_ASSERT( strcmp( buf, "H" ) == 0 );

	qm_os_memory_free( buf );
}
QM_TEST_FUNC_END()

QM_TEST_FUNC( time )
{
	const double seconds = qm_os_time_get_seconds();
	QM_TEST_ASSERT( seconds != 0.0 );

	const long ms = qm_os_time_get_milliseconds();
	QM_TEST_ASSERT( ms != 0 );
}
QM_TEST_FUNC_END()

int main( int, char ** )
{
	TEST_RUN_INIT
	CALL_FUNC_TEST( linked_list )
	CALL_FUNC_TEST( memory )
	CALL_FUNC_TEST( random )
	CALL_FUNC_TEST( shared_ptr )
	CALL_FUNC_TEST( string )
	CALL_FUNC_TEST( time )
	TEST_RUN_END
}
