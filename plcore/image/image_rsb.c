// SPDX-License-Identifier: MIT
// Copyright © 2017-2024 Mark E Sowden <hogsy@oldtimes-software.com>

#include "image_private.h"
#include "qmos/public/qm_os_memory.h"

// Red Storm Entertainment RSB Format

static const uint32_t RSB_VERSION_MIN = 0;
static const uint32_t RSB_VERSION_MAX = 1;

typedef struct RGBA {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} RGBA;
typedef RGBA Palette[ 256 ];

QmImage *qm_image_rsb_parse( QmFsFile *file ) {
	uint32_t version = QM_FS_FILE_READ_UINT32( file, false, NULL );
	if ( version < RSB_VERSION_MIN || version > RSB_VERSION_MAX ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "unexpected version (%u)", version );
		return NULL;
	}

	// size
	uint32_t width = QM_FS_FILE_READ_UINT32( file, false, NULL );
	uint32_t height = QM_FS_FILE_READ_UINT32( file, false, NULL );
	if ( width == 0 || height == 0 ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "invalid image dimensions (%ux%u)", width, height );
		return NULL;
	}

	QmImage *image = NULL;

	bool hasPalette = false;
	if ( version == 0 ) {
		hasPalette = ( bool ) QM_FS_FILE_READ_UINT32( file, false, NULL );
	}

	if ( hasPalette ) {
		Palette palette = {};
		qm_file_read( file, palette, sizeof( RGBA ), 256 );

		unsigned int size = width * height;
		uint8_t *src = QM_OS_MEMORY_NEW_( uint8_t, size );
		if ( qm_file_read( file, src, sizeof( uint8_t ), size ) == size ) {
			QmMathColour4ub *dst = QM_OS_MEMORY_NEW_( QmMathColour4ub, size );
			for ( unsigned int i = 0; i < size; ++i ) {
				dst[ i ].r = palette[ src[ i ] ].b;
				dst[ i ].g = palette[ src[ i ] ].g;
				dst[ i ].b = palette[ src[ i ] ].r;
				// no idea...
				dst[ i ].a = ( palette[ src[ i ] ].a == 0 ) ? 255 : 0;
			}

			image = PlCreateImage( dst, width, height, 0, PL_COLOURFORMAT_RGBA, PL_IMAGEFORMAT_RGBA8 );

			qm_os_memory_free( dst );
		}

		qm_os_memory_free( src );
	} else {
		uint32_t r = QM_FS_FILE_READ_UINT32( file, false, NULL );
		uint32_t g = QM_FS_FILE_READ_UINT32( file, false, NULL );
		uint32_t b = QM_FS_FILE_READ_UINT32( file, false, NULL );
		uint32_t a = QM_FS_FILE_READ_UINT32( file, false, NULL );

		unsigned int size = width * height;
		uint16_t *src = QM_OS_MEMORY_NEW_( uint16_t, size );
		if ( qm_file_read( file, src, sizeof( uint16_t ), size ) == size ) {
			QmMathColour4ub *dst = QM_OS_MEMORY_NEW_( QmMathColour4ub, size );
			uint32_t maskR = ( 1 << r ) - 1;
			uint32_t maskG = ( 1 << g ) - 1;
			uint32_t maskB = ( 1 << b ) - 1;
			uint32_t maskA = ( 1 << a ) - 1;

			uint32_t shiftA = r + g + b;
			uint32_t shiftR = g + b;
			uint32_t shiftG = b;
			uint32_t shiftB = 0;

			for ( unsigned int i = 0; i < size; i++ ) {
				dst[ i ].r = ( ( src[ i ] & ( maskR << shiftR ) ) >> shiftR ) * 255 / maskR;
				dst[ i ].g = ( ( src[ i ] & ( maskG << shiftG ) ) >> shiftG ) * 255 / maskG;
				dst[ i ].b = ( ( src[ i ] & ( maskB << shiftB ) ) >> shiftB ) * 255 / maskB;
				dst[ i ].a = a ? ( ( src[ i ] & ( maskA << shiftA ) ) >> shiftA ) * 255 / maskA : 255;
			}

			image = PlCreateImage( dst, width, height, 0, PL_COLOURFORMAT_RGBA, PL_IMAGEFORMAT_RGBA8 );

			qm_os_memory_free( dst );
		}

		qm_os_memory_free( src );
	}

	return image;
}
