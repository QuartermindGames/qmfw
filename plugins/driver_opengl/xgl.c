// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: OpenGL driver implementation.
// Author:  Mark E. Sowden

#include "plugin.h"

#include "qmos/public/qm_os.h"
#include "qmparse/public/qm_parse.h"

#include <plgraphics/plg.h>

#if !defined( NDEBUG )
#	define DEBUG_GL
#endif

static int xgl_versionMajor = 0;
static int xgl_versionMinor = 0;

#define XGL_VERSION( maj, min ) ( ( ( maj ) == xgl_versionMajor && ( min ) <= xgl_versionMinor ) || ( maj ) < xgl_versionMajor )

///////////////////////////////////////////
// Debug

static void xgl_debug_insert_marker( const char *msg )
{
	if ( !XGL_VERSION( 4, 3 ) )
	{
		return;
	}

	XGL_CALL( glDebugMessageInsert( GL_DEBUG_SOURCE_APPLICATION,
	                                GL_DEBUG_TYPE_MARKER,
	                                0,
	                                GL_DEBUG_SEVERITY_NOTIFICATION,
	                                -1,
	                                msg ) );
}

static void xgl_debug_push_group_marker( const char *msg )
{
	if ( !XGL_VERSION( 4, 3 ) )
	{
		return;
	}

	XGL_CALL( glPushDebugGroup( GL_DEBUG_SOURCE_APPLICATION, 0, -1, msg ) );
}

static void xgl_debug_pop_group_marker( void )
{
	if ( !XGL_VERSION( 4, 3 ) )
	{
		return;
	}

	XGL_CALL( glPopDebugGroup() );
}

static void xgl_translate_texture_filter_format( QmGfxTextureFilter filterMode, int *min, int *mag )
{
	switch ( filterMode )
	{
		case PLG_TEXTURE_FILTER_LINEAR:
			*min = *mag = GL_LINEAR;
			break;
		default:
		case PLG_TEXTURE_FILTER_NEAREST:
			*min = *mag = GL_NEAREST;
			break;
		case PLG_TEXTURE_FILTER_MIPMAP_LINEAR:
			*min = GL_LINEAR_MIPMAP_LINEAR;
			*mag = GL_LINEAR;
			break;
		case PLG_TEXTURE_FILTER_MIPMAP_LINEAR_NEAREST:
			*min = GL_LINEAR_MIPMAP_NEAREST;
			*mag = GL_LINEAR;
			break;
		case PLG_TEXTURE_FILTER_MIPMAP_NEAREST:
			*min = GL_NEAREST_MIPMAP_NEAREST;
			*mag = GL_NEAREST;
			break;
		case PLG_TEXTURE_FILTER_MIPMAP_NEAREST_LINEAR:
			*min = GL_NEAREST_MIPMAP_LINEAR;
			*mag = GL_NEAREST;
			break;
	}
}

/////////////////////////////////////////////////////////////

static void xgl_get_max_texture_units( unsigned int *num_units )
{
	XGL_CALL( glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS, ( GLint * ) num_units ) );
}

static void xgl_get_max_texture_size( unsigned int *s )
{
	XGL_CALL( glGetIntegerv( GL_MAX_TEXTURE_SIZE, ( GLint * ) s ) );
}

/////////////////////////////////////////////////////////////

static void xgl_set_clear_colour( QmMathColour4ub rgba )
{
	XGL_CALL( glClearColor(
	        QM_MATH_BTOF( rgba.r ),
	        QM_MATH_BTOF( rgba.g ),
	        QM_MATH_BTOF( rgba.b ),
	        QM_MATH_BTOF( rgba.a ) ) );
}

static void xgl_clear_buffers( unsigned int buffers )
{
	// Rather ugly, but translate it over to GL.
	unsigned int glclear = 0;
	if ( buffers & PLG_BUFFER_COLOUR ) glclear |= GL_COLOR_BUFFER_BIT;
	if ( buffers & PLG_BUFFER_DEPTH ) glclear |= GL_DEPTH_BUFFER_BIT;
	if ( buffers & PLG_BUFFER_STENCIL ) glclear |= GL_STENCIL_BUFFER_BIT;
	XGL_CALL( glClear( glclear ) );
}

static void xgl_set_depth_buffer_mode( unsigned int mode )
{
	switch ( mode )
	{
		default:
			XGL_DEBUG( "Unknown depth buffer mode, %d\n", mode );
			break;

		case PLG_DEPTHBUFFER_DISABLE:
			XGL_CALL( glDisable( GL_DEPTH_TEST ) );
			break;

		case PLG_DEPTHBUFFER_ENABLE:
			XGL_CALL( glEnable( GL_DEPTH_TEST ) );
			break;
	}
}

static void xgl_depth_mask( bool enable )
{
	XGL_CALL( glDepthMask( enable ) );
}

/////////////////////////////////////////////////////////////

static unsigned int TranslateBlendFunc( PLGBlend blend )
{
	switch ( blend )
	{
		default:
		case PLG_BLEND_ONE:
			return GL_ONE;
		case PLG_BLEND_ZERO:
			return GL_ZERO;
		case PLG_BLEND_SRC_COLOR:
			return GL_SRC_COLOR;
		case PLG_BLEND_ONE_MINUS_SRC_COLOR:
			return GL_ONE_MINUS_SRC_COLOR;
		case PLG_BLEND_SRC_ALPHA:
			return GL_SRC_ALPHA;
		case PLG_BLEND_ONE_MINUS_SRC_ALPHA:
			return GL_ONE_MINUS_SRC_ALPHA;
		case PLG_BLEND_DST_ALPHA:
			return GL_DST_ALPHA;
		case PLG_BLEND_ONE_MINUS_DST_ALPHA:
			return GL_ONE_MINUS_DST_ALPHA;
		case PLG_BLEND_DST_COLOR:
			return GL_DST_COLOR;
		case PLG_BLEND_ONE_MINUS_DST_COLOR:
			return GL_ONE_MINUS_DST_COLOR;
		case PLG_BLEND_SRC_ALPHA_SATURATE:
			return GL_SRC_ALPHA_SATURATE;
	}
}

static void xgl_set_blend_mode( PLGBlend a, PLGBlend b )
{
	if ( a == PLG_BLEND_NONE && b == PLG_BLEND_NONE )
	{
		XGL_CALL( glDisable( GL_BLEND ) );
	}
	else
	{
		XGL_CALL( glEnable( GL_BLEND ) );
	}

	XGL_CALL( glBlendFunc( TranslateBlendFunc( a ), TranslateBlendFunc( b ) ) );
}

static void xgl_set_cull_mode( PLGCullMode mode )
{
	if ( mode == PLG_CULL_NONE )
	{
		XGL_CALL( glDisable( GL_CULL_FACE ) );
	}
	else
	{
		XGL_CALL( glEnable( GL_CULL_FACE ) );
		XGL_CALL( glCullFace( GL_BACK ) );
		switch ( mode )
		{
			default:
			case PLG_CULL_NEGATIVE:
				XGL_CALL( glFrontFace( GL_CW ) );
				break;

			case PLG_CULL_POSITIVE:
				XGL_CALL( glFrontFace( GL_CCW ) );
				break;
		}
	}
}


/////////////////////////////////////////////////////////////
// Framebuffer

static unsigned int xgl_translate_fbo_binding( PLGFrameBufferObjectTarget targetBinding )
{
	switch ( targetBinding )
	{
		default:
			return XGL_INVALID;
		case PLG_FRAMEBUFFER_DEFAULT:
			return GL_FRAMEBUFFER;
		case PLG_FRAMEBUFFER_DRAW:
			return GL_DRAW_FRAMEBUFFER;
		case PLG_FRAMEBUFFER_READ:
			return GL_READ_FRAMEBUFFER;
	}
}

enum
{
	XGL_FRAMEBUFFER_TARGET_DRAW,
	XGL_FRAMEBUFFER_TARGET_READ,

	XGL_MAX_FRAMEBUFFER_TARGETS
};
static uint32_t boundFrameBuffers[ XGL_MAX_FRAMEBUFFER_TARGETS ] = {
        [XGL_FRAMEBUFFER_TARGET_DRAW] = ( uint32_t ) -1,
        [XGL_FRAMEBUFFER_TARGET_READ] = ( uint32_t ) -1,
};

static void xgl_fbo_bind( QmGfxFramebuffer *self, PLGFrameBufferObjectTarget target_binding )
{
	uint32_t fbo = self != nullptr ? self->fbo : 0;

	unsigned int target = xgl_translate_fbo_binding( target_binding );
	assert( target != XGL_INVALID );
	XGL_CALL( glBindFramebuffer( target, fbo ) );

	if ( target_binding == PLG_FRAMEBUFFER_DEFAULT )
	{
		boundFrameBuffers[ XGL_FRAMEBUFFER_TARGET_DRAW ] = fbo;
		boundFrameBuffers[ XGL_FRAMEBUFFER_TARGET_READ ] = fbo;
	}
	else if ( target_binding == PLG_FRAMEBUFFER_DRAW )
	{
		boundFrameBuffers[ XGL_FRAMEBUFFER_TARGET_DRAW ] = fbo;
	}
	else
	{
		boundFrameBuffers[ XGL_FRAMEBUFFER_TARGET_READ ] = fbo;
	}
}

