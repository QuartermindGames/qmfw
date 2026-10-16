// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Nexus/Plasma HSM loader.
// Author:  Mark E. Sowden

#include "image_private.h"

/**
 * Nexus, an early prototype using Plasma, seems to use HSM versions 0 and 1.
 * Format is incredibly simple as far as I can tell. No compression?
 *
 * Uru Online seems to use version 2 which has compression. I've not bothered
 * looking too much at that yet.
 */

static constexpr unsigned int HSM_MAX_VERSION = 1;

QmImage *qm_image_hsm_parse( QmFsFile *file )
{
	bool     status;
	uint32_t version = QM_FS_FILE_READ_UINT32( file, true, &status );
	uint32_t width   = QM_FS_FILE_READ_UINT32( file, true, &status );
	uint32_t height  = QM_FS_FILE_READ_UINT32( file, true, &status );
	if ( !status )
	{
		return nullptr;
	}

	if ( version > HSM_MAX_VERSION )
	{
		PlReportErrorF( PL_RESULT_FILEVERSION, "unexpected version (%u != %u)", version, HSM_MAX_VERSION );
		return nullptr;
	}

	if ( width == 0 )
	{
		PlReportErrorF( PL_RESULT_FILEERR, "invalid width (%u)", width );
		return nullptr;
	}

	if ( height == 0 )
	{
		PlReportErrorF( PL_RESULT_FILEERR, "invalid height (%u)", height );
		return nullptr;
	}

	// not currently sure of these...
	QM_FS_FILE_READ_UINT32( file, true, &status );
	QM_FS_FILE_READ_UINT32( file, true, &status );
	QM_FS_FILE_READ_UINT16( file, true, &status );
	if ( !status )
	{
		return nullptr;
	}

	unsigned int size = width * height * 4;
	uint8_t     *buf  = QM_OS_MEMORY_NEW_( uint8_t, size );
	if ( qm_file_read( file, buf, sizeof( uint8_t ), size ) != size )
	{
		qm_os_memory_free( buf );
		return nullptr;
	}

	QmImage *image = PlCreateImage( buf, width, height, 0, PL_COLOURFORMAT_RGBA, PL_IMAGEFORMAT_RGBA8 );

	qm_os_memory_free( buf );
	return image;
}
