// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

/////////////////////////////////////////////////////////////////////////////////////

typedef struct QmGfxOcclusionQuery QmGfxOcclusionQuery;

/**
 * Allocate a new GPU occlusion query.
 * Call memory_free to destroy (will auto-destroy nodes).
 */
QmGfxOcclusionQuery *qm_gfx_occlusion_query_create();

/**
 * If the last query is still waiting, this will return false.
 */
bool qm_gfx_occlusion_query_begin( QmGfxOcclusionQuery *self );

/**
 * ...
 */
void qm_gfx_occlusion_query_end( QmGfxOcclusionQuery *self );

/**
 * Attempts to fetch the query result.
 * Mind that depending on the backend implementation, these likely operate async, and
 * in such a case the result hasn't been returned yet, it will return the last
 * successful result.
 */
bool qm_gfx_occlusion_query_get_result( QmGfxOcclusionQuery *self );