static void xgl_fbo_destroy( QmGfxFramebuffer *buffer );
static bool xgl_fbo_create( QmGfxFramebuffer *buffer )
{
	XGL_CALL( glCreateFramebuffers( 1, &buffer->fbo ) );
	xgl_fbo_bind( buffer, PLG_FRAMEBUFFER_DEFAULT );

	if ( buffer->flags & PLG_BUFFER_COLOUR )
	{
		XGL_CALL( glCreateRenderbuffers( 1, &buffer->renderBuffers[ PLG_RENDERBUFFER_COLOUR ] ) );
		XGL_CALL( glNamedRenderbufferStorageMultisample( buffer->renderBuffers[ PLG_RENDERBUFFER_COLOUR ], buffer->numSamples, GL_RGBA, ( int ) buffer->width, ( int ) buffer->height ) );
		XGL_CALL( glNamedFramebufferRenderbuffer( buffer->fbo, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, buffer->renderBuffers[ PLG_RENDERBUFFER_COLOUR ] ) );
	}

	if ( ( buffer->flags & PLG_BUFFER_DEPTH ) && ( buffer->flags & PLG_BUFFER_STENCIL ) )
	{
		XGL_CALL( glCreateRenderbuffers( 1, &buffer->renderBuffers[ PLG_RENDERBUFFER_DEPTH ] ) );
		XGL_CALL( glNamedRenderbufferStorageMultisample( buffer->renderBuffers[ PLG_RENDERBUFFER_DEPTH ], buffer->numSamples, GL_DEPTH24_STENCIL8, ( int ) buffer->width, ( int ) buffer->height ) );
		XGL_CALL( glNamedFramebufferRenderbuffer( buffer->fbo, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, buffer->renderBuffers[ PLG_RENDERBUFFER_DEPTH ] ) );
	}
	else if ( buffer->flags & PLG_BUFFER_DEPTH )
	{
		XGL_CALL( glCreateRenderbuffers( 1, &buffer->renderBuffers[ PLG_RENDERBUFFER_DEPTH ] ) );
		XGL_CALL( glNamedRenderbufferStorageMultisample( buffer->renderBuffers[ PLG_RENDERBUFFER_DEPTH ], buffer->numSamples, GL_DEPTH_COMPONENT24, ( int ) buffer->width, ( int ) buffer->height ) );
		XGL_CALL( glNamedFramebufferRenderbuffer( buffer->fbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, buffer->renderBuffers[ PLG_RENDERBUFFER_DEPTH ] ) );
	}
	else if ( buffer->flags & PLG_BUFFER_STENCIL )
	{
		XGL_CALL( glCreateRenderbuffers( 1, &buffer->renderBuffers[ PLG_RENDERBUFFER_STENCIL ] ) );
		XGL_CALL( glNamedRenderbufferStorageMultisample( buffer->renderBuffers[ PLG_RENDERBUFFER_STENCIL ], buffer->numSamples, GL_STENCIL_INDEX8, ( int ) buffer->width, ( int ) buffer->height ) );
		XGL_CALL( glNamedFramebufferRenderbuffer( buffer->fbo, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, buffer->renderBuffers[ PLG_RENDERBUFFER_STENCIL ] ) );
	}

	GLenum err = glCheckFramebufferStatus( GL_FRAMEBUFFER );
	if ( err != GL_FRAMEBUFFER_COMPLETE )
	{
		const char *msg;
		switch ( err )
		{
			default:
				msg = "unknown";
				break;
			case GL_FRAMEBUFFER_UNDEFINED:
				msg = "the specified framebuffer is the default read or draw framebuffer, but the default framebuffer "
				      "does not exist";
				break;
			case GL_FRAMEBUFFER_UNSUPPORTED:
				msg = "the combination of internal formats of the attached images violates an implementation-dependent "
				      "set of restrictions";
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
				msg = "the framebuffer attachment points are framebuffer incomplete";
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
				msg = "the framebuffer does not have at least one image attached to it";
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
				msg = "the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for any color attachment point(s) "
				      "named by GL_DRAW_BUFFERi";
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
				msg = "GL_READ_BUFFER is not GL_NONE and the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE "
				      "for the color attachment point named by GL_READ_BUFFER";
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
				msg = "the value of GL_RENDERBUFFER_SAMPLES is not the same for all attached renderbuffers; "
				      "if the value of GL_TEXTURE_SAMPLES is the not same for all attached textures; "
				      "or, if the attached images are a mix of renderbuffers and textures, "
				      "the value of GL_RENDERBUFFER_SAMPLES does not match the value of GL_TEXTURE_SAMPLES";
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
				msg = "framebuffer attachment is layered, and any populated attachment is not layered, or if all "
				      "populated color attachments are not from textures of the same target";
				break;
		}

		//TODO: graphics API really needs it's own error reporting solution...
		gInterface->core->ReportError( PL_RESULT_UNSUPPORTED, __FUNCTION__, "%s", msg );

		xgl_fbo_destroy( buffer );
		return false;
	}

	return true;
}

static void xgl_fbo_destroy( QmGfxFramebuffer *buffer )
{
	if ( buffer == nullptr )
	{
		return;
	}

	// automatically unbind it if it's bound
	if ( boundFrameBuffers[ XGL_FRAMEBUFFER_TARGET_READ ] == buffer->fbo )
	{
		xgl_fbo_bind( nullptr, XGL_FRAMEBUFFER_TARGET_READ );
	}
	if ( boundFrameBuffers[ XGL_FRAMEBUFFER_TARGET_DRAW ] == buffer->fbo )
	{
		xgl_fbo_bind( nullptr, XGL_FRAMEBUFFER_TARGET_DRAW );
	}

	if ( buffer->fbo != 0 )
	{
		XGL_CALL( glDeleteFramebuffers( 1, &buffer->fbo ) );
		buffer->fbo = 0;
	}

	for ( unsigned int i = 0; i < PLG_MAX_RENDERBUFFER_TYPES; ++i )
	{
		if ( buffer->renderBuffers[ i ] == 0 )
		{
			continue;
		}

		XGL_CALL( glDeleteRenderbuffers( 1, &buffer->renderBuffers[ i ] ) );
		buffer->renderBuffers[ i ] = 0;
	}
}

static void xgl_fbo_blit( QmGfxFramebuffer *src_buffer,
                          unsigned int      src_w,
                          unsigned int      src_h,
                          QmGfxFramebuffer *dst_buffer,
                          unsigned int      dst_w,
                          unsigned int      dst_h,
                          unsigned int      mask,
                          bool              linear )
{
	GLbitfield bits = 0;
	if ( mask & PLG_BUFFER_DEPTH )
	{
		bits |= GL_DEPTH_BUFFER_BIT;
		linear = false;
	}
	if ( mask & PLG_BUFFER_STENCIL )
	{
		bits |= GL_STENCIL_BUFFER_BIT;
		linear = false;
	}
	if ( mask & PLG_BUFFER_COLOUR )
	{
		bits |= GL_COLOR_BUFFER_BIT;
	}

	XGL_CALL( glBlitNamedFramebuffer( src_buffer ? src_buffer->fbo : 0,
	                                  dst_buffer ? dst_buffer->fbo : 0,
	                                  0, 0, src_w, src_h,
	                                  0, 0, dst_w, dst_h,
	                                  bits,
	                                  linear ? GL_LINEAR : GL_NEAREST ) );
}

static void xgl_fbo_set_size( QmGfxFramebuffer *frameBuffer, unsigned int width, unsigned int height )
{
	/* just to be safe, flush the whole thing */
	xgl_fbo_destroy( frameBuffer );

	/* and given we don't flush any flags etc., we can
	 * pretty much just update what we want and create
	 * it again */
	frameBuffer->width  = width;
	frameBuffer->height = height;
	xgl_fbo_create( frameBuffer );
}

static void *xgl_fbo_read_region( QmGfxFramebuffer *frameBuffer, uint32_t x, uint32_t y, uint32_t w, uint32_t h, size_t dstSize, void *dstBuf )
{
	xgl_fbo_bind( frameBuffer, PLG_FRAMEBUFFER_READ );

	XGL_CALL( glReadnPixels( ( GLint ) x, ( GLint ) y,
	                         ( GLsizei ) w, ( GLsizei ) h,
	                         GL_RGBA, GL_UNSIGNED_BYTE,
	                         ( GLsizei ) dstSize, dstBuf ) );
	if ( glGetError() != GL_NO_ERROR )
	{
		return nullptr;
	}

	return dstBuf;
}

static void         xgl_texture_set_filter( QmGfxTexture *self, QmGfxTextureFilter filter );
static void         xgl_texture_set_wrap( QmGfxTexture *self, QmGfxTextureWrapMode wrapMode );
static unsigned int xgl_translate_wrap_mode( QmGfxTextureWrapMode wrapMode );

