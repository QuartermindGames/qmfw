// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_math.h>

typedef int          QmGfxResult;
static constexpr int QM_GFX_RESULT_SUCCESS = 0;

#include <plgraphics/plg_mesh.h>
#include <plgraphics/plg_texture.h>
#include <plgraphics/plg_framebuffer.h>

typedef struct QmGfxDriverImportTable QmGfxDriverImportTable;

typedef enum PLGCullMode
{
	PLG_CULL_NONE, /* disables backface culling */
	PLG_CULL_POSITIVE,
	PLG_CULL_NEGATIVE
} PLGCullMode;

// Blending Modes
typedef enum PLGBlend
{
	PLG_BLEND_NONE,// disables blending

	PLG_BLEND_ZERO,
	PLG_BLEND_ONE,
	PLG_BLEND_SRC_COLOR,
	PLG_BLEND_ONE_MINUS_SRC_COLOR,
	PLG_BLEND_SRC_ALPHA,
	PLG_BLEND_ONE_MINUS_SRC_ALPHA,
	PLG_BLEND_DST_ALPHA,
	PLG_BLEND_ONE_MINUS_DST_ALPHA,
	PLG_BLEND_DST_COLOR,
	PLG_BLEND_ONE_MINUS_DST_COLOR,
	PLG_BLEND_SRC_ALPHA_SATURATE,

	PLG_MAX_BLEND_MODES
} PLGBlend;
#define PLG_BLEND_DISABLE  PLG_BLEND_NONE, PLG_BLEND_NONE
#define PLG_BLEND_ADDITIVE PLG_BLEND_SRC_ALPHA, PLG_BLEND_ONE
#define PLG_BLEND_DEFAULT  PLG_BLEND_SRC_ALPHA, PLG_BLEND_ONE_MINUS_SRC_ALPHA

//-----------------
// Capabilities

/* Replace these with generic functions, rather than relying on just flags?
 * plEnableFog(bool active)
 * plEnableAlphaTest(bool active)
 * plEnableBlend(bool active)
 */

typedef enum PLGDrawState
{
	PLG_GFX_STATE_FOG,            // Fog.
	PLG_GFX_STATE_ALPHATEST,      // Alpha-testing.
	PLG_GFX_STATE_BLEND,          // Blending.
	PLG_GFX_STATE_DEPTHTEST,      // Depth-testing.
	PLG_GFX_STATE_STENCILTEST,    // Stencil-testing.
	PLG_GFX_STATE_MULTISAMPLE,    // Multisampling.
	PLG_GFX_STATE_ALPHATOCOVERAGE,// Alpha to Coverage
	PLG_GFX_STATE_DEPTH_CLAMP,
	PLG_GFX_STATE_WIREFRAME,

	PLG_GFX_MAX_STATES
} PLGDrawState;

//-----------------
// Framebuffers
// todo: move all of this into pl_graphics_framebuffer.h

typedef enum PLGStencilOp
{
	PLG_STENCIL_OP_KEEP,
	PLG_STENCIL_OP_ZERO,
	PLG_STENCIL_OP_REPLACE,
	PLG_STENCIL_OP_INCR,
	PLG_STENCIL_OP_INCRWRAP,
	PLG_STENCIL_OP_DECR,
	PLG_STENCIL_OP_DECRWRAP,
	PLG_STENCIL_OP_INVERT,

	PLG_MAX_STENCIL_OPS
} PLGStencilOp;

typedef enum PLGStencilFace
{
	PLG_STENCIL_FACE_FRONT,
	PLG_STENCIL_FACE_BACK,
	PLG_STENCIL_FACE_FRONTANDBACK,
} PLGStencilFace;

typedef struct QmGfxMeshVertex QmGfxMeshVertex;
typedef struct QmGfxMesh       QmGfxMesh;

typedef enum PLGCompareFunction
{
	PLG_COMPARE_NEVER,
	PLG_COMPARE_LESS,
	PLG_COMPARE_EQUAL,
	PLG_COMPARE_LEQUAL,
	PLG_COMPARE_GREATER,
	PLG_COMPARE_NOTEQUAL,
	PLG_COMPARE_GEQUAL,
	PLG_COMPARE_ALWAYS,

	PLG_MAX_COMPARE_FUNCTIONS
} PLGCompareFunction;

PL_EXTERN_C

#if !defined( PL_COMPILE_PLUGIN )

PLFunctionResult qm_gfx_initialize( void );
void             PlgShutdownGraphics( void );

