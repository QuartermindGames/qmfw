/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include "plg_private.h"

#include "qmos/public/qm_os_memory.h"

void PlgGenerateTextureCoordinates( QmGfxMeshVertex *vertices, unsigned int numVertices, QmMathVector2f textureOffset, QmMathVector2f textureScale )
{
	if ( textureScale.x == 0.0f || textureScale.y == 0.0f )
	{
		return;
	}

	unsigned int x, y;
	for ( unsigned int i = 0; i < numVertices; ++i )
	{
		if ( fabsf( vertices[ i ].normal.x ) > fabsf( vertices[ i ].normal.y ) &&
		     fabsf( vertices[ i ].normal.x ) > fabsf( vertices[ i ].normal.z ) )
		{
			x = vertices[ i ].normal.x > 0.0 ? 1 : 2;
			y = vertices[ i ].normal.x > 0.0 ? 2 : 1;
		}
		else if ( fabsf( vertices[ i ].normal.z ) > fabsf( vertices[ i ].normal.x ) &&
		          fabsf( vertices[ i ].normal.z ) > fabsf( vertices[ i ].normal.y ) )
		{
			x = vertices[ i ].normal.z > 0.0 ? 0 : 1;
			y = vertices[ i ].normal.z > 0.0 ? 1 : 0;
		}
		else
		{
			x = vertices[ i ].normal.y > 0.0 ? 2 : 0;
			y = vertices[ i ].normal.y > 0.0 ? 0 : 2;
		}

		vertices[ i ].st[ 0 ].x = ( PL_VECTOR3_I( vertices[ i ].position, x ) + textureOffset.x ) / textureScale.x;
		vertices[ i ].st[ 0 ].y = ( PL_VECTOR3_I( vertices[ i ].position, y ) + textureOffset.y ) / textureScale.y;
	}
}

QmGfxMesh *qm_gfx_mesh_create( QmGfxMeshPrimitive primitive, QmGfxMeshDrawMode mode, unsigned int numTriangles, unsigned int numVertices )
{
	QmGfxMesh *mesh = QM_OS_MEMORY_CALLOC( 1, sizeof( QmGfxMesh ) );
	mesh->primitive = primitive;
	mesh->mode      = mode;

	if ( numTriangles > 0 )
	{
		if ( mesh->primitive == QM_GFX_MESH_PRIMITIVE_TRIANGLES )
		{
			unsigned int numIndices = numTriangles * 3; /* todo: this is too assumptious... */
			mesh->maxIndices        = numIndices;
			mesh->indices           = QM_OS_MEMORY_CALLOC( mesh->maxIndices, sizeof( unsigned int ) );
		}
	}

	mesh->maxVertices = numVertices;
	mesh->vertices    = QM_OS_MEMORY_NEW_( QmGfxMeshVertex, mesh->maxVertices );

	static constexpr QmGfxMeshVertexAttribute DEFAULT_ATTRIBUTES[] = {
	        {0, 3, QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_FLOAT32, offsetof( QmGfxMeshVertex, position )                         },
	        {1, 3, QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_FLOAT32, offsetof( QmGfxMeshVertex, normal )                           },
	        {2, 4, QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_UINT8,   offsetof( QmGfxMeshVertex, colour )                           },
	        {3, 3, QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_FLOAT32, offsetof( QmGfxMeshVertex, tangent )                          },
	        {4, 3, QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_FLOAT32, offsetof( QmGfxMeshVertex, bitangent )                        },
	        {5, 2, QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_FLOAT32, offsetof( QmGfxMeshVertex, st )                               },
	        {6, 2, QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_FLOAT32, offsetof( QmGfxMeshVertex, st ) + 1 * sizeof( QmMathVector2f )},
	        {7, 2, QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_FLOAT32, offsetof( QmGfxMeshVertex, st ) + 2 * sizeof( QmMathVector2f )},
	        {8, 2, QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_FLOAT32, offsetof( QmGfxMeshVertex, st ) + 3 * sizeof( QmMathVector2f )},
	};
	static constexpr unsigned int NUM_DEFAULT_ATTRIBUTES = QM_OS_ARRAY_ELEMENTS( DEFAULT_ATTRIBUTES );

	//TODO: temporary
	qm_gfx_mesh_set_vertex_layout( mesh, DEFAULT_ATTRIBUTES, NUM_DEFAULT_ATTRIBUTES, sizeof( QmGfxMeshVertex ) );

	mesh->isDirty = true;

	CallGfxFunction( CreateMesh, mesh );

	return mesh;
}

void qm_gfx_mesh_destroy( QmGfxMesh *mesh )
{
	if ( mesh == NULL )
	{
		return;
	}

	CallGfxFunction( DeleteMesh, mesh );

	qm_os_memory_free( mesh->vertices );
	qm_os_memory_free( mesh->indices );
	qm_os_memory_free( mesh );
}

void PlgClearMesh( QmGfxMesh *mesh )
{
	mesh->num_verts = 0;
	mesh->range = mesh->start = 0;
	mesh->num_triangles = mesh->num_indices = 0;

	mesh->numSubMeshes = 0;

	mesh->isDirty = true;
}

