// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Tests for QmFs API.
// Author:  Mark E. Sowden

#include "plcore/pl_filesystem.h"

#include "qmtest/public/qm_test.h"

QM_TEST_FUNC( paths )
{
	QM_TEST_ASSERT( qm_fs_get_path_type( "." ) == QM_FS_PATH_TYPE_DIR );
	QM_TEST_ASSERT( qm_fs_get_path_type( "./qm-fs-test" ) == QM_FS_PATH_TYPE_FILE );
	QM_TEST_ASSERT( qm_fs_get_path_type( "I DO NOT EXIST" ) == QM_FS_PATH_TYPE_INVALID );
}
QM_TEST_FUNC_END()

int main( int, char ** )
{
	TEST_RUN_INIT
	CALL_FUNC_TEST( paths )
	TEST_RUN_END
}
