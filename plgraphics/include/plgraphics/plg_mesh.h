// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include <plcore/pl_math.h>
#include <plcore/pl_physics.h>

PL_EXTERN_C

typedef struct QmGfxMesh QmGfxMesh;

/////////////////////////////////////////////////////////////////////////////////////
// Vertex Attribute Declarations
// Much of the below is pretty gross and limiting, I know! For the moment, it's just
// to provide the basics and we'll expand on this further down the line so the caller
// can actually set up their own instead rather than using our built-in types.
/////////////////////////////////////////////////////////////////////////////////////

typedef enum QmGfxMeshVertexAttributeType : uint8_t
{
	QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_INT8,
	QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_INT16,
	QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_INT32,
	QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_UINT8,
	QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_UINT16,
	QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_UINT32,
	QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_FLOAT16,
	QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_FLOAT32,
	QM_GFX_MESH_VERTEX_ATTRIBUTE_TYPE_FLOAT64,

	QM_GFX_MESH_VERTEX_ATTRIBUTE_DATA_TYPE_MAX
} QmGfxMeshVertexAttributeType;

typedef struct QmGfxMeshVertexAttribute
{
	unsigned int                 location;
	size_t                       size;
	QmGfxMeshVertexAttributeType type;
	unsigned int                 offset;
} QmGfxMeshVertexAttribute;

static constexpr unsigned int QM_GFX_MESH_MAX_ATTRIBUTES = 16;

typedef struct QmGfxMeshVertexDescriptor
{
	uint64_t                 attributeTableHash;
	QmGfxMeshVertexAttribute attributes[ QM_GFX_MESH_MAX_ATTRIBUTES ];
	unsigned int             numAttributes;
	size_t                   size;
} QmGfxMeshVertexDescriptor;

#if !defined( PL_COMPILE_PLUGIN )

/**
 * Sets up the vertex layout for the given mesh.
 * -1 : Unsupported number of attributes.
 */
QmGfxResult qm_gfx_mesh_set_vertex_layout( QmGfxMesh *mesh, const QmGfxMeshVertexAttribute *attributes, unsigned int numAttributes, size_t size );

#endif

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

typedef enum QmGfxMeshPrimitive
{
	QM_GFX_MESH_PRIMITIVE_LINES,
	QM_GFX_MESH_PRIMITIVE_LINE_LOOP,
	QM_GFX_MESH_PRIMITIVE_POINTS,
	QM_GFX_MESH_PRIMITIVE_TRIANGLES,
	QM_GFX_MESH_PRIMITIVE_TRIANGLE_STRIP,
	QM_GFX_MESH_PRIMITIVE_TRIANGLE_FAN,
	QM_GFX_MESH_PRIMITIVE_TRIANGLE_FAN_LINE,

	QM_GFX_MESH_PRIMITIVE_MAX
} QmGfxMeshPrimitive;

/* If these confuse you, like they do me, there's a really good write-up here.
 * https://www.reddit.com/r/opengl/comments/57i9cl/comment/d8s8wnq/
 * tldr; stream for update every frame, dynamic for update occasionally and static for no updates.
 */
typedef enum QmGfxMeshDrawMode
{
	QM_GFX_MESH_DRAW_MODE_STREAM,
	QM_GFX_MESH_DRAW_MODE_STATIC,
	QM_GFX_MESH_DRAW_MODE_DYNAMIC,
} QmGfxMeshDrawMode;

typedef struct QmGfxMeshVertex
{
	QmMathVector3f  position, normal;
	QmMathColour4ub colour;
	QmMathVector3f  tangent, bitangent;
	QmMathVector2f  st[ 4 ];
} QmGfxMeshVertex;

