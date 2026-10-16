// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#define PL_COMPILE_PLUGIN 1
#include <plgraphics/qm_gfx_driver.h>
extern const PLGDriverExportTable *gInterface;

#include <GL/glew.h>
#if defined( _WIN32 )
#	include <GL/wglew.h>
#else
#	include <GL/glxew.h>
#endif

#if !defined( NDEBUG )
#	define XGL_DEBUG( ... ) printf( __VA_ARGS__ )
#else
#	define XGL_DEBUG( ... )
#endif

#if !defined( NDEBUG )
#	define XGL_CALL( X )                     \
		{                                     \
			glGetError();                     \
			X;                                \
			unsigned int _err = glGetError(); \
			assert( _err == GL_NO_ERROR );    \
		}
#else
#	define XGL_CALL( X ) X
#endif

static constexpr unsigned int XGL_INVALID = ( unsigned int ) -1;

const QmMathVector4f *xgl_get_clip_plane();
const PLMatrix4      *xgl_get_clip_matrix();

typedef struct XglTexture
{
	unsigned int id;
	unsigned int target;
	unsigned int faceIndex;// for cubemaps

	bool hasStorage;
} XglTexture;

typedef enum XglShaderUniformType : unsigned int
{
	XGL_SHADER_UNIFORM_TYPE_MODEL_MATRIX,
	XGL_SHADER_UNIFORM_TYPE_VIEW_MATRIX,
	XGL_SHADER_UNIFORM_TYPE_PROJECTION_MATRIX,
	XGL_SHADER_UNIFORM_TYPE_TEXTURE_MATRIX,
	XGL_SHADER_UNIFORM_TYPE_CLIP_PLANE,
	XGL_SHADER_UNIFORM_TYPE_CLIP_PLANE_MATRIX,
	XGL_SHADER_UNIFORM_TYPE_MAX
} XglShaderUniformType;

typedef enum XglShaderAttributeType : unsigned int
{
	XGL_SHADER_ATTRIBUTE_TYPE_POSITION,
	XGL_SHADER_ATTRIBUTE_TYPE_NORMAL,
	XGL_SHADER_ATTRIBUTE_TYPE_COLOUR,
	XGL_SHADER_ATTRIBUTE_TYPE_TANGENT,
	XGL_SHADER_ATTRIBUTE_TYPE_BITANGENT,
	XGL_SHADER_ATTRIBUTE_TYPE_UV0,
	XGL_SHADER_ATTRIBUTE_TYPE_UV1,
	XGL_SHADER_ATTRIBUTE_TYPE_UV2,
	XGL_SHADER_ATTRIBUTE_TYPE_UV3,

	XGL_SHADER_ATTRIBUTE_TYPE_MAX
} XglShaderAttributeType;

typedef struct XglShaderProgram
{
	unsigned int id;

	XglShaderUniformType defaultUniforms[ XGL_SHADER_UNIFORM_TYPE_MAX ];
} XglShaderProgram;

/////////////////////////////////////////////////////////////
// Mesh

enum
{
	XGL_MESH_BUFFER_VERTEX = 0,
	XGL_MESH_BUFFER_ELEMENT,

	XGL_MESH_BUFFER_MAX
};

void xgl_mesh_vao_manager_initialize();
void xgl_mesh_vao_manager_shutdown();

void xgl_mesh_create( QmGfxMesh *self );
void xgl_mesh_upload( QmGfxMesh *self, QmGfxShaderProgram *program, const void *vertexPtr, unsigned int numVertices, const void *elementsPtr, unsigned int numElements );
void xgl_mesh_delete( QmGfxMesh *self );
void xgl_mesh_draw_instanced( QmGfxMesh *self, QmGfxShaderProgram *program, const PLMatrix4 *transforms, unsigned int instanceCount );
void xgl_mesh_draw( QmGfxMesh *self, QmGfxShaderProgram *program, unsigned int start, unsigned int range );

/////////////////////////////////////////////////////////////