static QmGfxTexture *xgl_fbo_create_texture_attachment( QmGfxFramebuffer *buffer, unsigned int components, QmGfxTextureFilter filter, QmGfxTextureWrapMode wrap )
{
	// urgh, this is to ensure all interactions with the texture later on are correct
	QmGfxTexture *texture = gInterface->CreateTexture( buffer->numSamples > 0 ? QM_GFX_TEXTURE_TYPE_2D_MULTISAMPLE : QM_GFX_TEXTURE_TYPE_2D );
	if ( texture == nullptr )
	{
		return nullptr;
	}

	/* all of this is going to change later...
	 * this is just the bare minimum to get things going */

	texture->w = buffer->width;
	texture->h = buffer->height;

	xgl_fbo_bind( buffer, PLG_FRAMEBUFFER_DRAW );

	XGL_CALL( glBindTexture( ( ( XglTexture * ) texture->driver )->target, ( ( XglTexture * ) texture->driver )->id ) );

	xgl_texture_set_filter( texture, filter );
	xgl_texture_set_wrap( texture, wrap );

	/* sigh... */
	if ( components & PLG_BUFFER_DEPTH || components & PLG_BUFFER_STENCIL )
	{
		if ( buffer->flags & PLG_BUFFER_DEPTH && buffer->flags & PLG_BUFFER_STENCIL )
		{
			/* so yeah, this sucks, but if both of these are active we assume it's packed */
			XGL_CALL( glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, buffer->width, buffer->height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr ) );
			if ( components & PLG_BUFFER_DEPTH )
			{
				XGL_CALL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, ( ( XglTexture * ) texture->driver )->id, 0 ) );
			}
			if ( components & PLG_BUFFER_STENCIL )
			{
				XGL_CALL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, ( ( XglTexture * ) texture->driver )->id, 0 ) );
			}
		}
		else
		{
			/* otherwise, assumed not packed */
			if ( components & PLG_BUFFER_DEPTH )
			{
				XGL_CALL( glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, buffer->width, buffer->height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr ) );
				XGL_CALL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, ( ( XglTexture * ) texture->driver )->id, 0 ) );
			}
			else if ( components & PLG_BUFFER_STENCIL )
			{
				XGL_CALL( glTexImage2D( GL_TEXTURE_2D, 0, GL_STENCIL_INDEX8, buffer->width, buffer->height, 0, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, nullptr ) );
				XGL_CALL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, ( ( XglTexture * ) texture->driver )->id, 0 ) );
			}
		}
	}
	else if ( components & PLG_BUFFER_COLOUR )
	{
		if ( buffer->numSamples > 0 )
		{
			XGL_CALL( glTexImage2DMultisample( GL_TEXTURE_2D_MULTISAMPLE, buffer->numSamples, GL_RGBA8, buffer->width, buffer->height, GL_TRUE ) );
		}
		else
		{
			XGL_CALL( glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, buffer->width, buffer->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr ) );
		}
		XGL_CALL( glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, ( ( XglTexture * ) texture->driver )->target, ( ( XglTexture * ) texture->driver )->id, 0 ) );
	}

	XGL_CALL( glBindTexture( ( ( XglTexture * ) texture->driver )->target, 0 ) );

	return texture;
}

static void xgl_colour_mask( bool r, bool g, bool b, bool a )
{
	XGL_CALL( glColorMask( r, g, b, a ) );
}

static void xgl_stencil_mask( unsigned int mask )
{
	XGL_CALL( glStencilMask( mask ) );
}

/////////////////////////////////////////////////////////////
// Stencil Operations

/////////////////////////////////////////////////////////////
// Texture

static unsigned int xgl_translate_image_format( PLImageFormat format )
{
	switch ( format )
	{
		default:
			return XGL_INVALID;
		case PL_IMAGEFORMAT_R8:
			return GL_R8;
		case PL_IMAGEFORMAT_RGB8:
			return GL_RGB8;
		case PL_IMAGEFORMAT_RGBA8:
			return GL_RGBA8;
		case PL_IMAGEFORMAT_RGB4:
			return GL_RGB4;
		case PL_IMAGEFORMAT_RGBA4:
			return GL_RGBA4;
		case PL_IMAGEFORMAT_RGB5:
			return GL_RGB5;
		case PL_IMAGEFORMAT_RGB5A1:
			return GL_RGB5_A1;

		case PL_IMAGEFORMAT_RGB16F:
			return GL_RGB16F;
		case PL_IMAGEFORMAT_RGBA16F:
			return GL_RGBA16F;

		case PL_IMAGEFORMAT_RGB32F:
			return GL_RGB32F;
		case PL_IMAGEFORMAT_RGBA32F:
			return GL_RGBA32F;

		case PL_IMAGEFORMAT_RGB_DXT1:
			return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		case PL_IMAGEFORMAT_RGBA_DXT1:
			return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		case PL_IMAGEFORMAT_RGBA_DXT3:
			return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		case PL_IMAGEFORMAT_RGBA_DXT5:
			return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;

		case PL_IMAGEFORMAT_RGB_FXT1:
			return GL_COMPRESSED_RGB_FXT1_3DFX;
	}
}

static unsigned int xgl_get_storage_format_for_image_format( PLImageFormat format )
{
	switch ( format )
	{
		default:
		case PL_IMAGEFORMAT_UNKNOWN:
			return XGL_INVALID;
		case PL_IMAGEFORMAT_RGB4:
		case PL_IMAGEFORMAT_RGBA4:
			return GL_UNSIGNED_SHORT_4_4_4_4;
		case PL_IMAGEFORMAT_RGB5:
		case PL_IMAGEFORMAT_RGB5A1:
			return GL_UNSIGNED_SHORT_5_5_5_1;
		case PL_IMAGEFORMAT_RGB565:
			return GL_UNSIGNED_SHORT_5_6_5;
		case PL_IMAGEFORMAT_R8:
		case PL_IMAGEFORMAT_RGB8:
		case PL_IMAGEFORMAT_BGR8:
		case PL_IMAGEFORMAT_RGBA8:
		case PL_IMAGEFORMAT_BGRA8:
		case PL_IMAGEFORMAT_BGRX8:
			return GL_UNSIGNED_BYTE;
		case PL_IMAGEFORMAT_RGBA12:
			return GL_UNSIGNED_INT_10_10_10_2;
		case PL_IMAGEFORMAT_RGBA16:
			return GL_UNSIGNED_SHORT;
		case PL_IMAGEFORMAT_RGB16F:
		case PL_IMAGEFORMAT_RGBA16F:
			return GL_HALF_FLOAT;
		case PL_IMAGEFORMAT_RGB32F:
		case PL_IMAGEFORMAT_RGBA32F:
			return GL_FLOAT;
		case PL_IMAGEFORMAT_RGBA_DXT1:
			return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		case PL_IMAGEFORMAT_RGB_DXT1:
			return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		case PL_IMAGEFORMAT_RGBA_DXT3:
			return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		case PL_IMAGEFORMAT_RGBA_DXT5:
			return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		case PL_IMAGEFORMAT_RGB_FXT1:
			return GL_COMPRESSED_RGB_FXT1_3DFX;
	}
}

static unsigned int xgl_get_colour_format_for_image_format( PLImageFormat format )
{
	switch ( format )
	{
		default:
			return XGL_INVALID;
		case PL_IMAGEFORMAT_R8:
			return GL_RED;
		case PL_IMAGEFORMAT_RGB4:
		case PL_IMAGEFORMAT_RGB5:
		case PL_IMAGEFORMAT_RGB565:
		case PL_IMAGEFORMAT_RGB8:
		case PL_IMAGEFORMAT_RGB_DXT1:
		case PL_IMAGEFORMAT_RGB_FXT1:
		case PL_IMAGEFORMAT_RGB16F:
		case PL_IMAGEFORMAT_RGB32F:
			return GL_RGB;
		case PL_IMAGEFORMAT_RGBA8:
		case PL_IMAGEFORMAT_RGBA_DXT3:
			return GL_RGBA;
	}
}

static unsigned int xgl_texture_get_target_for_type( const QmGfxTextureType type )
{
	switch ( type )
	{
		default:
			return XGL_INVALID;
		case QM_GFX_TEXTURE_TYPE_1D: return GL_TEXTURE_1D;
		case QM_GFX_TEXTURE_TYPE_2D: return GL_TEXTURE_2D;
		case QM_GFX_TEXTURE_TYPE_2D_MULTISAMPLE: return GL_TEXTURE_2D_MULTISAMPLE;
		case QM_GFX_TEXTURE_TYPE_3D: return GL_TEXTURE_3D;
		case QM_GFX_TEXTURE_TYPE_CUBEMAP: return GL_TEXTURE_CUBE_MAP;
	}
}

static void xgl_texture_create( QmGfxTexture *self )
{
	self->driver = gInterface->core->MAlloc( sizeof( XglTexture ), true );
	assert( self->driver != nullptr );

	( ( XglTexture * ) self->driver )->target = xgl_texture_get_target_for_type( self->type );
	XGL_CALL( glCreateTextures( ( ( XglTexture * ) self->driver )->target, 1, &( ( XglTexture * ) self->driver )->id ) );

	self->wrapMode = PLG_TEXTURE_WRAP_MODE_REPEAT;
}

static void xgl_texture_destroy( QmGfxTexture *self )
{
	XGL_CALL( glDeleteTextures( 1, &( ( XglTexture * ) self->driver )->id ) );

	gInterface->core->Free( self->driver );
	self->driver = nullptr;
}

