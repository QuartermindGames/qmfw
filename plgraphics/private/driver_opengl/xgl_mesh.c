// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: OpenGL mesh management stuff :)
// Author:  Mark E. Sowden

#include "xgl.h"

typedef struct XglMesh
{
	unsigned int buffers[ XGL_MESH_BUFFER_MAX ];
	unsigned int vertexLayout;
} XglMesh;

/////////////////////////////////////////////////////////////////////////////////////
// VAO Management
// Urgh... This *should* use a hash lookup, but that's not
// exposed to our dumb plugin interface thing which is going
// to go out the window anyway. So, that's why this is just
// awful for now. Sorry! :(

// I decided this probably doesn't matter, so parked and put it behind this for now
// odds are an application is only going to create so many of these and they'll
// rarely change (my engine only needs three?)
//#define XGL_MESH_VAO_REF_COUNTING

typedef struct XglMeshVertexLayout
{
	GLuint   vao;
	uint64_t hash;
#ifdef XGL_MESH_VAO_REF_COUNTING
	unsigned int refs;
#endif
} XglMeshVertexLayout;

static unsigned int         maxMeshVertexLayouts;
static unsigned int         numMeshVertexLayouts;
static XglMeshVertexLayout *meshVertexLayouts;

static unsigned int xgl_translate_attribute_type( const QmGfxMeshVertexAttributeType type )
{
	switch ( type )
	{
		default: return XGL_INVALID;
		case QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_INT8: return GL_BYTE;
		case QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_INT16: return GL_SHORT;
		case QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_INT32: return GL_INT;
		case QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_UINT8: return GL_UNSIGNED_BYTE;
		case QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_UINT16: return GL_UNSIGNED_SHORT;
		case QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_UINT32: return GL_UNSIGNED_INT;
		case QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_FLOAT16: return GL_HALF_FLOAT;
		case QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_FLOAT32: return GL_FLOAT;
		case QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_FLOAT64: return GL_DOUBLE;
	}
}

static unsigned int xgl_mesh_vao_get( const QmGfxMeshVertexAttribute *attributes, unsigned int numAttributes, const uint64_t hash )
{
	//TODO: temporary until we can use our hash lookup api
#ifdef XGL_MESH_VAO_REF_COUNTING
	XglMeshVertexLayout *emptySlot = nullptr;
#endif
	for ( unsigned int i = 0; i < numMeshVertexLayouts; ++i )
	{
#ifdef XGL_MESH_VAO_REF_COUNTING
		if ( emptySlot == nullptr && meshVertexLayouts[ i ].hash == 0 )
		{
			emptySlot = &meshVertexLayouts[ i ];
			continue;
		}
#endif

		if ( hash != meshVertexLayouts[ i ].hash )
		{
			continue;
		}

#ifdef XGL_MESH_VAO_REF_COUNTING
		meshVertexLayouts[ i ].refs++;
#endif
		return i;
	}

	unsigned int vao;
	XGL_CALL( glCreateVertexArrays( 1, &vao ) );
	for ( unsigned int i = 0; i < numAttributes; ++i )
	{
		unsigned int type = xgl_translate_attribute_type( attributes[ i ].type );
		assert( type != XGL_INVALID );
		XGL_CALL( glVertexArrayAttribFormat( vao,
		                                     attributes[ i ].location,
		                                     attributes[ i ].size,
		                                     type,
		                                     // this is a botch, sorry! the reason why there's no API to do this for now is because it seems normalization here
		                                     // is a GLism, so in the long-term it probably doesn't really make sense to keep it...
		                                     !( type == GL_HALF_FLOAT || type == GL_FLOAT || type == GL_DOUBLE ),
		                                     attributes[ i ].offset ) );
		XGL_CALL( glEnableVertexArrayAttrib( vao, attributes[ i ].location ) );
		XGL_CALL( glVertexArrayAttribBinding( vao, attributes[ i ].location, 0 ) );
	}

	// vao doesn't exist in the list, so whip up a new one
	XglMeshVertexLayout *vertexLayout = &meshVertexLayouts[ numMeshVertexLayouts ];
	vertexLayout->vao                 = vao;
	vertexLayout->hash                = hash;

#ifdef XGL_MESH_VAO_REF_COUNTING
	vertexLayout->refs++;
#endif

	if ( numMeshVertexLayouts + 1 >= maxMeshVertexLayouts )
	{
		maxMeshVertexLayouts = numMeshVertexLayouts + 8;
		meshVertexLayouts    = gInterface->core->ReAlloc( meshVertexLayouts, sizeof( XglMeshVertexLayout ) * maxMeshVertexLayouts, true );
	}

	return numMeshVertexLayouts++;
}

