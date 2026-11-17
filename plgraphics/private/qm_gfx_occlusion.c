// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Interface for performing GPU occlusion queries.
// Author:  Mark E. Sowden

#include "plg_private.h"

#include "plgraphics/qm_gfx_occlusion.h"

static void occlusion_query_destroy( void *ptr )
{
	CallGfxFunction( destroyOcclusionQuery, ( QmGfxOcclusionQuery * ) ptr );
}

QmGfxOcclusionQuery *qm_gfx_occlusion_query_create()
{
	QmGfxOcclusionQuery *query = QM_OS_MEMORY_NEW_D( QmGfxOcclusionQuery, occlusion_query_destroy );
	CallGfxFunction( createOcclusionQuery, query );
	return query;
}

bool qm_gfx_occlusion_query_begin( QmGfxOcclusionQuery *self )
{
	CallReturningGfxFunction( beginOcclusionQuery, false, self );
}

void qm_gfx_occlusion_query_end( QmGfxOcclusionQuery *self )
{
	CallGfxFunction( endOcclusionQuery, self );
}

bool qm_gfx_occlusion_query_get_result( QmGfxOcclusionQuery *self )
{
	CallReturningGfxFunction( getOcclusionQueryResult, false, self );
}
