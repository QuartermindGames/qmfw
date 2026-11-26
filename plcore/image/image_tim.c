/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include "image_private.h"
#include "qmos/public/qm_os_memory.h"

/*  http://rewiki.regengedanken.de/wiki/.TIM
 *  https://mrclick.zophar.net/TilEd/download/timgfx.txt
 */

typedef struct TIMHeader {
	uint8_t flag1;
	uint8_t flag2;
	uint8_t flag3;
	uint8_t flag4;
} TIMHeader;

#define TIM_FLAG1_TYPE_MASK ( 1 | 2 )
#define TIM_FLAG1_CLP 8

typedef struct TIMPaletteInfo {
	uint32_t palette_size; /* Length of the palette header and pixel data, in bytes. */
	uint16_t palette_org_x;
	uint16_t palette_org_y;
	uint16_t palette_width;  /* "width" and "height" of palette, multiply these together to get the */
	uint16_t palette_height; /* ...length of the palette in 16-bit words. */
} TIMPaletteInfo;

typedef struct TIMImageInfo {
	uint32_t image_size; /* Length of image header and pixel data, in bytes */
	uint16_t org_x;
	uint16_t org_y;
	uint16_t width;  /* Length of each row, in 16-bit words */
	uint16_t height; /* Height of the image, in pixels */
} TIMImageInfo;

enum TIMType {
	TIM_TYPE_4BPP = 0,
	TIM_TYPE_8BPP = 1,
	TIM_TYPE_16BPP = 2,
	TIM_TYPE_24BPP = 3,
};

#define TIM_IDENT 16

static bool TIM_FormatCheck( QmFsFile *fin ) {
	qm_fs_file_rewind( fin );

	uint32_t ident;
	if ( qm_file_read( fin, &ident, sizeof( uint32_t ), 1 ) != 1 ) {
		return false;
	}

	return ( bool ) ( ident == TIM_IDENT );
}

/* Convert a 16-bit TIM colour value to RGB5A1. */
static uint16_t _tim16toRGB51A( uint16_t colour_in ) {
	uint16_t colour_out = 0;

	uint8_t *cin = ( uint8_t * ) ( &colour_in );
	uint8_t *cout = ( uint8_t * ) ( &colour_out );

	/* Shift the colour bits around:
     * GGGRRRRR:ABBBBBGG => RRRRRGGG:GGBBBBBA
    */

	/* Red */
	cout[ 0 ] |= ( cin[ 0 ] & 0x1F ) << 3;

	/* Green */
	cout[ 0 ] |= ( cin[ 1 ] & 0x03 ) << 1;
	cout[ 0 ] |= ( cin[ 0 ] & 0x80 ) >> 7;
	cout[ 1 ] |= ( cin[ 0 ] & 0x60 ) << 1;

	/* Blue */
	cout[ 1 ] |= ( cin[ 1 ] & 0x7C ) >> 1;

	/* Handle the alpha channel... if the "STP" bit in the TIM data is on, the colour is
     * transparent, unless the colour is black, in which case the bit is inverted.
     */

	bool is_black = ( colour_out == 0 );
	bool stp_on = ( cin[ 1 ] & 0x80 );

	if ( ( is_black && stp_on ) || ( !is_black && !stp_on ) ) {
		cout[ 1 ] |= 0x01;
	}

	return colour_out;
}