static void xgl_texture_bind( const QmGfxTexture *texture, unsigned int unit )
{
	if ( texture == nullptr )
	{
		XGL_CALL( glBindTextureUnit( unit, 0 ) );
		return;
	}

	XGL_CALL( glBindTextureUnit( unit, ( ( XglTexture * ) texture->driver )->id ) );
}

static bool xgl_is_compressed_image_format( PLImageFormat format )
{
	switch ( format )
	{
		default:
			return false;
		case PL_IMAGEFORMAT_RGBA_DXT1:
		case PL_IMAGEFORMAT_RGBA_DXT3:
		case PL_IMAGEFORMAT_RGBA_DXT5:
		case PL_IMAGEFORMAT_RGB_DXT1:
		case PL_IMAGEFORMAT_RGB_FXT1:
			return true;
	}
}

static void xgl_texture_upload( QmGfxTexture *self, const QmImage *upload )
{
	assert( upload->data != nullptr && upload->data[ 0 ] != nullptr );

	xgl_texture_set_wrap( self, self->wrapMode );
	xgl_texture_set_filter( self, self->filter );

	XglTexture *drv = self->driver;
	assert( drv != nullptr );

	if ( upload->levels == 1 && !( self->filter == PLG_TEXTURE_FILTER_LINEAR || self->filter == PLG_TEXTURE_FILTER_NEAREST ) )
	{
		if ( drv->target != GL_TEXTURE_CUBE_MAP || drv->faceIndex == QM_GFX_TEXTURE_MAX_CUBEMAP_FACES )
		{
			self->flags &= ~PLG_TEXTURE_FLAG_NOMIPS;
		}
	}
	else if ( upload->levels > 1 )
	{
		self->flags &= ~PLG_TEXTURE_FLAG_NOMIPS;
	}
	else
	{
		self->flags |= PLG_TEXTURE_FLAG_NOMIPS;
	}

	bool generateMipmaps = !( self->flags & PLG_TEXTURE_FLAG_NOMIPS ) && upload->levels <= 1;
	if ( drv->target == GL_TEXTURE_CUBE_MAP && drv->faceIndex < QM_GFX_TEXTURE_MAX_CUBEMAP_FACES - 1 )
	{
		// only generate the mipmaps at the end for cubemaps
		generateMipmaps = false;
	}

	unsigned int internalFormat = xgl_translate_image_format( upload->format );
	assert( internalFormat != XGL_INVALID );

	GLuint id = drv->id;
	if ( !drv->hasStorage )
	{
		XGL_CALL( glTextureStorage2D( id, generateMipmaps ? 4 : upload->levels, internalFormat, upload->width, upload->height ) );
		drv->hasStorage = true;
	}

	for ( unsigned int i = 0; i < upload->levels; ++i )
	{
		const void *data = upload->data[ i ];
		GLsizei     w    = upload->width >> i;
		GLsizei     h    = upload->height >> i;
		if ( xgl_is_compressed_image_format( upload->format ) )
		{
			GLsizei size;
			if ( i > 0 )
			{
				size = ( GLsizei ) gInterface->core->GetImageSize( upload->format, w, h );
			}
			else
			{
				size = ( GLsizei ) upload->size;
			}

			if ( drv->target == GL_TEXTURE_CUBE_MAP )
			{
				XGL_CALL( glCompressedTextureSubImage3D( id, i, 0, 0,
				                                         drv->faceIndex,
				                                         w, h, 1,
				                                         internalFormat, size, data ) );
			}
			else
			{
				XGL_CALL( glCompressedTextureSubImage2D( id, i, 0, 0,
				                                         w, h,
				                                         internalFormat, size, data ) );
			}
		}
		else
		{
			GLenum format = xgl_get_colour_format_for_image_format( upload->format );
			assert( format != 0 );

			GLenum type = xgl_get_storage_format_for_image_format( upload->format );
			assert( type != 0 );

			if ( drv->target == GL_TEXTURE_CUBE_MAP )
			{
				XGL_CALL( glTextureSubImage3D( id, 0, 0, 0,
				                               drv->faceIndex,
				                               upload->width, upload->height, 1,
				                               format, type, data ) );
			}
			else
			{
				XGL_CALL( glTextureSubImage2D( id, i, 0, 0, w, h, format, type, data ) );
			}
		}
	}

	if ( generateMipmaps )
	{
		XGL_CALL( glGenerateTextureMipmap( drv->id ) );
	}

	// cubemaps are uh, fun... especially because we worried about them later!
	if ( drv->target == GL_TEXTURE_CUBE_MAP )
	{
		// increment the index and wrap if we go over...
		drv->faceIndex++;
		if ( drv->faceIndex > QM_GFX_TEXTURE_MAX_CUBEMAP_FACES )
		{
			drv->faceIndex = 0;
		}
	}
}

static void xgl_texture_set_anisotropy( QmGfxTexture *self, uint32_t value )
{
	GLuint id = ( ( XglTexture * ) self->driver )->id;
	XGL_CALL( glTextureParameterf( id, GL_TEXTURE_MAX_ANISOTROPY, ( float ) value ) );
}

static void xgl_texture_set_filter( QmGfxTexture *self, QmGfxTextureFilter filter )
{
	GLenum target = ( ( XglTexture * ) self->driver )->target;
	if ( target != GL_TEXTURE_2D_MULTISAMPLE )
	{
		int min, mag;
		xgl_translate_texture_filter_format( filter, &min, &mag );

		GLuint id = ( ( XglTexture * ) self->driver )->id;
		XGL_CALL( glTextureParameteri( id, GL_TEXTURE_MAG_FILTER, mag ) );
		XGL_CALL( glTextureParameteri( id, GL_TEXTURE_MIN_FILTER, min ) );
	}

	self->filter = filter;
}

static unsigned int xgl_translate_wrap_mode( QmGfxTextureWrapMode wrapMode )
{
	unsigned int glWrapMode;
	switch ( wrapMode )
	{
		default:
			return XGL_INVALID;
		case PLG_TEXTURE_WRAP_MODE_REPEAT:
			glWrapMode = GL_REPEAT;
			break;
		case PLG_TEXTURE_WRAP_MODE_CLAMP_BORDER:
			glWrapMode = GL_CLAMP_TO_BORDER;
			break;
		case PLG_TEXTURE_WRAP_MODE_CLAMP_EDGE:
			glWrapMode = GL_CLAMP_TO_EDGE;
			break;
		case PLG_TEXTURE_WRAP_MODE_MIRRORED_REPEAT:
			glWrapMode = GL_MIRRORED_REPEAT;
			break;
	}

	return glWrapMode;
}

static void xgl_texture_set_wrap( QmGfxTexture *self, QmGfxTextureWrapMode wrapMode )
{
	int glWrapMode = xgl_translate_wrap_mode( wrapMode );
	assert( glWrapMode != XGL_INVALID );

	GLuint id     = ( ( XglTexture * ) self->driver )->id;
	GLenum target = ( ( XglTexture * ) self->driver )->target;
	if ( target == GL_TEXTURE_CUBE_MAP )
	{
		XGL_CALL( glTextureParameteri( id, GL_TEXTURE_WRAP_S, glWrapMode ) );
		XGL_CALL( glTextureParameteri( id, GL_TEXTURE_WRAP_T, glWrapMode ) );
		XGL_CALL( glTextureParameteri( id, GL_TEXTURE_WRAP_R, glWrapMode ) );
	}
	else if ( target != GL_TEXTURE_2D_MULTISAMPLE )
	{
		XGL_CALL( glTextureParameteri( id, GL_TEXTURE_WRAP_S, glWrapMode ) );
		XGL_CALL( glTextureParameteri( id, GL_TEXTURE_WRAP_T, glWrapMode ) );
	}

	self->wrapMode = wrapMode;
}

/////////////////////////////////////////////////////////////

static QmMathVector4f clipPlane;
static PLMatrix4      clipPlaneMatrix;

static void xgl_set_clip_plane( const QmMathVector4f *clip, const PLMatrix4 *transform, bool transpose )
{
	if ( clip == nullptr )
	{
		glDisable( GL_CLIP_DISTANCE0 );
		return;
	}

	glEnable( GL_CLIP_DISTANCE0 );
	clipPlane = *clip;

	clipPlaneMatrix = transform != nullptr ? *transform : PlMatrix4Identity();
	if ( transpose )
	{
		clipPlaneMatrix = PlTransposeMatrix4( &clipPlaneMatrix );
	}
}

const QmMathVector4f *xgl_get_clip_plane()
{
	return &clipPlane;
}

const PLMatrix4 *xgl_get_clip_matrix()
{
	return &clipPlaneMatrix;
}

/////////////////////////////////////////////////////////////
// Viewport

static struct
{
	int x, y, w, h;
} viewport;

static void xgl_clip_viewport( int x, int y, int width, int height )
{
	if ( viewport.x != x || viewport.y != y || viewport.w != width || viewport.h != height )
	{
		XGL_CALL( glEnable( GL_SCISSOR_TEST ) );
	}
	else
	{
		XGL_CALL( glDisable( GL_SCISSOR_TEST ) );
		return;
	}

	XGL_CALL( glScissor( x, y, width, height ) );
}

