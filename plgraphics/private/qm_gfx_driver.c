// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Driver interfaces.
// Author:  Mark E. Sowden

#include <plgraphics/qm_gfx_driver.h>

#include "plg_private.h"

#include "qmos/public/qm_os_library.h"
#include "qmos/public/qm_os_memory.h"

#if defined( QM_GFX_OPENGL )
extern const QmGfxDriverImportTable qm_gfx_xglInterface_;
#endif
static const QmGfxDriverImportTable *driverInterfaces[ QM_GFX_DRIVER_INTERFACE_MAX ] = {
#if defined( QM_GFX_OPENGL )
        [QM_GFX_DRIVER_INTERFACE_OPENGL] = &qm_gfx_xglInterface_,
#endif
};

static PLGDriverExportTable exportTable = {
        .CreateTexture = qm_gfx_texture_create,
};

//TEMPORARY!!!!
extern const PLGDriverExportTable *gInterface;

PLFunctionResult qm_gfx_driver_set( const unsigned int mode )
{
	exportTable.core = PlGetExportTable();
	//TEMPORARY!!!!
	gInterface = &exportTable;
	//TEMPORARY!!!!

	const QmGfxDriverImportTable *interface = driverInterfaces[ mode ];
	if ( interface == NULL )
	{
		PlReportErrorF( PL_RESULT_GRAPHICSINIT, "invalid graphics interface \"%s\" selected", mode );
		return PL_RESULT_GRAPHICSINIT;
	}

	if ( gfx_state.interface != NULL && interface == gfx_state.interface )
	{
		PlReportErrorF( PL_RESULT_GRAPHICSINIT, "chosen interface \"%s\" is already active", mode );
		return PL_RESULT_GRAPHICSINIT;
	}

	gfx_state.interface = interface;
	if ( gfx_state.interface->Initialize == NULL )
	{
		PlReportErrorF( PL_RESULT_GRAPHICSINIT, "no initialization function bound for graphics interface \"%s\"", mode );
		return PL_RESULT_GRAPHICSINIT;
	}

	PLFunctionResult result;
	if ( ( result = gfx_state.interface->Initialize() ) != PL_RESULT_SUCCESS )
	{
		return result;
	}

	//TODO: do we still need this other crap??

	qm_os_memory_free( gfx_state.tmu );

	const unsigned int numUnits = qm_gfx_get_max_texture_units();
	gfx_state.tmu               = ( PLGTextureMappingUnit * ) QM_OS_MEMORY_CALLOC( numUnits, sizeof( PLGTextureMappingUnit ) );
	for ( unsigned int i = 0; i < numUnits; i++ )
	{
		gfx_state.tmu[ i ].current_envmode = PLG_TEXTUREMODE_REPLACE;
	}

	return PL_RESULT_SUCCESS;
}