#ifdef XGL_MESH_VAO_REF_COUNTING
static void xgl_mesh_vao_destroy( unsigned int vertexLayoutIndex )
{
	if ( vertexLayoutIndex != XGL_INVALID )
	{
		return;
	}

	XglMeshVertexLayout *vertexLayout = &meshVertexLayouts[ vertexLayoutIndex ];

	vertexLayout->refs--;
	if ( vertexLayout->refs == 0 )
	{
		XGL_CALL( glDeleteVertexArrays( 1, &vertexLayout->vao ) );
		vertexLayout->vao  = XGL_INVALID;
		vertexLayout->hash = 0;
	}
}
#endif

void xgl_mesh_vao_manager_initialize()
{
	maxMeshVertexLayouts = 8;
	meshVertexLayouts    = gInterface->core->CAlloc( maxMeshVertexLayouts, sizeof( XglMeshVertexLayout ), true );
}

void xgl_mesh_vao_manager_shutdown()
{
	for ( unsigned int i = 0; i < numMeshVertexLayouts; ++i )
	{
		if ( meshVertexLayouts[ i ].hash == 0 )
		{
			continue;
		}

		XGL_CALL( glDeleteVertexArrays( 1, &meshVertexLayouts[ i ].vao ) );
	}

	gInterface->core->Free( meshVertexLayouts );
}

/////////////////////////////////////////////////////////////
// Mesh

static unsigned int xgl_translate_primitive_mode( QmGfxMeshPrimitive mode )
{
	switch ( mode )
	{
		default:
			return XGL_INVALID;
		case QM_GFX_MESH_PRIMITIVE_LINES:
			return GL_LINES;
		case QM_GFX_MESH_PRIMITIVE_LINE_LOOP:
			return GL_LINE_LOOP;
		case QM_GFX_MESH_PRIMITIVE_POINTS:
			return GL_POINTS;
		case QM_GFX_MESH_PRIMITIVE_TRIANGLES:
			return GL_TRIANGLES;
		case QM_GFX_MESH_PRIMITIVE_TRIANGLE_FAN:
			return GL_TRIANGLE_FAN;
		case QM_GFX_MESH_PRIMITIVE_TRIANGLE_FAN_LINE:
			return GL_LINES;
		case QM_GFX_MESH_PRIMITIVE_TRIANGLE_STRIP:
			return GL_TRIANGLE_STRIP;
	}
}

static unsigned int xgl_translate_draw_mode( QmGfxMeshDrawMode mode )
{
	switch ( mode )
	{
		default:
			return XGL_INVALID;
		case QM_GFX_MESH_DRAW_MODE_DYNAMIC:
			return GL_DYNAMIC_DRAW;
		case QM_GFX_MESH_DRAW_MODE_STATIC:
			return GL_STATIC_DRAW;
		case QM_GFX_MESH_DRAW_MODE_STREAM:
			return GL_STREAM_DRAW;
	}
}

void xgl_mesh_create( QmGfxMesh *self )
{
	XglMesh *drv = gInterface->core->MAlloc( sizeof( XglMesh ), true );

	drv->vertexLayout = XGL_INVALID;

	// Create our internal buffers for GL
	XGL_CALL( glCreateBuffers( XGL_MESH_BUFFER_MAX, drv->buffers ) );

	self->driver = drv;
}

