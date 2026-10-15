// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "qmos/public/qm_os_memory.h"

#include <plgraphics/plg.h>
#include <plgraphics/qm_gfx_driver.h>
#include <plgraphics/plg_texture.h>

PL_EXTERN_C

#if !defined( NDEBUG )
#	define GfxLog( FORMAT, ... ) printf( FORMAT, ##__VA_ARGS__ )
#else
#	define GfxLog( ... )
#endif

typedef struct GfxState
{
	const QmGfxDriverImportTable *interface;

	PLGCullMode current_cullmode;

	QmMathColour4ub current_clearcolour;
	QmMathColour4ub current_colour;// Current global colour.

	bool         current_capabilities[ PLG_GFX_MAX_STATES ];// Enabled capabilities.
	unsigned int current_textureunit;

	// Textures

	PLGTextureMappingUnit *tmu;

	// Shader states

	QmGfxShaderProgram *current_program;
	PLPath              shaderCacheLocation; /* where shaders should be cached to, if supported */

	// Hardware / Driver information

	const char *hw_vendor;
	const char *hw_renderer;
	const char *hw_version;
	const char *hw_extensions;

	unsigned int hw_maxtexturesize;
	unsigned int hw_maxtextureunits;
	unsigned int hw_maxtextureanistropy;

	PLMatrix4 projection_matrix;
	PLMatrix4 view_matrix;

	////////////////////////////////////////

	struct
	{
		int x, y;
		int w, h;
	} viewport;

	bool mode_debug;
} GfxState;

#define CallGfxFunction( FUNCTION, ... )                                   \
	if ( gfx_state.interface != NULL )                                     \
	{                                                                      \
		if ( gfx_state.interface->FUNCTION != NULL )                       \
			gfx_state.interface->FUNCTION( __VA_ARGS__ );                  \
		else                                                               \
			GfxLog( "Unbound layer function %s was called\n", #FUNCTION ); \
	}
#define CallReturningGfxFunction( FUNCTION, RETURN, ... )                  \
	if ( gfx_state.interface != NULL )                                     \
	{                                                                      \
		if ( gfx_state.interface->FUNCTION != NULL )                       \
		{                                                                  \
			return gfx_state.interface->FUNCTION( __VA_ARGS__ );           \
		}                                                                  \
		else                                                               \
		{                                                                  \
			GfxLog( "Unbound layer function %s was called\n", #FUNCTION ); \
			return ( RETURN );                                             \
		}                                                                  \
	}                                                                      \
	else                                                                   \
	{                                                                      \
		return ( RETURN );                                                 \
	}

PL_EXTERN GfxState gfx_state;

PL_EXTERN_C_END