void qm_gfx_set_clip_plane( const QmMathVector4f *clip, const PLMatrix4 *clipMatrix, bool transpose );

#endif

/////////////////////////////////////////////////////////////////////////////////////
// Shaders
/////////////////////////////////////////////////////////////////////////////////////

typedef int PLGShaderAttribute;

typedef enum QmGfxShaderStageType : uint8_t
{
	QM_GFX_SHADER_STAGE_TYPE_VERTEX,
	QM_GFX_SHADER_STAGE_TYPE_FRAGMENT,
	QM_GFX_SHADER_STAGE_TYPE_COMPUTE,

	QM_GFX_MAX_SHADER_STAGE_TYPES
} QmGfxShaderStageType;

typedef enum QmGfxShaderUniformType : uint8_t
{
	QM_GFX_SHADER_UNIFORM_TYPE_INVALID,

	QM_GFX_SHADER_UNIFORM_TYPE_FLOAT,
	QM_GFX_SHADER_UNIFORM_TYPE_INT,
	QM_GFX_SHADER_UNIFORM_TYPE_UINT,
	QM_GFX_SHADER_UNIFORM_TYPE_BOOL,
	QM_GFX_SHADER_UNIFORM_TYPE_DOUBLE,

	/* textures */
	QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER1D,
	QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER2D,
	QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER3D,
	QM_GFX_SHADER_UNIFORM_TYPE_SAMPLERCUBE,
	QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER1DSHADOW,
	QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER2DSHADOW,

	/* vectors */
	QM_GFX_SHADER_UNIFORM_TYPE_VEC2,
	QM_GFX_SHADER_UNIFORM_TYPE_VEC3,
	QM_GFX_SHADER_UNIFORM_TYPE_VEC4,

	/* matrices */
	QM_GFX_SHADER_UNIFORM_TYPE_MAT3,
	QM_GFX_SHADER_UNIFORM_TYPE_MAT4,

	QM_GFX_MAX_SHADER_UNIFORM_TYPES
} QmGfxShaderUniformType;

typedef struct QmGfxShaderProgram QmGfxShaderProgram;
typedef struct QmGfxShaderStage   QmGfxShaderStage;

#define PLG_MAX_DEFINITION_LENGTH 16
#define PLG_MAX_DEFINITIONS       16

#define PLG_MAX_UNIFORM_NAME_LENGTH 128

typedef struct QmGfxShaderStage
{
	QmGfxShaderStageType type;

	/* software implementation of the shader stage */
	void ( *SWFallback )( QmGfxShaderProgram *program, QmGfxShaderStageType type );

	QmGfxShaderProgram *program;

	struct
	{
		unsigned int id;
	} internal;

	unsigned int numDefinitions;
	char         definitions[ PLG_MAX_DEFINITIONS ][ PLG_MAX_DEFINITION_LENGTH ];

	PLPath path; /* original location it was loaded from */
} QmGfxShaderStage;

typedef struct QmGfxShaderProgram
{
	struct
	{
		char                   name[ PLG_MAX_UNIFORM_NAME_LENGTH ];
		int                    slot;
		unsigned int           numElements;
		QmGfxShaderUniformType type;

		union
		{
			int          defaultInt;
			unsigned int defaultUInt;
			bool         defaultBool;
			double       defaultDouble;
			float        defaultFloat;

			QmMathVector2f defaultVec2;
			QmMathVector3f defaultVec3;
			QmMathVector4f defaultVec4;

			PLMatrix3 defaultMat3;
			PLMatrix4 defaultMat4;
		};
	}           *uniforms;
	unsigned int numUniforms;

	QmGfxShaderStage *stages[ QM_GFX_MAX_SHADER_STAGE_TYPES ];
	unsigned int      num_stages;

	bool is_linked;

	void *driver;// driver specific data
} QmGfxShaderProgram;

#if !defined( PL_COMPILE_PLUGIN )

/**
 * Create the shader stage and generate it on the GPU, if applicable.
 * Call memory_free to destroy.
 *
 * @param type the type of shader stage.
 * @return the new shader stage.
 */
QmGfxShaderStage *qm_gfx_shader_stage_create( QmGfxShaderStageType type );

/**
 * Compiles the given shader stage on the GPU, otherwise it
 * will be ignored when performing software-rendering or if
 * your GPU doesn't support the shader compilation.
 *
 * If the compilation fails an error will be reported and this will
 * automatically fallback when rendering anything if it's active.
 *
 * @param self the stage we're going to be compiling.
 * @param buf pointer to buffer containing the shader we're compiling.
 * @param length the length of the buffer.
 */
