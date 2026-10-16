// SPDX-License-Identifier: MIT
// Copyright © 2017-2024 Mark E Sowden <hogsy@oldtimes-software.com>

#include "plcore/pl_image.h"
#include "qmos/public/qm_os_memory.h"

// Quickly whipped up loader for 3D Realms' TEX format

QmImage *qm_image_3dr_parse( QmFsFile *file ) {
	// possibly some sort of image mode? non-zero for alpha textures
	uint16_t mode = QM_FS_FILE_READ_UINT16( file, false, NULL );
	if ( mode != 0 && mode != 0x500 ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "unexpected mode" );
		return NULL;
	}

	uint16_t numMipmaps = QM_FS_FILE_READ_UINT16( file, false, NULL );
	if ( numMipmaps == 0 ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "unexpected number of mipmaps" );
		return NULL;
	}

	uint16_t width = QM_FS_FILE_READ_UINT16( file, false, NULL );
	uint16_t height = QM_FS_FILE_READ_UINT16( file, false, NULL );
	if ( width == 0 || height == 0 ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "invalid image dimensions" );
		return NULL;
	}

	// no idea...
	qm_fs_file_seek( file, 28, QM_FS_SEEK_CUR );

	// not sure why we've got this again, but make sure it matches for some validation
	uint16_t w2 = QM_FS_FILE_READ_UINT16( file, false, NULL );
	uint16_t h2 = QM_FS_FILE_READ_UINT16( file, false, NULL );
	if ( w2 != width || h2 != height ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "secondary image dimensions do not match" );
		return NULL;
	}

	// again, no idea...
	qm_fs_file_seek( file, 12, QM_FS_SEEK_CUR );

	unsigned int size = width * height;
	uint16_t *src = QM_OS_MEMORY_NEW_( uint16_t, size );
	if ( qm_file_read( file, src, sizeof( uint16_t ), size ) != size ) {
		qm_os_memory_free( src );
		return NULL;
	}

	QmMathColour4ub *dst = QM_OS_MEMORY_NEW_( QmMathColour4ub, size );
	if ( mode == 0x500 ) {
		const unsigned int shiftA = 12;
		const unsigned int shiftR = 8;
		const unsigned int shiftG = 4;
		const unsigned int shiftB = 0;

		const unsigned int mask = 15;

		for ( unsigned int i = 0; i < size; i++ ) {
			dst[ i ].a = ( ( src[ i ] & ( mask << shiftA ) ) >> shiftA ) * 255 / mask;
			dst[ i ].r = ( ( src[ i ] & ( mask << shiftR ) ) >> shiftR ) * 255 / mask;
			dst[ i ].g = ( ( src[ i ] & ( mask << shiftG ) ) >> shiftG ) * 255 / mask;
			dst[ i ].b = ( ( src[ i ] & ( mask << shiftB ) ) >> shiftB ) * 255 / mask;
		}
	} else {
		const unsigned int shiftR = 11;
		const unsigned int shiftG = 5;
		const unsigned int shiftB = 0;

		const unsigned int maskR = 31;
		const unsigned int maskG = 63;
		const unsigned int maskB = 31;

		for ( unsigned int i = 0; i < size; i++ ) {
			dst[ i ].a = 255;
			dst[ i ].r = ( ( src[ i ] & ( maskR << shiftR ) ) >> shiftR ) * 255 / maskR;
			dst[ i ].g = ( ( src[ i ] & ( maskG << shiftG ) ) >> shiftG ) * 255 / maskG;
			dst[ i ].b = ( ( src[ i ] & ( maskB << shiftB ) ) >> shiftB ) * 255 / maskB;
		}
	}

	QmImage *image = PlCreateImage( dst, width, height, 0, PL_COLOURFORMAT_RGBA, PL_IMAGEFORMAT_RGBA8 );

	qm_os_memory_free( src );
	qm_os_memory_free( dst );

	return image;
}