static void xgl_set_viewport( int x, int y, int width, int height )
{
	XGL_CALL( glViewport( x, y, width, height ) );

	viewport.x = x;
	viewport.y = y;
	viewport.w = width;
	viewport.h = height;
}

/////////////////////////////////////////////////////////////
// Shader

static const char *uniformDescriptors[ QM_GFX_MAX_SHADER_UNIFORM_TYPES ] = {
        [QM_GFX_SHADER_UNIFORM_TYPE_INVALID]         = "invalid",
        [QM_GFX_SHADER_UNIFORM_TYPE_FLOAT]           = "float",
        [QM_GFX_SHADER_UNIFORM_TYPE_INT]             = "int",
        [QM_GFX_SHADER_UNIFORM_TYPE_UINT]            = "uint",
        [QM_GFX_SHADER_UNIFORM_TYPE_BOOL]            = "bool",
        [QM_GFX_SHADER_UNIFORM_TYPE_DOUBLE]          = "double",
        [QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER1D]       = "sampler1D",
        [QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER2D]       = "sampler2D",
        [QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER3D]       = "sampler3D",
        [QM_GFX_SHADER_UNIFORM_TYPE_SAMPLERCUBE]     = "samplerCube",
        [QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER1DSHADOW] = "sampler1DShadow",
        [QM_GFX_SHADER_UNIFORM_TYPE_VEC2]            = "vec2",
        [QM_GFX_SHADER_UNIFORM_TYPE_VEC3]            = "vec3",
        [QM_GFX_SHADER_UNIFORM_TYPE_VEC4]            = "vec4",
        [QM_GFX_SHADER_UNIFORM_TYPE_MAT3]            = "mat3",
        [QM_GFX_SHADER_UNIFORM_TYPE_MAT4]            = "mat4",
};

static QmGfxShaderUniformType xgl_convert_gl_uniform_type( unsigned int type )
{
	switch ( type )
	{
		case GL_FLOAT:
			return QM_GFX_SHADER_UNIFORM_TYPE_FLOAT;
		case GL_FLOAT_VEC2:
			return QM_GFX_SHADER_UNIFORM_TYPE_VEC2;
		case GL_FLOAT_VEC3:
			return QM_GFX_SHADER_UNIFORM_TYPE_VEC3;
		case GL_FLOAT_VEC4:
			return QM_GFX_SHADER_UNIFORM_TYPE_VEC4;
		case GL_FLOAT_MAT3:
			return QM_GFX_SHADER_UNIFORM_TYPE_MAT3;
		case GL_FLOAT_MAT4:
			return QM_GFX_SHADER_UNIFORM_TYPE_MAT4;

		case GL_DOUBLE:
			return QM_GFX_SHADER_UNIFORM_TYPE_DOUBLE;

		case GL_INT:
			return QM_GFX_SHADER_UNIFORM_TYPE_INT;
		case GL_UNSIGNED_INT:
			return QM_GFX_SHADER_UNIFORM_TYPE_UINT;

		case GL_BOOL:
			return QM_GFX_SHADER_UNIFORM_TYPE_BOOL;

		case GL_SAMPLER_1D:
			return QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER1D;
		case GL_SAMPLER_1D_SHADOW:
			return QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER1DSHADOW;
		case GL_SAMPLER_2D:
			return QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER2D;
		case GL_SAMPLER_2D_SHADOW:
			return QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER2DSHADOW;
		case GL_SAMPLER_CUBE:
			return QM_GFX_SHADER_UNIFORM_TYPE_SAMPLERCUBE;

		default:
		{
			XGL_DEBUG( "Unhandled GLSL data type, \"%u\"!\n", type );
			return QM_GFX_SHADER_UNIFORM_TYPE_INVALID;
		}
	}
}

static GLenum gl_translate_shader_stage_type( QmGfxShaderStageType type )
{
	switch ( type )
	{
		default:
			return XGL_INVALID;
		case QM_GFX_SHADER_STAGE_TYPE_VERTEX:
			return GL_VERTEX_SHADER;
		case QM_GFX_SHADER_STAGE_TYPE_COMPUTE:
			return GL_COMPUTE_SHADER;
		case QM_GFX_SHADER_STAGE_TYPE_FRAGMENT:
			return GL_FRAGMENT_SHADER;
	}
}

/**
 * Inserts the given string into an existing string buffer.
 * Automatically reallocs buffer if it doesn't fit.
 * todo: consider cleaning this up and making part of API?
 */
static char *InsertString( const char *string, char **buf, size_t *bufSize, size_t *maxBufSize )
{
	/* check if it's going to fit first */
	size_t strLength    = strlen( string );
	size_t originalSize = *bufSize;
	*bufSize += strLength;
	if ( *bufSize >= *maxBufSize )
	{
		*maxBufSize = *bufSize + strLength;
		*buf        = gInterface->core->ReAlloc( *buf, *maxBufSize + 1, true );
	}

	/* now copy it into our buffer */
	strncpy( *buf + originalSize, string, strLength );

	return *buf + originalSize + strLength;
}

/**
 * A basic pre-processor for GLSL - will condense the shader as much as possible
 * and handle any pre-processor commands.
 * todo: this is dumb... rewrite it
 */
static char *xgl_shader_stage_pp_glsl( QmGfxShaderStage *self, char *buf, size_t *length, bool head, const char *directory )
{
	/* setup the destination buffer */
	size_t actualLength = 0;
	size_t maxLength    = *length;
	char  *dstBuffer    = gInterface->core->MAlloc( maxLength + 1, true );
	char  *dstPos       = dstBuffer;

	/* built-ins */
#define insert( str ) dstPos = InsertString( ( str ), &dstBuffer, &actualLength, &maxLength )
	if ( head )
	{
		insert( "#version 430 core\n" );
		insert( "uniform mat4 pl_model;\n" );
		insert( "uniform mat4 pl_view;\n" );
		insert( "uniform mat4 pl_proj;\n" );
		insert( "uniform mat4 pl_texture;\n" );
		insert( "uniform vec4 pl_clipplane;\n" );
		insert( "uniform mat4 pl_clipplane_matrix;\n" );
		if ( self->type == QM_GFX_SHADER_STAGE_TYPE_VERTEX )
		{
			insert( "out float gl_ClipDistance[1];\n" );
			insert( "#define PLG_COMPILE_VERTEX 1\n" );
		}
		else if ( self->type == QM_GFX_SHADER_STAGE_TYPE_FRAGMENT )
		{
			insert( "out vec4 pl_frag;\n" );
			insert( "#define PLG_COMPILE_FRAGMENT 1\n" );
		}
		for ( unsigned int i = 0; i < self->numDefinitions; ++i )
		{
			char line[ 32 ];
			snprintf( line, sizeof( line ), "#define %s 1\n", self->definitions[ i ] );
			insert( line );
		}
	}

	const char *srcPos = buf;
	const char *srcEnd = buf + *length;
	while ( srcPos < srcEnd )
	{
		if ( *srcPos == '\0' )
		{
			break;
		}

		if ( *srcPos == '#' )
		{
			const char *p = srcPos;
			srcPos++;

			char token[ 32 ];
			gInterface->core->ParseToken( &srcPos, token, sizeof( token ) );
			if ( strcmp( token, "include" ) == 0 )
			{
				gInterface->core->SkipWhitespace( &srcPos );

				/* pull the path - needs to be enclosed otherwise this'll fail */
				PLPath path;
				gInterface->core->ParseEnclosedString( &srcPos, path, sizeof( path ) );
				if ( directory != nullptr )
				{
					PLPath tmp;
					gInterface->core->SetupPath( tmp, true, "%s/%s", directory, path );
					strcpy( path, tmp );
				}

				QmFsFile *file = gInterface->core->OpenFile( path, true );
				if ( file != nullptr )
				{
					/* allocate a temporary buffer */
					size_t incLength = gInterface->core->GetFileSize( file );
					char  *incBuf    = gInterface->core->MAlloc( incLength + 1, true );
					memcpy( incBuf, gInterface->core->GetFileData( file ), incLength );

					/* close the current file, to avoid recursively opening files
					 * and hitting any limits */
					gInterface->core->CloseFile( file );

					PLPath tmp;
					gInterface->core->SetupPath( tmp, true, "%s", path );
					char *sep = strrchr( tmp, '/' );
					if ( sep != nullptr )
					{
						*sep = '\0';
					}

					/* now throw it into the pre-processor */
					incBuf = xgl_shader_stage_pp_glsl( self, incBuf, &incLength, false, tmp );

					/* and finally, push it into our destination */
					dstPos = InsertString( incBuf, &dstBuffer, &actualLength, &maxLength );
					gInterface->core->Free( incBuf );
				}
				else
				{
					XGL_DEBUG( "Failed to load include \"%s\": %s\n", path, gInterface->core->GetError() );
				}

				gInterface->core->SkipLine( &srcPos );
				continue;
			}

			/* we didn't need to do anything, so restore our position */
			srcPos = p;
		}

		if ( ++actualLength > maxLength )
		{
			maxLength += 256;
			char *oldDstBuffer = dstBuffer;
			dstBuffer          = gInterface->core->ReAlloc( dstBuffer, maxLength + 1, true );
			dstPos             = dstBuffer + ( dstPos - oldDstBuffer );
		}

		*dstPos++ = *srcPos++;
	}

	/* free the original buffer that was passed in */
	gInterface->core->Free( buf );

	/* resize and update buf to match */
	*length              = actualLength;
	dstBuffer[ *length ] = '\0';

	return dstBuffer;
}