/**
 * Sets the draw scale for the particular primitive type.
 * Only applies for POINTS / LINES.
 */
void qm_gfx_mesh_set_primitive_scale( QmGfxMesh *mesh, float scale )
{
	mesh->primitiveScale = scale;
}

unsigned int PlgAddMeshVertex( QmGfxMesh *mesh, const QmMathVector3f *position, const QmMathVector3f *normal, const QmMathColour4ub *colour, const QmMathVector2f *st )
{
	unsigned int vertexIndex = mesh->num_verts++;
	if ( vertexIndex >= mesh->maxVertices )
	{
		mesh->vertices = qm_os_memory_realloc( mesh->vertices, ( mesh->maxVertices += 16 ) * sizeof( QmGfxMeshVertex ) );
	}

	mesh->vertices[ vertexIndex ].position = *position;
	mesh->vertices[ vertexIndex ].normal   = *normal;
	mesh->vertices[ vertexIndex ].colour   = *colour;
	mesh->vertices[ vertexIndex ].st[ 0 ]  = *st;

	return vertexIndex;
}

unsigned int PlgAddMeshTriangle( QmGfxMesh *mesh, unsigned int x, unsigned int y, unsigned int z )
{
	unsigned int triangleIndex = mesh->num_indices;

	mesh->num_indices += 3;
	if ( mesh->num_indices >= mesh->maxIndices )
	{
		mesh->indices = qm_os_memory_realloc( mesh->indices, ( mesh->maxIndices += 16 ) * sizeof( unsigned int ) );
	}

	mesh->range = mesh->num_indices;

	mesh->indices[ triangleIndex ]     = x;
	mesh->indices[ triangleIndex + 1 ] = y;
	mesh->indices[ triangleIndex + 2 ] = z;

	mesh->num_triangles++;

	return triangleIndex;
}

void qm_gfx_mesh_upload( QmGfxMesh *mesh, const void *vertexPtr, const void *elementsPtr )
{
	CallGfxFunction( UploadMesh, mesh, gfx_state.current_program,
	                 vertexPtr != nullptr ? vertexPtr : mesh->vertices,
	                 elementsPtr != nullptr ? elementsPtr : mesh->indices );
}

void qm_gfx_mesh_draw( QmGfxMesh *mesh )
{
	if ( gfx_state.current_program != NULL )
	{
		int slot;
		if ( ( slot = qm_gfx_shader_program_get_uniform_slot( gfx_state.current_program, "pl_view" ) ) != -1 )
		{
			qm_gfx_shader_program_set_uniform( gfx_state.current_program, slot, gfx_state.view_matrix.m, false );
		}
		if ( ( slot = qm_gfx_shader_program_get_uniform_slot( gfx_state.current_program, "pl_proj" ) ) != -1 )
		{
			qm_gfx_shader_program_set_uniform( gfx_state.current_program, slot, gfx_state.projection_matrix.m, false );
		}
	}

	CallGfxFunction( DrawMesh, mesh, gfx_state.current_program );
}

/**
 * Draws a collection of subsets of the given mesh.
 */
void qm_gfx_mesh_draw_sub( QmGfxMesh *mesh, int32_t *firstSubMeshes, int32_t *subMeshes, uint32_t numSubMeshes )
{
	// urgh, this is just a botch for now,
	// eventually we should introduce a proper call for it, probably
	mesh->subMeshes      = subMeshes;
	mesh->firstSubMeshes = firstSubMeshes;
	mesh->numSubMeshes   = numSubMeshes;

	qm_gfx_mesh_draw( mesh );

	// we can just set it to 0 here, and it won't operate any more...
	mesh->numSubMeshes = 0;
}

void qm_gfx_mesh_draw_instanced( QmGfxMesh *mesh, const PLMatrix4 *transforms, unsigned int instanceCount )
{
	CallGfxFunction( DrawInstancedMesh, mesh, gfx_state.current_program, transforms, instanceCount );
}

/////////////////////////////////////////////////////////////////////////////////////
// Vertex Attribute Declarations
/////////////////////////////////////////////////////////////////////////////////////

QmGfxResult qm_gfx_mesh_set_vertex_layout( QmGfxMesh *mesh, const QmGfxMeshVertexAttribute *attributes, unsigned int numAttributes, size_t size )
{
	if ( mesh->vertexDescriptor.numAttributes >= QM_GFX_MESH_MAX_ATTRIBUTES )
	{
		return -1;
	}

	// we don't yet expose our hashing methods to the backend, so we'll have to compute the hash here
	// but why? WHY!? well we need a vao on gl side, so we hash it so we only create one as and when
	// necessary (which I *think* makes sense?)
	mesh->vertexDescriptor.attributeTableHash = PlGenerateHashFNV1( attributes, sizeof( QmGfxMeshVertexAttribute ) * numAttributes );

	mesh->vertexDescriptor.numAttributes = numAttributes;
	memcpy( mesh->vertexDescriptor.attributes, attributes, sizeof( QmGfxMeshVertexAttribute ) * numAttributes );

	mesh->vertexDescriptor.size = size;

	return QM_GFX_RESULT_SUCCESS;
}
