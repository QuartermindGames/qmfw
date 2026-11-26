/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include "image_private.h"
#include "qmos/public/qm_os_memory.h"

/**
 * 3dfx 3DF Loader
 * Added purely because Shadow Man 97 uses this format,
 * and done the bare minimum to get it working with that.
 **/

static PLImageFormat FD3_GetImageFormat( const char *formatStr ) {
	if ( strcmp( "argb1555\n", formatStr ) == 0 ) {
		return PL_IMAGEFORMAT_RGB5A1;
	} else if ( strcmp( "argb4444\n", formatStr ) == 0 ) {
		return PL_IMAGEFORMAT_RGBA4;
	} else if ( strcmp( "rgb565\n", formatStr ) == 0 ) {
		return PL_IMAGEFORMAT_RGB565;
	}

	return PL_IMAGEFORMAT_UNKNOWN;
}

QmImage *qm_image_3df_parse( QmFsFile *file ) {
	/* read in the header */
	char buf[ 64 ];

	/* identifier */
	if ( qm_fs_file_read_string( file, buf, sizeof( buf ) ) == NULL ) {
		return NULL;
	}
	if ( strncmp( buf, "3df ", 4 ) != 0 ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "invalid identifier, expected \"3df \"" );
		return NULL;
	}

	/* image format */
	qm_fs_file_read_string( file, buf, sizeof( buf ) );
	PLImageFormat dataFormat = FD3_GetImageFormat( buf );
	if ( dataFormat == PL_IMAGEFORMAT_UNKNOWN ) {
		PlReportErrorF( PL_RESULT_IMAGEFORMAT, "unsupported image format, \"%s\"", buf );
		return NULL;
	}

	/* lod */
	if ( qm_fs_file_read_string( file, buf, sizeof( buf ) ) == NULL ) {
		return NULL;
	}
	int w, h;
	if ( sscanf( buf, "lod range: %d %d\n", &w, &h ) != 2 ) {
		PlReportErrorF( PL_RESULT_FILEREAD, "failed to read lod range" );
		return NULL;
	}
	if ( w <= 0 || h <= 0 || w > 256 || h > 256 ) {
		PlReportBasicError( PL_RESULT_IMAGERESOLUTION );
		return NULL;
	}

	/* aspect */
	if ( qm_fs_file_read_string( file, buf, sizeof( buf ) ) == NULL ) {
		return NULL;
	}
	int x, y;
	if ( sscanf( buf, "aspect ratio: %d %d\n", &x, &y ) != 2 ) {
		PlReportErrorF( PL_RESULT_FILEREAD, "failed to read aspect ratio" );
		return NULL;
	}

	switch ( ( x << 4 ) | ( y ) ) {
		case 0x81:
			h = h / 8;
			break;
		case 0x41:
			h = h / 4;
			break;
		case 0x21:
			h = h / 2;
			break;
		case 0x11:
			h = h / 1;
			break;
		case 0x12:
			w = w / 2;
			break;
		case 0x14:
			w = w / 4;
			break;
		case 0x18:
			w = w / 8;
			break;
		default:
			PlReportErrorF( PL_RESULT_FAIL, "unexpected aspect-ratio: %dx%d", x, y );
			return NULL;
	}

	/* now we can load the actual data in */
	size_t srcSize = qm_image_get_format_size( dataFormat, w, h );
	uint8_t *srcBuf = QM_OS_MEMORY_MALLOC_( srcSize );
	if ( qm_file_read( file, srcBuf, sizeof( char ), srcSize ) != srcSize ) {
		qm_os_memory_free( srcBuf );
		return NULL;
	}

	/* convert it... */
	size_t dstSize = qm_image_get_format_size( PL_IMAGEFORMAT_RGBA8, w, h );
	uint8_t *dstBuf = QM_OS_MEMORY_MALLOC_( dstSize );
	if ( dataFormat != PL_IMAGEFORMAT_RGBA8 ) {
		switch ( dataFormat ) {
			case PL_IMAGEFORMAT_RGB5A1: {
				uint8_t *dstPos = dstBuf;
				for ( size_t i = 0; i < srcSize; i += 2 ) {
					dstPos[ PL_RED ] = ( ( srcBuf[ i ] & 124 ) << 1 );
					dstPos[ PL_GREEN ] = ( ( srcBuf[ i ] & 3 ) << 6 ) | ( ( srcBuf[ i + 1 ] & 224 ) >> 2 );
					dstPos[ PL_BLUE ] = ( ( srcBuf[ i + 1 ] & 31 ) << 3 );
					dstPos[ PL_ALPHA ] = ( srcBuf[ i ] & 128 ) ? 0 : 255;
					dstPos += 4;
				}

				qm_os_memory_free( srcBuf );
				break;
			}
			default:
				qm_os_memory_free( dstBuf );
				dstBuf = srcBuf;
				break;
		}
	}

	QmImage *image = PlCreateImage( dstBuf, w, h, 0, PL_COLOURFORMAT_RGBA, PL_IMAGEFORMAT_RGBA8 );

	/* no longer need this */
	qm_os_memory_free( dstBuf );

	return image;
}
