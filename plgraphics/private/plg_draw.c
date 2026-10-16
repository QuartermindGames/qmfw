// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Helper drawing methods.
//			This is deprecated!
// Author:  Mark E. Sowden

#include "plg_private.h"

/****************************************
 * Immediate-mode style API
 * Possibly consider moving these into
 * some higher-level library?
 ****************************************/

static QmGfxMesh *currentDynamicMesh;

static constexpr unsigned int MAXIMUM_STORAGE = 4096;

static unsigned int currentTriangle;
static unsigned int currentVertex;

static QmGfxMesh *meshes[ QM_GFX_MESH_PRIMITIVE_MAX ];

QmGfxMesh *PlgImmBegin( QmGfxMeshPrimitive primitive )
{
	if ( meshes[ primitive ] == NULL )
	{
		meshes[ primitive ] = qm_gfx_mesh_create( primitive, QM_GFX_MESH_DRAW_MODE_STREAM, MAXIMUM_STORAGE, MAXIMUM_STORAGE );
	}

	currentDynamicMesh = meshes[ primitive ];
	PlgClearMesh( currentDynamicMesh );
	qm_gfx_mesh_set_primitive_scale( currentDynamicMesh, 1.0f );

	currentVertex = 0;

	return currentDynamicMesh;
}

unsigned int PlgImmPushVertex( float x, float y, float z )
{
	return currentVertex = PlgAddMeshVertex( currentDynamicMesh, &QM_MATH_VECTOR3F( x, y, z ), &QM_MATH_VECTOR3F_ZERO, &PL_COLOUR_WHITE, &QM_MATH_VECTOR2F_ZERO );
}

void PlgImmNormal( float x, float y, float z )
{
	currentDynamicMesh->vertices[ currentVertex ].normal.x = x;
	currentDynamicMesh->vertices[ currentVertex ].normal.y = y;
	currentDynamicMesh->vertices[ currentVertex ].normal.z = z;
}

void PlgImmColour( uint8_t r, uint8_t g, uint8_t b, uint8_t a )
{
	currentDynamicMesh->vertices[ currentVertex ].colour.r = r;
	currentDynamicMesh->vertices[ currentVertex ].colour.g = g;
	currentDynamicMesh->vertices[ currentVertex ].colour.b = b;
	currentDynamicMesh->vertices[ currentVertex ].colour.a = a;
}

void PlgImmTextureCoord( float s, float t )
{
	currentDynamicMesh->vertices[ currentVertex ].st[ 0 ].x = s;
	currentDynamicMesh->vertices[ currentVertex ].st[ 0 ].y = t;
}

unsigned int PlgImmPushTriangle( unsigned int x, unsigned int y, unsigned int z )
{
	return currentTriangle = PlgAddMeshTriangle( currentDynamicMesh, x, y, z );
}

void PlgImmSetPrimitiveScale( float scale )
{
	qm_gfx_mesh_set_primitive_scale( currentDynamicMesh, scale );
}

void PlgImmDraw( void )
{
	QmGfxShaderProgram *program = PlgGetCurrentShaderProgram();
	if ( program )
	{
		int slot;
		if ( ( slot = qm_gfx_shader_program_get_uniform_slot( program, "pl_model" ) ) >= 0 )
		{
			qm_gfx_shader_program_set_uniform( program, slot, PlGetMatrix( PL_MODELVIEW_MATRIX ), false );
		}
		if ( ( slot = qm_gfx_shader_program_get_uniform_slot( program, "pl_texture" ) ) >= 0 )
		{
			qm_gfx_shader_program_set_uniform( program, slot, PlGetMatrix( PL_TEXTURE_MATRIX ), false );
		}
	}

	qm_gfx_mesh_upload( currentDynamicMesh, nullptr, nullptr );
	qm_gfx_mesh_draw( currentDynamicMesh );
}

unsigned int PlgPushTriangle( QmGfxMesh *mesh, unsigned int x, unsigned int y, unsigned int z )
{
	return PlgAddMeshTriangle( mesh, x, y, z );
}

