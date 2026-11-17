// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: GPU occlusion queries.
// Author:  Mark E. Sowden

#include "xgl.h"

typedef struct XglOcclusionQuery
{
	GLuint query;

	bool isWaiting;
	bool isOccluded;
} XglOcclusionQuery;

void xgl_occlusion_query_create( QmGfxOcclusionQuery *self )
{
	XglOcclusionQuery *drv = gInterface->core->MAlloc( sizeof( XglOcclusionQuery ), true );

	XGL_CALL( glGenQueries( 1, &drv->query ) );

	self->driver = drv;
}

void xgl_occlusion_query_destroy( QmGfxOcclusionQuery *self )
{
	XGL_CALL( glDeleteQueries( 1, &( ( XglOcclusionQuery * ) self->driver )->query ) );
}

bool xgl_occlusion_query_begin( QmGfxOcclusionQuery *self )
{
	const XglOcclusionQuery *driver = self->driver;
	if ( driver->isWaiting )
	{
		return false;
	}

	XGL_CALL( glBeginQuery( GL_ANY_SAMPLES_PASSED, ( ( XglOcclusionQuery * ) self->driver )->query ) );
	return true;
}

void xgl_occlusion_query_end( [[maybe_unused]] QmGfxOcclusionQuery *self )
{
	XGL_CALL( glEndQuery( GL_ANY_SAMPLES_PASSED ) );

	XglOcclusionQuery *driver = self->driver;
	driver->isWaiting         = true;
}

bool xgl_occlusion_query_get_result( QmGfxOcclusionQuery *self )
{
	XglOcclusionQuery *driver = self->driver;

	GLuint status;
	XGL_CALL( glGetQueryObjectuiv( driver->query, GL_QUERY_RESULT_AVAILABLE, &status ) );
	if ( !status )
	{
		// return the last result
		driver->isWaiting = true;
		return !driver->isOccluded;
	}

	XGL_CALL( glGetQueryObjectuiv( driver->query, GL_QUERY_RESULT, &status ) );
	driver->isOccluded = !status;
	driver->isWaiting  = false;
	return !driver->isOccluded;
}