static void xgl_shader_program_create( QmGfxShaderProgram *program )
{
	XglShaderProgram *drv = gInterface->core->MAlloc( sizeof( XglShaderProgram ), true );

	drv->id = glCreateProgram();
	if ( drv->id == 0 )
	{
		XGL_DEBUG( "Failed to generate shader program!\n" );
		return;
	}

	memset( drv->defaultUniforms, XGL_INVALID, sizeof( XglShaderUniformType ) * XGL_SHADER_UNIFORM_TYPE_MAX );

	program->driver = drv;
}

static void xgl_shader_program_destroy( QmGfxShaderProgram *program )
{
	XglShaderProgram *drv = program->driver;

	if ( drv->id != 0 )
	{
		glDeleteProgram( drv->id );
	}

	gInterface->core->Free( drv );
	program->driver = nullptr;
}

static void xgl_shader_program_attach_stage( QmGfxShaderProgram *self, QmGfxShaderStage *stage )
{
	XglShaderProgram *drv = self->driver;
	XGL_CALL( glAttachShader( drv->id, stage->internal.id ) );
}

static void xgl_shader_stage_create( QmGfxShaderStage *self )
{
	GLenum type = gl_translate_shader_stage_type( self->type );
	if ( type == XGL_INVALID )
	{
		gInterface->core->ReportError( PL_RESULT_INVALID_SHADER_TYPE, __FUNCTION__, "%u", type );
		return;
	}

	self->internal.id = glCreateShader( type );
	if ( self->internal.id == 0 )
	{
		gInterface->core->ReportError( PL_RESULT_INVALID_SHADER_TYPE, __FUNCTION__, "%u", type );
		return;
	}
}

static void xgl_shader_stage_destroy( QmGfxShaderStage *self )
{
	if ( self->program != nullptr )
	{
		XglShaderProgram *drv = self->program->driver;
		XGL_CALL( glDetachShader( drv->id, self->internal.id ) );
		self->program = nullptr;
	}

	XGL_CALL( glDeleteShader( self->internal.id ) );
	self->internal.id = 0;
}

static bool xgl_shader_stage_compile( QmGfxShaderStage *stage, const char *buf, size_t length, const char *directory )
{
	/* shove this here for now... */
	char *temp = gInterface->core->MAlloc( length + 1, true );
	memcpy( temp, buf, length );

	temp = xgl_shader_stage_pp_glsl( stage, temp, &length, true, directory );

	XGL_CALL( glShaderSource( stage->internal.id, 1, ( const GLchar ** ) &temp, ( GLint * ) &length ) );
	XGL_CALL( glCompileShader( stage->internal.id ) );

	GLboolean status;
	XGL_CALL( glGetShaderiv( stage->internal.id, GL_COMPILE_STATUS, ( GLint * ) &status ) );
	if ( !status )
	{
		int s_length;
		XGL_CALL( glGetShaderiv( stage->internal.id, GL_INFO_LOG_LENGTH, &s_length ) );
		if ( s_length > 1 )
		{
			char *log = gInterface->core->CAlloc( ( size_t ) s_length, sizeof( char ), true );
			XGL_CALL( glGetShaderInfoLog( stage->internal.id, s_length, nullptr, log ) );
			XGL_DEBUG( " COMPILE ERROR:\n%s\n", log );
			gInterface->core->ReportError( PL_RESULT_SHADER_COMPILE, "%s", log );
			gInterface->core->Free( log );
		}
	}

	gInterface->core->Free( temp );

	return status;
}

static void xgl_shader_program_set_uniform( QmGfxShaderProgram *self, const int slot, const void *value, bool transpose )
{
	switch ( self->uniforms[ slot ].type )
	{
		case QM_GFX_SHADER_UNIFORM_TYPE_FLOAT:
			XGL_CALL( glUniform1f( self->uniforms[ slot ].slot, *( float * ) value ) );
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER2D:
		case QM_GFX_SHADER_UNIFORM_TYPE_INT:
			XGL_CALL( glUniform1i( self->uniforms[ slot ].slot, *( int * ) value ) );
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_UINT:
			XGL_CALL( glUniform1ui( self->uniforms[ slot ].slot, *( unsigned int * ) value ) );
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_BOOL:
			XGL_CALL( glUniform1i( self->uniforms[ slot ].slot, *( bool * ) value ) );
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_DOUBLE:
			XGL_CALL( glUniform1d( self->uniforms[ slot ].slot, *( double * ) value ) );
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_VEC2:
			XGL_CALL( glUniform2fv( self->uniforms[ slot ].slot, 1, value ) );
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_VEC3:
			XGL_CALL( glUniform3fv( self->uniforms[ slot ].slot, 1, value ) );
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_VEC4:
			XGL_CALL( glUniform4fv( self->uniforms[ slot ].slot, 1, value ) );
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_MAT3:
		{
			PLMatrix3 mat3 = *( PLMatrix3 * ) value;
			XGL_CALL( glUniformMatrix3fv( self->uniforms[ slot ].slot, 1, transpose ? GL_TRUE : GL_FALSE, mat3.m ) );
			break;
		}
		case QM_GFX_SHADER_UNIFORM_TYPE_MAT4:
		{
			PLMatrix4 mat4 = *( PLMatrix4 * ) value;
			XGL_CALL( glUniformMatrix4fv( self->uniforms[ slot ].slot, 1, transpose ? GL_TRUE : GL_FALSE, mat4.m ) );
			break;
		}
		default:
			break;
	}
}

static void xgl_shader_program_register_data( QmGfxShaderProgram *self )
{
	if ( self->uniforms != nullptr )
	{
		XGL_DEBUG( "Uniforms have already been initialised!\n" );
		return;
	}

	XglShaderProgram *drv = self->driver;
	XGL_CALL( drv->defaultUniforms[ XGL_SHADER_UNIFORM_TYPE_MODEL_MATRIX ] = ( unsigned int ) glGetUniformLocation( drv->id, "pl_model" ) );
	XGL_CALL( drv->defaultUniforms[ XGL_SHADER_UNIFORM_TYPE_VIEW_MATRIX ] = ( unsigned int ) glGetUniformLocation( drv->id, "pl_view" ) );
	XGL_CALL( drv->defaultUniforms[ XGL_SHADER_UNIFORM_TYPE_PROJECTION_MATRIX ] = ( unsigned int ) glGetUniformLocation( drv->id, "pl_proj" ) );
	XGL_CALL( drv->defaultUniforms[ XGL_SHADER_UNIFORM_TYPE_TEXTURE_MATRIX ] = ( unsigned int ) glGetUniformLocation( drv->id, "pl_texture" ) );
	XGL_CALL( drv->defaultUniforms[ XGL_SHADER_UNIFORM_TYPE_CLIP_PLANE ] = ( unsigned int ) glGetUniformLocation( drv->id, "pl_clipplane" ) );
	XGL_CALL( drv->defaultUniforms[ XGL_SHADER_UNIFORM_TYPE_CLIP_PLANE_MATRIX ] = ( unsigned int ) glGetUniformLocation( drv->id, "pl_clipplane_matrix" ) );

	int numUniforms = 0;
	XGL_CALL( glGetProgramiv( drv->id, GL_ACTIVE_UNIFORMS, &numUniforms ) );
	if ( numUniforms <= 0 )
	{
		/* true, because technically this isn't a fault - there just aren't any */
		XGL_DEBUG( "No uniforms found in shader program...\n" );
		return;
	}
	self->numUniforms = ( unsigned int ) numUniforms;

	XGL_DEBUG( "Found %u uniforms in shader\n", self->numUniforms );

	self->uniforms          = gInterface->core->CAlloc( ( size_t ) self->numUniforms, sizeof( *self->uniforms ), true );
	unsigned int registered = 0;
	for ( unsigned int i = 0; i < self->numUniforms; ++i )
	{
		int maxUniformNameLength;
		XGL_CALL( glGetActiveUniformsiv( drv->id, 1, &i, GL_UNIFORM_NAME_LENGTH, &maxUniformNameLength ) );

		GLchar *uniformName = gInterface->core->MAlloc( maxUniformNameLength, true );
		GLsizei nameLength;

		GLenum glType;
		GLint  uniformSize;

		XGL_CALL( glGetActiveUniform( drv->id, i, maxUniformNameLength, &nameLength, &uniformSize, &glType, uniformName ) );
		if ( nameLength == 0 )
		{
			gInterface->core->Free( uniformName );

			XGL_DEBUG( "No information available for uniform %d!\n", i );
			continue;
		}

		XGL_CALL( self->uniforms[ i ].slot = glGetUniformLocation( drv->id, uniformName ) );

		self->uniforms[ i ].type        = xgl_convert_gl_uniform_type( glType );
		self->uniforms[ i ].numElements = uniformSize;
		snprintf( self->uniforms[ i ].name, sizeof( self->uniforms[ i ].name ), "%s", uniformName );

		/* fetch it's current value, assume it's the default */
		switch ( self->uniforms[ i ].type )
		{
			case QM_GFX_SHADER_UNIFORM_TYPE_FLOAT:
				XGL_CALL( glGetUniformfv( drv->id, self->uniforms[ i ].slot, &self->uniforms[ i ].defaultFloat ) );
				break;
			case QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER2D:
			case QM_GFX_SHADER_UNIFORM_TYPE_INT:
				XGL_CALL( glGetUniformiv( drv->id, self->uniforms[ i ].slot, &self->uniforms[ i ].defaultInt ) );
				break;
			case QM_GFX_SHADER_UNIFORM_TYPE_UINT:
				XGL_CALL( glGetUniformuiv( drv->id, self->uniforms[ i ].slot, &self->uniforms[ i ].defaultUInt ) );
				break;
			case QM_GFX_SHADER_UNIFORM_TYPE_BOOL:
				XGL_CALL( glGetUniformiv( drv->id, self->uniforms[ i ].slot, ( GLint * ) &self->uniforms[ i ].defaultBool ) );
				break;
			case QM_GFX_SHADER_UNIFORM_TYPE_DOUBLE:
				XGL_CALL( glGetUniformdv( drv->id, self->uniforms[ i ].slot, &self->uniforms[ i ].defaultDouble ) );
				break;
			case QM_GFX_SHADER_UNIFORM_TYPE_VEC2:
				XGL_CALL( glGetUniformfv( drv->id, self->uniforms[ i ].slot, ( GLfloat * ) &self->uniforms[ i ].defaultVec2 ) );
				break;
			case QM_GFX_SHADER_UNIFORM_TYPE_VEC3:
				XGL_CALL( glGetUniformfv( drv->id, self->uniforms[ i ].slot, ( GLfloat * ) &self->uniforms[ i ].defaultVec3 ) );
				break;
			case QM_GFX_SHADER_UNIFORM_TYPE_VEC4:
				XGL_CALL( glGetUniformfv( drv->id, self->uniforms[ i ].slot, ( GLfloat * ) &self->uniforms[ i ].defaultVec4 ) );
				break;
			case QM_GFX_SHADER_UNIFORM_TYPE_MAT3:
				XGL_CALL( glGetUniformfv( drv->id, self->uniforms[ i ].slot, ( GLfloat * ) &self->uniforms[ i ].defaultMat3 ) );
				break;
			case QM_GFX_SHADER_UNIFORM_TYPE_MAT4:
				XGL_CALL( glGetUniformfv( drv->id, self->uniforms[ i ].slot, ( GLfloat * ) &self->uniforms[ i ].defaultMat4 ) );
				break;
			default:
				break;
		}

		XGL_DEBUG( " %4d (%20s) %s\n", i, self->uniforms[ i ].name, uniformDescriptors[ self->uniforms[ i ].type ] );

		registered++;
	}

	if ( registered == 0 )
	{
		XGL_DEBUG( "Failed to validate any shader program uniforms!\n" );
	}
}