typedef struct QmGfxMesh
{
	QmGfxMeshVertex *vertices;
	unsigned int     num_verts;
	unsigned int     maxVertices;

	uint32_t numSubMeshes;
	uint32_t maxSubMeshes;
	int32_t *subMeshes;
	int32_t *firstSubMeshes;

	unsigned int *indices;
	unsigned int  num_indices;
	unsigned int  maxIndices;
	unsigned int  start;
	unsigned int  range;

	unsigned int num_triangles;

	QmGfxMeshPrimitive primitive;
	QmGfxMeshDrawMode  mode;

	bool isDirty;

	float primitiveScale; /* only matters for points/lines */

	QmGfxMeshVertexDescriptor vertexDescriptor;

	void *driver;
} QmGfxMesh;

#if !defined( PL_COMPILE_PLUGIN )

QmGfxMesh *qm_gfx_mesh_create( QmGfxMeshPrimitive primitive, QmGfxMeshDrawMode mode, unsigned int numTriangles, unsigned int numVertices );
void       qm_gfx_mesh_destroy( QmGfxMesh *mesh );

[[deprecated]] void PlgDrawRectangle( float x, float y, float w, float h, QmMathColour4ub colour );
[[deprecated]] void PlgDrawLineRectangle( float x, float y, float w, float h, QmMathColour4ub colour );
[[deprecated]] void PlgDrawLines( const QmMathVector3f *points, unsigned int numPoints, QmMathColour4ub colour, float thickness );
[[deprecated]] void PlgDrawLine( QmMathVector3f startPos, QmMathColour4ub startColour, QmMathVector3f endPos, QmMathColour4ub endColour );
[[deprecated]] void PlgDrawSimpleLine( QmMathVector3f startPos, QmMathVector3f endPos, QmMathColour4ub colour );

[[deprecated]] void PlgClearMesh( QmGfxMesh *mesh );

void qm_gfx_mesh_set_primitive_scale( QmGfxMesh *mesh, float scale );

[[deprecated]] unsigned int PlgAddMeshVertex( QmGfxMesh *mesh, const QmMathVector3f *position, const QmMathVector3f *normal, const QmMathColour4ub *colour, const QmMathVector2f *st );
[[deprecated]] unsigned int PlgAddMeshTriangle( QmGfxMesh *mesh, unsigned int x, unsigned int y, unsigned int z );

void qm_gfx_mesh_upload( QmGfxMesh *mesh, const void *vertexPtr, const void *elementsPtr );

void qm_gfx_mesh_draw( QmGfxMesh *mesh );
void qm_gfx_mesh_draw_sub( QmGfxMesh *mesh, int32_t *firstSubMeshes, int32_t *subMeshes, uint32_t numSubMeshes );
void qm_gfx_mesh_draw_instanced( QmGfxMesh *mesh, const PLMatrix4 *transforms, unsigned int instanceCount );

/**
 * Generate cubic coordinates for the given vertices.
 */
void PlgGenerateTextureCoordinates( QmGfxMeshVertex *vertices, unsigned int numVertices, QmMathVector2f textureOffset, QmMathVector2f textureScale );

/* immediate mode style api */
//TODO: move into engine
QmGfxMesh   *PlgImmBegin( QmGfxMeshPrimitive primitive );
unsigned int PlgImmPushVertex( float x, float y, float z );
void         PlgImmNormal( float x, float y, float z );
void         PlgImmColour( uint8_t r, uint8_t g, uint8_t b, uint8_t a );
void         PlgImmTextureCoord( float s, float t );
unsigned int PlgImmPushTriangle( unsigned int x, unsigned int y, unsigned int z );
void         PlgImmSetPrimitiveScale( float scale );
void         PlgImmDraw();

[[deprecated]] unsigned int PlgPushTriangle( QmGfxMesh *mesh, unsigned int x, unsigned int y, unsigned int z );
[[deprecated]] unsigned int PlgPushVertex3f( QmGfxMesh *mesh, float x, float y, float z );
[[deprecated]] void         PlgColour4bv( QmGfxMesh *mesh, const QmMathColour4ub *col );

#endif

PL_EXTERN_C_END