static bool TIM_ReadFile( QmFsFile *fin, QmImage *out ) {
	if ( !TIM_FormatCheck( fin ) ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "invalid/unexpected identifier for TIM" );
		return false;
	}

	uint16_t *palette = NULL;
	uint8_t *image_data = NULL;

	TIMHeader header;
	if ( qm_file_read( fin, &header, sizeof( TIMHeader ), 1 ) != 1 ) {
		goto UNEXPECTED_EOF;
	}

	uint32_t palette_size = 0; /* Number of colours in palette */

	if ( header.flag1 & TIM_FLAG1_CLP ) {
		/* File has a palette (CLUT), read it it in */

		TIMPaletteInfo palette_info;
		if ( qm_file_read( fin, &palette_info, sizeof( TIMPaletteInfo ), 1 ) != 1 ) {
			goto UNEXPECTED_EOF;
		}

		palette_size = ( ( uint32_t ) ( palette_info.palette_width ) ) * ( ( uint32_t ) ( palette_info.palette_height ) );

		/* Check the size and width/height values in the header match. */
		if ( palette_size >= palette_info.palette_size || ( palette_size * sizeof( uint16_t ) ) != ( palette_info.palette_size - sizeof( palette_info ) ) ) {
			PlReportErrorF( PL_RESULT_FILETYPE, "invalid size/width/height in TIM palette header" );
			goto ERR_CLEANUP;
		}

		palette = QM_OS_MEMORY_CALLOC( palette_size, sizeof( uint16_t ) );
		if ( palette == NULL ) {
			goto ERR_CLEANUP;
		}

		if ( qm_file_read( fin, palette, sizeof( uint16_t ), palette_size ) != palette_size ) {
			goto UNEXPECTED_EOF;
		}
	}

	TIMImageInfo image_info;
	if ( qm_file_read( fin, &image_info, sizeof( TIMImageInfo ), 1 ) != 1 ) {
		goto UNEXPECTED_EOF;
	}

	/* Check the size and width/height values in the header match. */
	{
		uint32_t image_width_bytes = ( ( uint32_t ) ( image_info.width ) ) * 2;
		if ( image_width_bytes >= image_info.image_size || ( image_width_bytes * image_info.height ) != ( image_info.image_size - sizeof( image_info ) ) ) {
			PlReportErrorF( PL_RESULT_FILETYPE, "invalid size/width/height in TIM image header" );
			goto ERR_CLEANUP;
		}
	}

	/* Read in the image data. */
	size_t image_data_len = image_info.image_size - sizeof( image_info );
	image_data = QM_OS_MEMORY_MALLOC_( image_data_len );
	if ( image_data == NULL ) {
		goto ERR_CLEANUP;
	}

	if ( qm_file_read( fin, image_data, image_data_len, 1 ) != 1 ) {
		goto UNEXPECTED_EOF;
	}

	/* Prepare the metadata and image buffer in the PLImage structure. */

	uint8_t type = ( uint8_t ) ( header.flag1 & TIM_FLAG1_TYPE_MASK );
	switch ( type ) {
		case TIM_TYPE_4BPP: {
			out->width = ( unsigned int ) ( image_info.width * 4 );
			out->height = image_info.height;
			out->format = PL_IMAGEFORMAT_RGB5A1;
		} break;

		case TIM_TYPE_8BPP: {
			out->width = ( unsigned int ) ( image_info.width * 2 );
			out->height = image_info.height;
			out->format = PL_IMAGEFORMAT_RGB5A1;
		} break;

		case TIM_TYPE_16BPP: {
			out->width = image_info.width;
			out->height = image_info.height;
			out->format = PL_IMAGEFORMAT_RGB5A1;
		} break;

		case TIM_TYPE_24BPP: {
			out->width = image_info.width / 1.5;
			out->height = image_info.height;
			out->format = PL_IMAGEFORMAT_RGB8;
		} break;

		default: {
			PlReportErrorF( PL_RESULT_IMAGEFORMAT, "invalid image format" );
			goto ERR_CLEANUP;
		}
	}

	out->size = qm_image_get_format_size( out->format, out->width, out->height );
	out->levels = 1;

	out->data = QM_OS_MEMORY_CALLOC( out->levels, sizeof( uint8_t * ) );
	if ( out->data == NULL ) {
		goto ERR_CLEANUP;
	}

	out->data[ 0 ] = QM_OS_MEMORY_CALLOC( out->size, sizeof( uint8_t ) );
	if ( out->data[ 0 ] == NULL ) {
		goto ERR_CLEANUP;
	}

	/* Copy the image data into the PLImage buffer. */

	switch ( type ) {
		case TIM_TYPE_4BPP: {
			uint8_t *indata = image_data;
			uint16_t *outdata = ( uint16_t * ) ( out->data[ 0 ] );

			for ( ; indata < ( uint8_t * ) ( image_data + image_data_len ); ++indata ) {
				uint8_t p1 = ( *indata & 0x0F );
				uint8_t p2 = ( *indata & 0xF0 ) >> 4;

				if ( p1 >= palette_size || p2 >= palette_size ) {
					PlReportErrorF( PL_RESULT_FILETYPE, "out-of-range palette index in TIM image" );
					goto ERR_CLEANUP;
				}

				*( outdata++ ) = _tim16toRGB51A( palette[ p1 ] );
				*( outdata++ ) = _tim16toRGB51A( palette[ p2 ] );
			}

			break;
		}

		case TIM_TYPE_8BPP: {
			uint8_t *indata = image_data;
			uint16_t *outdata = ( uint16_t * ) ( out->data[ 0 ] );

			for ( ; indata < ( uint8_t * ) ( image_data + image_data_len ); ++indata ) {
				uint8_t p = *indata;

				if ( p >= palette_size ) {
					PlReportErrorF( PL_RESULT_FILETYPE, "out-of-range palette index in TIM image" );
					goto ERR_CLEANUP;
				}

				*( outdata++ ) = _tim16toRGB51A( palette[ p ] );
			}

			break;
		}

		case TIM_TYPE_16BPP: {
			uint8_t *indata = image_data;
			uint8_t *outdata = out->data[ 0 ];

			for ( ; indata < ( uint8_t * ) ( image_data + image_data_len ); ++indata ) {
				uint16_t colour = ( uint16_t ) ( ( *indata ) + ( ( *indata ) << 8 ) );
				*( outdata++ ) = ( uint8_t ) ( colour );//(_tim16toRGB51A(colour));
			}

			break;
		}

		case TIM_TYPE_24BPP:
		default:
			PlReportErrorF( PL_RESULT_IMAGEFORMAT, "unsupported tim type (%d)", type );
			goto ERR_CLEANUP;
	}

	qm_os_memory_free( image_data );
	qm_os_memory_free( palette );

	return true;

UNEXPECTED_EOF:
	PlReportErrorF( PL_RESULT_FILEREAD, "unexpected EOF when loading TIM file" );

ERR_CLEANUP:

	if ( out->data != NULL ) {
		qm_os_memory_free( out->data[ 0 ] );
		qm_os_memory_free( out->data );
	}

	qm_os_memory_free( image_data );
	qm_os_memory_free( palette );

	return false;
}

QmImage *qm_image_tim_parse( QmFsFile *file ) {
	if ( !TIM_FormatCheck( file ) ) {
		return NULL;
	}

	qm_fs_file_rewind( file );

	QmImage *image = QM_OS_MEMORY_CALLOC( 1, sizeof( QmImage ) );
	if ( !TIM_ReadFile( file, image ) ) {
		qm_os_memory_free( image );
		image = NULL;
	}

	return image;
}