static void xgl_shader_program_set( QmGfxShaderProgram *self )
{
	unsigned int id = 0;
	if ( self != nullptr )
	{
		id = ( ( XglShaderProgram * ) self->driver )->id;
	}

	XGL_CALL( glUseProgram( id ) );
}

static void xgl_shader_program_link( QmGfxShaderProgram *self )
{
	XglShaderProgram *drv = self->driver;
	XGL_CALL( glLinkProgram( drv->id ) );

	int status;
	XGL_CALL( glGetProgramiv( drv->id, GL_LINK_STATUS, &status ) );
	if ( status == 0 )
	{
		int length;
		XGL_CALL( glGetProgramiv( drv->id, GL_INFO_LOG_LENGTH, &length ) );
		if ( length > 1 )
		{
			char *log = gInterface->core->CAlloc( ( size_t ) length, sizeof( char ), true );
			XGL_CALL( glGetProgramInfoLog( drv->id, length, nullptr, log ) );
			XGL_DEBUG( " LINK ERROR:\n%s\n", log );
			gInterface->core->Free( log );
			gInterface->core->ReportError( PL_RESULT_SHADER_COMPILE, __FUNCTION__, log );
		}
		else
		{
			XGL_DEBUG( " UNKNOWN LINK ERROR!\n" );
			gInterface->core->ReportError( PL_RESULT_SHADER_COMPILE, __FUNCTION__, "unknown error" );
		}

		return;
	}

	self->is_linked = true;

	xgl_shader_program_register_data( self );
}

/////////////////////////////////////////////////////////////
// Stencil Operations

static GLenum xgl_translate_compare_function( PLGCompareFunction compareFunction )
{
	switch ( compareFunction )
	{
		default:
			return XGL_INVALID;
		case PLG_COMPARE_NEVER:
			return GL_NEVER;
		case PLG_COMPARE_LESS:
			return GL_LESS;
		case PLG_COMPARE_EQUAL:
			return GL_EQUAL;
		case PLG_COMPARE_LEQUAL:
			return GL_LEQUAL;
		case PLG_COMPARE_GREATER:
			return GL_GREATER;
		case PLG_COMPARE_NOTEQUAL:
			return GL_NOTEQUAL;
		case PLG_COMPARE_GEQUAL:
			return GL_GEQUAL;
		case PLG_COMPARE_ALWAYS:
			return GL_ALWAYS;
	}
}

static void xgl_depth_buffer_function( const PLGCompareFunction compareFunction )
{
	GLenum glCompare = xgl_translate_compare_function( compareFunction );
	assert( glCompare != XGL_INVALID );
	XGL_CALL( glDepthFunc( glCompare ) );
}

static void xgl_stencil_function( const PLGCompareFunction compareFunction, const int ref, const unsigned int mask )
{
	GLenum glCompare = xgl_translate_compare_function( compareFunction );
	assert( glCompare != XGL_INVALID );
	XGL_CALL( glStencilFunc( glCompare, ref, mask ) );
}

static GLenum xgl_translate_stencil_op( const PLGStencilOp stencilOp )
{
	switch ( stencilOp )
	{
		default:
			return XGL_INVALID;
		case PLG_STENCIL_OP_KEEP:
			return GL_KEEP;
		case PLG_STENCIL_OP_ZERO:
			return GL_ZERO;
		case PLG_STENCIL_OP_REPLACE:
			return GL_REPLACE;
		case PLG_STENCIL_OP_INCR:
			return GL_INCR;
		case PLG_STENCIL_OP_INCRWRAP:
			return GL_INCR_WRAP;
		case PLG_STENCIL_OP_DECR:
			return GL_DECR;
		case PLG_STENCIL_OP_DECRWRAP:
			return GL_DECR_WRAP;
		case PLG_STENCIL_OP_INVERT:
			return GL_INVERT;
	}
}

static GLenum xgl_translate_stencil_face( PLGStencilFace face )
{
	switch ( face )
	{
		default:
			return XGL_INVALID;
		case PLG_STENCIL_FACE_FRONT:
			return GL_FRONT;
		case PLG_STENCIL_FACE_BACK:
			return GL_BACK;
		case PLG_STENCIL_FACE_FRONTANDBACK:
			return GL_FRONT_AND_BACK;
	}
}

static void xgl_stencil_op( PLGStencilFace face, PLGStencilOp stencilFailOp, PLGStencilOp depthFailOp, PLGStencilOp depthPassOp )
{
	GLenum glface = xgl_translate_stencil_face( face );
	if ( glface == XGL_INVALID )
	{
		gInterface->core->ReportError( PL_RESULT_FAIL, __FUNCTION__, "invalid stencil face specified" );
		return;
	}

	GLenum sfail = xgl_translate_stencil_op( stencilFailOp );
	if ( sfail == XGL_INVALID )
	{
		gInterface->core->ReportError( PL_RESULT_FAIL, __FUNCTION__, "invalid stencil fail operation" );
		return;
	}

	GLenum dpfail = xgl_translate_stencil_op( depthFailOp );
	if ( sfail == XGL_INVALID )
	{
		gInterface->core->ReportError( PL_RESULT_FAIL, __FUNCTION__, "invalid depth fail operation" );
		return;
	}

	GLenum dppass = xgl_translate_stencil_op( depthPassOp );
	if ( sfail == XGL_INVALID )
	{
		gInterface->core->ReportError( PL_RESULT_FAIL, __FUNCTION__, "invalid depth pass operation" );
		return;
	}

	XGL_CALL( glStencilOpSeparate( glface, sfail, dpfail, dppass ) );
}

/////////////////////////////////////////////////////////////
// Generic State Management

static unsigned int xgl_translate_graphics_state( PLGDrawState state )
{
	switch ( state )
	{
		default:
			break;
		case PLG_GFX_STATE_FOG:
			if ( XGL_VERSION( 3, 0 ) )
			{
				return 0;
			}
			return GL_FOG;
		case PLG_GFX_STATE_ALPHATEST:
			if ( XGL_VERSION( 3, 0 ) )
			{
				return 0;
			}
			return GL_ALPHA_TEST;
		case PLG_GFX_STATE_BLEND:
			return GL_BLEND;
		case PLG_GFX_STATE_DEPTHTEST:
			return GL_DEPTH_TEST;
		case PLG_GFX_STATE_STENCILTEST:
			return GL_STENCIL_TEST;
		case PLG_GFX_STATE_MULTISAMPLE:
			return GL_MULTISAMPLE;
		case PLG_GFX_STATE_ALPHATOCOVERAGE:
			return GL_SAMPLE_ALPHA_TO_COVERAGE;
		case PLG_GFX_STATE_DEPTH_CLAMP:
			return GL_DEPTH_CLAMP;
	}

	return 0;
}