unsigned int PlgPushVertex3f( QmGfxMesh *mesh, float x, float y, float z )
{
	return PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( x, y, z ), &QM_MATH_VECTOR3F_ZERO, &PL_COLOUR_WHITE, &QM_MATH_VECTOR2F_ZERO );
}

void PlgColour4bv( QmGfxMesh *mesh, const QmMathColour4ub *col )
{
	mesh->vertices[ mesh->num_verts - 1 ].colour = *col;
}

/****************************************
 ****************************************/

void PlgClearInternalMeshes( void )
{
	for ( unsigned int i = 0; i < QM_GFX_MESH_PRIMITIVE_MAX; ++i )
	{
		if ( meshes[ i ] == NULL )
		{
			continue;
		}

		qm_os_memory_free( meshes[ i ] );
		meshes[ i ] = nullptr;
	}
}

static void SetupRectangleMesh( float x, float y, float w, float h, QmMathColour4ub colour )
{
	PlgImmPushVertex( x, y, 0.0f );
	PlgImmTextureCoord( 0.0f, 0.0f );
	PlgImmColour( colour.r, colour.g, colour.b, colour.a );

	PlgImmPushVertex( x, y + h, 0.0f );
	PlgImmTextureCoord( 0.0f, 1.0f );
	PlgImmColour( colour.r, colour.g, colour.b, colour.a );

	PlgImmPushVertex( x + w, y, 0.0f );
	PlgImmTextureCoord( 1.0f, 0.0f );
	PlgImmColour( colour.r, colour.g, colour.b, colour.a );

	PlgImmPushVertex( x + w, y + h, 0.0f );
	PlgImmTextureCoord( 1.0f, 1.0f );
	PlgImmColour( colour.r, colour.g, colour.b, colour.a );
}

void PlgDrawRectangle( float x, float y, float w, float h, QmMathColour4ub colour )
{
	PlgImmBegin( QM_GFX_MESH_PRIMITIVE_TRIANGLE_STRIP );

	SetupRectangleMesh( x, y, w, h, colour );

	PlgImmDraw();
}

void PlgDrawLineRectangle( float x, float y, float w, float h, QmMathColour4ub colour )
{
	PlgImmBegin( QM_GFX_MESH_PRIMITIVE_LINE_LOOP );
	PlgImmPushVertex( x, y, 0.0f );
	PlgImmColour( colour.r, colour.g, colour.b, colour.a );
	PlgImmPushVertex( x + w, y, 0.0f );
	PlgImmColour( colour.r, colour.g, colour.b, colour.a );
	PlgImmPushVertex( x + w, y + h, 0.0f );
	PlgImmColour( colour.r, colour.g, colour.b, colour.a );
	PlgImmPushVertex( x, y + h, 0.0f );
	PlgImmColour( colour.r, colour.g, colour.b, colour.a );
	PlgImmDraw();
}

void PlgDrawLines( const QmMathVector3f *points, unsigned int numPoints, QmMathColour4ub colour, float thickness )
{
	QmGfxMesh *mesh      = PlgImmBegin( QM_GFX_MESH_PRIMITIVE_LINES );
	mesh->primitiveScale = thickness;

	for ( unsigned int i = 0; i < numPoints; ++i )
	{
		PlgImmPushVertex( points[ i ].x, points[ i ].y, points[ i ].z );
		PlgImmColour( colour.r, colour.g, colour.b, colour.a );
	}

	PlgImmDraw();

	mesh->primitiveScale = 1.0f;
}

void PlgDrawLine( QmMathVector3f startPos, QmMathColour4ub startColour, QmMathVector3f endPos, QmMathColour4ub endColour )
{
	PlgImmBegin( QM_GFX_MESH_PRIMITIVE_LINES );

	PlgImmPushVertex( startPos.x, startPos.y, startPos.z );
	PlgImmColour( startColour.r, startColour.g, startColour.b, startColour.a );

	PlgImmPushVertex( endPos.x, endPos.y, endPos.z );
	PlgImmColour( endColour.r, endColour.g, endColour.b, endColour.a );

	PlgImmDraw();
}

void PlgDrawSimpleLine( QmMathVector3f startPos, QmMathVector3f endPos, QmMathColour4ub colour )
{
	PlgDrawLine( startPos, colour, endPos, colour );
}