void xgl_mesh_delete( QmGfxMesh *self )
{
	XglMesh *drv = self->driver;
	XGL_CALL( glDeleteBuffers( XGL_MESH_BUFFER_MAX, drv->buffers ) );

#ifdef XGL_MESH_VAO_REF_COUNTING
	xgl_mesh_vao_destroy( drv->vertexLayout );
#endif

	gInterface->core->Free( drv );
	self->driver = nullptr;
}

void xgl_mesh_upload( QmGfxMesh *self, QmGfxShaderProgram *program, const void *vertexPtr, const void *elementsPtr )
{
	if ( !self->isDirty )
	{
		return;
	}

	// check if there's a custom vertex layout specified
	//TODO: this should be done on CREATION not UPLOAD, but we're
	//		botching for now to work around the existing API design
	XglMesh *drv = self->driver;
	if ( drv->vertexLayout == XGL_INVALID )
	{
		drv->vertexLayout = xgl_mesh_vao_get( self->vertexDescriptor.attributes, self->vertexDescriptor.numAttributes, self->vertexDescriptor.attributeTableHash );
	}

	unsigned int drawMode = xgl_translate_draw_mode( self->mode );
	assert( drawMode != XGL_INVALID );

	// Write the current CPU vertex data into the VBO
	XGL_CALL( glNamedBufferData( drv->buffers[ XGL_MESH_BUFFER_VERTEX ], ( GLsizei ) ( self->vertexDescriptor.size * self->num_verts ), vertexPtr, drawMode ) );

	//Point to the different substreams of the interleaved BVO
	//Args: Index, Size, Type, (Normalized), Stride, StartPtr

	if ( drv->buffers[ XGL_MESH_BUFFER_ELEMENT ] != 0 )
	{
		XGL_CALL( glNamedBufferData( drv->buffers[ XGL_MESH_BUFFER_ELEMENT ], sizeof( GLuint ) * self->num_indices, elementsPtr, drawMode ) );
	}

	self->isDirty = false;
}

void xgl_mesh_draw_instanced( QmGfxMesh *self, QmGfxShaderProgram *program, const PLMatrix4 *transforms, unsigned int instanceCount )
{
	XglMesh *drv = self->driver;
	if ( drv->buffers[ XGL_MESH_BUFFER_VERTEX ] == 0 )
	{
		XGL_DEBUG( "invalid buffer provided, skipping draw!\n" );
		return;
	}

	if ( self->primitiveScale != 0.0f )
	{
		if ( self->primitive == QM_GFX_MESH_PRIMITIVE_LINES )
		{
			XGL_CALL( glLineWidth( self->primitiveScale ) );
		}
		else if ( self->primitive == QM_GFX_MESH_PRIMITIVE_POINTS )
		{
			XGL_CALL( glPointSize( self->primitiveScale ) );
		}
	}

	if ( self->num_indices > 0 && drv->buffers[ XGL_MESH_BUFFER_ELEMENT ] != 0 )
	{
		XGL_CALL( glVertexArrayElementBuffer( meshVertexLayouts[ drv->vertexLayout ].vao, drv->buffers[ XGL_MESH_BUFFER_ELEMENT ] ) );
	}

	XGL_CALL( glVertexArrayVertexBuffer( meshVertexLayouts[ drv->vertexLayout ].vao, 0, drv->buffers[ XGL_MESH_BUFFER_VERTEX ], 0, self->vertexDescriptor.size ) );

	//Ensure VAO/VBO/EBO are bound
	XGL_CALL( glBindVertexArray( meshVertexLayouts[ drv->vertexLayout ].vao ) );

	//draw
	GLuint mode = xgl_translate_primitive_mode( self->primitive );
	assert( mode != XGL_INVALID );
	if ( self->num_indices > 0 )
	{
		XGL_CALL( glDrawElementsInstanced( mode, self->num_indices, GL_UNSIGNED_INT, nullptr, instanceCount ) );
	}
	else
	{
		XGL_CALL( glDrawArraysInstanced( mode, 0, self->num_verts, instanceCount ) );
	}

	if ( self->primitiveScale != 0.0f )
	{
		if ( self->primitive == QM_GFX_MESH_PRIMITIVE_LINES )
		{
			XGL_CALL( glLineWidth( 1.0f ) );
		}
		else if ( self->primitive == QM_GFX_MESH_PRIMITIVE_POINTS )
		{
			XGL_CALL( glPointSize( 1.0f ) );
		}
	}
}