static void xgl_enable_state( PLGDrawState state )
{
	unsigned int gl_state = xgl_translate_graphics_state( state );
	if ( !gl_state )
	{
		if ( state == PLG_GFX_STATE_WIREFRAME )
		{
			XGL_CALL( glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ) );
		}

		/* probably unsupported */
		return;
	}

	XGL_CALL( glEnable( gl_state ) );
}

static void xgl_disable_state( PLGDrawState state )
{
	unsigned int gl_state = xgl_translate_graphics_state( state );
	if ( !gl_state )
	{
		if ( state == PLG_GFX_STATE_WIREFRAME )
		{
			XGL_CALL( glPolygonMode( GL_FRONT_AND_BACK, GL_FILL ) );
		}

		/* probably unsupported */
		return;
	}

	XGL_CALL( glDisable( gl_state ) );
}

/////////////////////////////////////////////////////////////

static char gl_extensions[ 4096 ][ 4096 ] = { { '\0' } };

#if defined( DEBUG_GL )
static void xgl_message_callback(
        [[maybe_unused]] GLenum  source,
        GLenum                   type,
        [[maybe_unused]] GLuint  id,
        GLenum                   severity,
        [[maybe_unused]] GLsizei length,
        const GLchar            *message,
        [[maybe_unused]] void   *param )
{
	if ( severity == GL_DEBUG_SEVERITY_LOW )
	{
		return;
	}

	const char *s_severity;
	switch ( severity )
	{
		case GL_DEBUG_SEVERITY_HIGH:
			s_severity = "HIGH";
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			s_severity = "MEDIUM";
			break;
		case GL_DEBUG_SEVERITY_LOW:
			s_severity = "LOW";
			break;

		default:
			return;
	}

	const char *s_type;
	switch ( type )
	{
		case GL_DEBUG_TYPE_ERROR:
			s_type = "ERROR";
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			s_type = "DEPRECATED";
			break;
		case GL_DEBUG_TYPE_MARKER:
			s_type = "MARKER";
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			s_type = "PERFORMANCE";
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			s_type = "PORTABILITY";
			break;
		default:
			s_type = "OTHER";
			break;
	}

	if ( message != nullptr && message[ 0 ] != '\0' )
	{
		XGL_DEBUG( "(%s) %s - %s\n", s_type, s_severity, message );
	}
}
#endif

static PLFunctionResult xgl_initialize()
{
	glewExperimental = true;
	GLenum err       = glewInit();
	// under wayland, glx requests aren't available; https://github.com/nigels-com/glew/issues/172
	if ( err != GLEW_OK && err != GLEW_ERROR_NO_GLX_DISPLAY )
	{
		gInterface->core->ReportError( PL_RESULT_GRAPHICSINIT, __FUNCTION__, "failed to initialize glew, %s", ( char * ) glewGetErrorString( err ) );
		return PL_RESULT_GRAPHICSINIT;
	}

	const char *version = ( const char * ) glGetString( GL_VERSION );
	xgl_versionMajor    = version[ 0 ] - '0';
	xgl_versionMinor    = version[ 2 ] - '0';

	if ( XGL_VERSION( 3, 0 ) )
	{
		int minor, major;
		glGetIntegerv( GL_MAJOR_VERSION, &major );
		glGetIntegerv( GL_MINOR_VERSION, &minor );
		if ( major > 0 )
		{
			xgl_versionMajor = major;
			xgl_versionMinor = minor;
		}
		else
		{
			XGL_DEBUG( "failed to get OpenGL version, expect some functionality not to work!\n" );
		}
	}

	XGL_DEBUG( " OpenGL %d.%d\n", xgl_versionMajor, xgl_versionMinor );
	XGL_DEBUG( "  renderer:   %s\n", ( const char * ) glGetString( GL_RENDERER ) );
	XGL_DEBUG( "  vendor:     %s\n", ( const char * ) glGetString( GL_VENDOR ) );
	XGL_DEBUG( "  version:    %s\n", version );

#if defined( DEBUG_GL )
	XGL_DEBUG( "  extensions:\n" );
	unsigned int numExtensions;
	XGL_CALL( glGetIntegerv( GL_NUM_EXTENSIONS, ( GLint * ) &numExtensions ) );
	for ( unsigned int i = 0; i < numExtensions; ++i )
	{
		const char *extension = ( char * ) glGetStringi( GL_EXTENSIONS, i );
		snprintf( gl_extensions[ i ], sizeof( gl_extensions[ i ] ), "%s", extension );
		XGL_DEBUG( "    %s\n", extension );
	}
#endif

	if ( !XGL_VERSION( 4, 6 ) )
	{
		gInterface->core->ReportError( PL_RESULT_GRAPHICSINIT, __FUNCTION__, "unsupported hardware (4.6 required)" );
		return PL_RESULT_GRAPHICSINIT;
	}

#if defined( DEBUG_GL )
	XGL_CALL( glEnable( GL_DEBUG_OUTPUT ) );
	XGL_CALL( glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS ) );

	XGL_CALL( glDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE ) );
	XGL_CALL( glDebugMessageCallback( ( GLDEBUGPROC ) xgl_message_callback, nullptr ) );
#endif

	XGL_CALL( glEnable( GL_TEXTURE_CUBE_MAP_SEAMLESS ) );

	xgl_mesh_vao_manager_initialize();

	return PL_RESULT_SUCCESS;
}

static void xgl_shutdown()
{
	xgl_mesh_vao_manager_shutdown();

#if defined( DEBUG_GL )
	XGL_CALL( glDisable( GL_DEBUG_OUTPUT ) );
	XGL_CALL( glDisable( GL_DEBUG_OUTPUT_SYNCHRONOUS ) );
#endif
}

/////////////////////////////////////////////////////////////

PLGDriverImportTable graphicsInterface = {
        .Initialize = xgl_initialize,
        .Shutdown   = xgl_shutdown,

        .InsertDebugMarker    = xgl_debug_insert_marker,
        .PushDebugGroupMarker = xgl_debug_push_group_marker,
        .PopDebugGroupMarker  = xgl_debug_pop_group_marker,

        .GetMaxTextureUnits = xgl_get_max_texture_units,
        .GetMaxTextureSize  = xgl_get_max_texture_size,

        .EnableState  = xgl_enable_state,
        .DisableState = xgl_disable_state,

        .SetBlendMode = xgl_set_blend_mode,
        .SetCullMode  = xgl_set_cull_mode,

        .SetClearColour = xgl_set_clear_colour,
        .ClearBuffers   = xgl_clear_buffers,

        .SetDepthBufferMode = xgl_set_depth_buffer_mode,

        .DepthMask   = xgl_depth_mask,
        .ColourMask  = xgl_colour_mask,
        .StencilMask = xgl_stencil_mask,

        .CreateMesh        = xgl_mesh_create,
        .UploadMesh        = xgl_mesh_upload,
        .DrawMesh          = xgl_mesh_draw,
        .DrawInstancedMesh = xgl_mesh_draw_instanced,
        .DeleteMesh        = xgl_mesh_delete,

        .CreateFrameBuffer               = xgl_fbo_create,
        .DeleteFrameBuffer               = xgl_fbo_destroy,
        .BindFrameBuffer                 = xgl_fbo_bind,
        .GetFrameBufferTextureAttachment = xgl_fbo_create_texture_attachment,
        .BlitFrameBuffers                = xgl_fbo_blit,
        .SetFrameBufferSize              = xgl_fbo_set_size,
        .ReadFrameBufferRegion           = xgl_fbo_read_region,

        .CreateTexture        = xgl_texture_create,
        .DeleteTexture        = xgl_texture_destroy,
        .BindTexture          = xgl_texture_bind,
        .UploadTexture        = xgl_texture_upload,
        .SetTextureAnisotropy = xgl_texture_set_anisotropy,
        .SetTextureFilter     = xgl_texture_set_filter,
        .SetTextureWrapMode   = xgl_texture_set_wrap,

        .ClipViewport = xgl_clip_viewport,
        .SetViewport  = xgl_set_viewport,

        .CreateShaderProgram   = xgl_shader_program_create,
        .DestroyShaderProgram  = xgl_shader_program_destroy,
        .AttachShaderStage     = xgl_shader_program_attach_stage,
        .LinkShaderProgram     = xgl_shader_program_link,
        .SetShaderProgram      = xgl_shader_program_set,
        .CreateShaderStage     = xgl_shader_stage_create,
        .DestroyShaderStage    = xgl_shader_stage_destroy,
        .CompileShaderStage    = xgl_shader_stage_compile,
        .SetShaderUniformValue = xgl_shader_program_set_uniform,

        .DepthBufferFunction   = xgl_depth_buffer_function,
        .StencilBufferFunction = xgl_stencil_function,
        .StencilOp             = xgl_stencil_op,

        .SetClipPlane = xgl_set_clip_plane,
};
