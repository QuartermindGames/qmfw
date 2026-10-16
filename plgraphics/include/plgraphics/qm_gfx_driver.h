/**
 * Hei Platform Library
 * Copyright (C) 2017-2023 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

#include <plcore/pl_plugin_interface.h>
#include <plgraphics/plg.h>

PL_EXTERN_C

typedef struct QmGfxShaderStage   QmGfxShaderStage;
typedef struct QmGfxShaderProgram QmGfxShaderProgram;

typedef enum QmGfxDriverInterface
{
	QM_GFX_DRIVER_INTERFACE_OPENGL,

	QM_GFX_DRIVER_INTERFACE_MAX
} QmGfxDriverInterface;

/**
 * For a little further clarity, technically
 * speaking 'drivers' in this context are handled
 * exactly the same as plugins for the most part
 * but are setup through the plgraphics module
 * instead - and are exclusively assumed to support
 * either audio/video.
 */

#define PLG_INTERFACE_VERSION_MAJOR 9
#define PLG_INTERFACE_VERSION_MINOR 0
#define PLG_INTERFACE_VERSION \
	( uint16_t[ 2 ] ) { PLG_INTERFACE_VERSION_MAJOR, PLG_INTERFACE_VERSION_MINOR }

typedef struct PLGDriverDescription
{
	uint16_t coreInterfaceVersion[ 2 ];
	uint16_t graphicsInterfaceVersion[ 2 ];

	const char  *identifier;         /* 'opengl3' */
	unsigned int driverVersion[ 3 ]; /* major, minor and patch */
	const char  *description;        /* 'OpenGL 3.3 Graphics Driver' */
} PLGDriverDescription;

typedef struct PLGDriverExportTable
{
	const PLPluginExportTable *core;

	/* plg_texture */
	QmGfxTexture *( *CreateTexture )( QmGfxTextureType type );
} PLGDriverExportTable;