void xgl_mesh_draw( QmGfxMesh *self, QmGfxShaderProgram *program )
{
	XglMesh *drv = self->driver;
	if ( drv->buffers[ XGL_MESH_BUFFER_VERTEX ] == 0 )
	{
		XGL_DEBUG( "invalid vertex buffer provided, skipping draw!\n" );
		return;
	}
	if ( self->num_indices > 0 && drv->buffers[ XGL_MESH_BUFFER_ELEMENT ] == 0 )
	{
		XGL_DEBUG( "invalid element buffer provided, skipping draw!\n" );
		return;
	}

#if 1
	// Set up the default uniforms
	unsigned int slot;
	if ( ( slot = ( ( XglShaderProgram * ) program->driver )->defaultUniforms[ XGL_SHADER_UNIFORM_TYPE_CLIP_PLANE ] ) != XGL_INVALID )
	{
		const QmMathVector4f *clipPlane = xgl_get_clip_plane();
		XGL_CALL( glUniform4fv( slot, 1, ( float * ) &clipPlane ) );
	}
	if ( ( slot = ( ( XglShaderProgram * ) program->driver )->defaultUniforms[ XGL_SHADER_UNIFORM_TYPE_CLIP_PLANE_MATRIX ] ) != XGL_INVALID )
	{
		const PLMatrix4 *clipMatrix = xgl_get_clip_matrix();
		XGL_CALL( glUniformMatrix4fv( slot, 1, GL_FALSE, clipMatrix->m ) );
	}
#endif

	if ( self->primitiveScale != 0.0f )
	{
		if ( self->primitive == QM_GFX_MESH_PRIMITIVE_LINES )
		{
			XGL_CALL( glLineWidth( self->primitiveScale ) );
		}
		else if ( self->primitive == QM_GFX_MESH_PRIMITIVE_POINTS )
		{
			XGL_CALL( glPointSize( self->primitiveScale ) );
		}
	}

	if ( self->num_indices > 0 && drv->buffers[ XGL_MESH_BUFFER_ELEMENT ] != 0 )
	{
		XGL_CALL( glVertexArrayElementBuffer( meshVertexLayouts[ drv->vertexLayout ].vao, drv->buffers[ XGL_MESH_BUFFER_ELEMENT ] ) );
	}

	XGL_CALL( glVertexArrayVertexBuffer( meshVertexLayouts[ drv->vertexLayout ].vao, 0, drv->buffers[ XGL_MESH_BUFFER_VERTEX ], 0, self->vertexDescriptor.size ) );

	XGL_CALL( glBindVertexArray( meshVertexLayouts[ drv->vertexLayout ].vao ) );

	//draw
	GLuint mode = xgl_translate_primitive_mode( self->primitive );
	assert( mode != XGL_INVALID );
	if ( self->num_indices > 0 )
	{
		if ( self->numSubMeshes > 0 )
		{
			XGL_CALL( glMultiDrawElements( mode, self->subMeshes, GL_UNSIGNED_INT, nullptr, self->numSubMeshes ) );
		}
		else
		{
			XGL_CALL( glDrawElements( mode, self->range, GL_UNSIGNED_INT, ( void * ) ( self->start * sizeof( GLuint ) ) ) );
		}
	}
	else
	{
		if ( self->numSubMeshes > 0 )
		{
			XGL_CALL( glMultiDrawArrays( mode, self->firstSubMeshes, self->subMeshes, self->numSubMeshes ) );
		}
		else
		{
			XGL_CALL( glDrawArrays( mode, self->start, self->num_verts ) );
		}
	}

	if ( self->primitiveScale != 0.0f )
	{
		if ( self->primitive == QM_GFX_MESH_PRIMITIVE_LINES )
		{
			XGL_CALL( glLineWidth( 1.0f ) );
		}
		else if ( self->primitive == QM_GFX_MESH_PRIMITIVE_POINTS )
		{
			XGL_CALL( glPointSize( 1.0f ) );
		}
	}
}