bool qm_gfx_shader_stage_compile( QmGfxShaderStage *self, const char *buf, size_t length, const char *localDirectory );

/**
 * Sets out what definitions should be applied when compiling the shader stage.
 */
void qm_gfx_shader_stage_set_definitions( QmGfxShaderStage *self, const char definitions[][ PLG_MAX_DEFINITION_LENGTH ], unsigned int numDefinitions );

void qm_gfx_shader_program_attach_stage( QmGfxShaderProgram *self, QmGfxShaderStage *stage );
bool qm_gfx_shader_program_link( QmGfxShaderProgram *self );

/**
 * Searches through programs registered uniforms
 * for the specified uniform entry.
 *
 * If it fails to find the uniform it'll return '-1'.
 */
int qm_gfx_shader_program_get_uniform_slot( QmGfxShaderProgram *self, const char *name );

QmGfxShaderUniformType qm_gfx_shader_program_get_uniform_type( const QmGfxShaderProgram *self, int slot );
unsigned int           qm_gfx_shader_program_get_num_uniform_elements( const QmGfxShaderProgram *self, int slot );

void qm_gfx_shader_program_set_uniform_default( QmGfxShaderProgram *self, int slot, const void *defaultValue );
void qm_gfx_shader_program_set_uniform_to_default( QmGfxShaderProgram *self, int slot );
void qm_gfx_shader_program_set_uniform( QmGfxShaderProgram *self, int slot, const void *value, bool transpose );

/**
 * allocates a new shader program in memory and generates it on the GPU, if applicable.
 * Call memory_free to destroy.
 *
 * @return the new shader program.
 */
QmGfxShaderProgram *qm_gfx_shader_program_create();

/**
 * returns the currently active shader program.
 *
 * @return pointer to the currently active shader program.
 */
QmGfxShaderProgram *PlgGetCurrentShaderProgram( void );

/**
 * sets the currently active shader program. means that any
 * subsequent draw calls will use this shader program.
 *
 * @param program
 */
void PlgSetShaderProgram( QmGfxShaderProgram *program );

/**
 * checks whether the given shader program is currently
 * enabled - meaning it will be used in any subsequent draw
 * calls.
 *
 * @param program
 * @return true if the program is equal to that currently enabled.
 */
bool PlgIsShaderProgramEnabled( QmGfxShaderProgram *program );

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

// Debugging
void qm_gfx_debug_insert_marker( const char *msg );
void qm_gfx_debug_push_group_marker( const char *msg );
void qm_gfx_debug_pop_group_marker( void );

// Hardware Information
const char *PlgGetHWExtensions( void );
const char *PlgGetHWRenderer( void );
const char *PlgGetHWVendor( void );
const char *PlgGetHWVersion( void );

void PlgSetCullMode( PLGCullMode mode );
void PlgSetBlendMode( PLGBlend a, PLGBlend b );

void PlgSetDepthBufferMode( unsigned int mode );
void PlgDepthMask( bool enable );

/* stencil operations */

void PlgDepthBufferFunction( PLGCompareFunction compareFunction );
void PlgStencilBufferFunction( PLGCompareFunction function, int reference, unsigned int mask );

void PlgStencilOp( PLGStencilFace face, PLGStencilOp stencilFailOp, PLGStencilOp depthFailOp, PLGStencilOp depthPassOp );

bool PlgIsGraphicsStateEnabled( PLGDrawState state );

void PlgEnableGraphicsState( PLGDrawState state );
void PlgDisableGraphicsState( PLGDrawState state );

/////////////////////////////////////////////////////////////////////////////////////
// Viewport
/////////////////////////////////////////////////////////////////////////////////////

void qm_gfx_clip_viewport( int x, int y, int width, int height );
void qm_gfx_set_viewport( int x, int y, int width, int height );
void qm_gfx_get_viewport( int *x, int *y, int *width, int *height );

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/***** TEMPORARY CRAP START *****/

void      PlgSetViewMatrix( const PLMatrix4 *viewMatrix );
PLMatrix4 PlgGetViewMatrix( void );
void      PlgSetProjectionMatrix( const PLMatrix4 *projMatrix );
PLMatrix4 PlgGetProjectionMatrix( void );

/***** TEMPORARY CRAP END 	*****/

#endif

PL_EXTERN_C_END