typedef struct QmGfxDriverImportTable
{
	PLFunctionResult ( *Initialize )( void );
	void ( *Shutdown )( void );

	// Debug
	void ( *InsertDebugMarker )( const char *msg );
	void ( *PushDebugGroupMarker )( const char *msg );
	void ( *PopDebugGroupMarker )( void );

	/* hw information */

	void ( *GetMaxTextureUnits )( unsigned int *num_units );
	void ( *GetMaxTextureSize )( unsigned int *s );

	/******************************************/

	/* generic state management */
	void ( *EnableState )( PLGDrawState state );
	void ( *DisableState )( PLGDrawState state );

	void ( *SetBlendMode )( PLGBlend a, PLGBlend b );
	void ( *SetCullMode )( PLGCullMode mode );

	void ( *SetClearColour )( QmMathColour4ub rgba );
	void ( *ClearBuffers )( unsigned int buffers );

	void ( *DrawPixel )( int x, int y, QmMathColour4ub colour );

	void ( *SetDepthBufferMode )( unsigned int mode );

	void ( *DepthMask )( bool enable );
	void ( *ColourMask )( bool r, bool g, bool b, bool a );
	void ( *StencilMask )( unsigned int mask );

	// Mesh
	void ( *CreateMesh )( QmGfxMesh *mesh );
	void ( *UploadMesh )( QmGfxMesh *mesh, QmGfxShaderProgram *program, const void *vertexPtr, unsigned int numVertices, const void *elementsPtr, unsigned int numElements );
	void ( *DrawMesh )( QmGfxMesh *mesh, QmGfxShaderProgram *program, unsigned int start, unsigned int range );
	void ( *DrawInstancedMesh )( QmGfxMesh *mesh, QmGfxShaderProgram *program, const PLMatrix4 *transforms, unsigned int instanceCount );
	void ( *DeleteMesh )( QmGfxMesh *mesh );

	// Framebuffer
	bool ( *CreateFrameBuffer )( QmGfxFramebuffer *buffer );
	void ( *DeleteFrameBuffer )( QmGfxFramebuffer *buffer );
	void ( *BindFrameBuffer )( QmGfxFramebuffer *buffer, PLGFrameBufferObjectTarget targetBinding );
	QmGfxTexture *( *GetFrameBufferTextureAttachment )( QmGfxFramebuffer *buffer, unsigned int component, QmGfxTextureFilter filter, QmGfxTextureWrapMode wrap );
	void ( *BlitFrameBuffers )( QmGfxFramebuffer *srcBuffer,
	                            unsigned int      srcW,
	                            unsigned int      srcH,
	                            QmGfxFramebuffer *dstBuffer,
	                            unsigned int      dstW,
	                            unsigned int      dstH,
	                            unsigned int      mask,
	                            bool              linear );
	void ( *SetFrameBufferSize )( QmGfxFramebuffer *frameBuffer, unsigned int width, unsigned int height );

	// Texture
	void ( *CreateTexture )( QmGfxTexture *texture );
	void ( *DeleteTexture )( QmGfxTexture *texture );
	void ( *BindTexture )( const QmGfxTexture *texture, unsigned int unit );
	void ( *UploadTexture )( QmGfxTexture *texture, const QmImage *upload );
	void ( *SetTextureAnisotropy )( QmGfxTexture *texture, uint32_t value );
	void ( *SetTextureFilter )( QmGfxTexture *texture, QmGfxTextureFilter filter );

	/* viewport */
	void ( *ClipViewport )( int x, int y, int width, int height );
	void ( *SetViewport )( int x, int y, int width, int height );

	///////////////////////////////////////////

	// Shaders
	/* todo: simplify this interface */
	void ( *CreateShaderProgram )( QmGfxShaderProgram *program );
	void ( *DestroyShaderProgram )( QmGfxShaderProgram *program );
	void ( *AttachShaderStage )( QmGfxShaderProgram *program, QmGfxShaderStage *stage );
	void ( *DetachShaderStage )( QmGfxShaderProgram *program, QmGfxShaderStage *stage );
	void ( *LinkShaderProgram )( QmGfxShaderProgram *program );
	void ( *SetShaderProgram )( QmGfxShaderProgram *program );
	void ( *CreateShaderStage )( QmGfxShaderStage *stage );
	void ( *DestroyShaderStage )( QmGfxShaderStage *stage );
	bool ( *CompileShaderStage )( QmGfxShaderStage *stage, const char *buf, size_t length, const char *directory );
	void ( *SetShaderUniformValue )( QmGfxShaderProgram *program, int slot, const void *value, bool transpose );

	// Stencil operations
	void ( *StencilBufferFunction )( PLGCompareFunction compareFunction, int reference, unsigned int mask );
	void ( *DepthBufferFunction )( PLGCompareFunction compareFunction );

	void ( *StencilOp )( PLGStencilFace face, PLGStencilOp stencilFailOp, PLGStencilOp depthFailOp, PLGStencilOp depthPassOp );

	// v3.1
	void *( *ReadFrameBufferRegion )( QmGfxFramebuffer *frameBuffer, uint32_t x, uint32_t y, uint32_t w, uint32_t h, size_t dstSize, void *dstBuf );

	// v3.2
	void ( *SetTextureWrapMode )( QmGfxTexture *texture, QmGfxTextureWrapMode wrapMode );

	void ( *SetClipPlane )( const QmMathVector4f *clipPlane, const PLMatrix4 *clipPlaneMatrix, bool transpose );
} QmGfxDriverImportTable;

#if !defined( PL_COMPILE_PLUGIN )
#	define PLG_DRIVER_QUERY_FUNCTION "QueryGraphicsDriver"
typedef const PLGDriverDescription *( *PLGDriverQueryFunction )( void );
#	define PLG_DRIVER_INIT_FUNCTION "InitializeGraphicsDriver"
typedef const QmGfxDriverImportTable *( *PLGDriverInitializationFunction )( const PLGDriverExportTable *exportTable );

PL_EXTERN PLFunctionResult qm_gfx_driver_set( unsigned int mode );
#endif

PL_EXTERN_C_END
