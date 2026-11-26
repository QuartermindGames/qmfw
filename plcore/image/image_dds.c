/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include "qmos/public/qm_os_memory.h"

#include "image_private.h"

#define DDS_MAGIC      QM_OS_MAGIC_TO_NUM( 'D', 'D', 'S', ' ' )
#define DDS_MAGIC_DXT1 QM_OS_MAGIC_TO_NUM( 'D', 'X', 'T', '1' )
#define DDS_MAGIC_DXT2 QM_OS_MAGIC_TO_NUM( 'D', 'X', 'T', '2' )
#define DDS_MAGIC_DXT3 QM_OS_MAGIC_TO_NUM( 'D', 'X', 'T', '3' )
#define DDS_MAGIC_DXT4 QM_OS_MAGIC_TO_NUM( 'D', 'X', 'T', '4' )
#define DDS_MAGIC_DXT5 QM_OS_MAGIC_TO_NUM( 'D', 'X', 'T', '5' )
#define DDS_MAGIC_ATI1 QM_OS_MAGIC_TO_NUM( 'A', 'T', 'I', '1' )
#define DDS_MAGIC_ATI2 QM_OS_MAGIC_TO_NUM( 'A', 'T', 'I', '2' )
#define DDS_MAGIC_BC4U QM_OS_MAGIC_TO_NUM( 'B', 'C', '4', 'U' )
#define DDS_MAGIC_BC4S QM_OS_MAGIC_TO_NUM( 'B', 'C', '4', 'S' )
#define DDS_MAGIC_BC5U QM_OS_MAGIC_TO_NUM( 'B', 'C', '5', 'U' )
#define DDS_MAGIC_BC5S QM_OS_MAGIC_TO_NUM( 'B', 'C', '5', 'S' )

#define DDS_HEADER_SIZE    124 /* DDSHeader size minus magic */
#define DDS_HEADER_PF_SIZE 32

typedef struct DDSPixelFormat {
	uint32_t size;
	uint32_t flags;
	uint32_t fourCC;
	uint32_t rgbBitCount;
	uint32_t rBitMask;
	uint32_t gBitMask;
	uint32_t bBitMask;
	uint32_t aBitMask;
} DDSPixelFormat;
PL_STATIC_ASSERT( sizeof( DDSPixelFormat ) == DDS_HEADER_PF_SIZE, "Invalid DDSPixelFormat size!" );

#define DDS_PF_FLAGS_ALPHAPIXELS 0x1
#define DDS_PF_FLAGS_ALPHA       0x2
#define DDS_PF_FLAGS_FOURCC      0x4
#define DDS_PF_FLAGS_RGB         0x40
#define DDS_PF_FLAGS_YUV         0x200
#define DDS_PF_FLAGS_LUMINANCE   0x20000

#define DDS_PF_FORMAT_HINT_COMPRESSED ( DDS_PF_FLAGS_FOURCC )
#define DDS_PF_FORMAT_HINT_RGB        ( DDS_PF_FLAGS_RGB )
#define DDS_PF_FORMAT_HINT_RGBA       ( DDS_PF_FLAGS_RGB | DDS_PF_FLAGS_ALPHAPIXELS )

typedef struct DDSHeader {
	uint32_t magic; /* 'DDS ' */
	uint32_t size;  /* should always be 124 */
	uint32_t flags;
	uint32_t height, width;
	uint32_t pitchLinear;
	uint32_t depth;
	uint32_t numMipMaps;
	uint32_t reserved1[ 11 ];
	DDSPixelFormat format;
	uint32_t caps;
	uint32_t caps2;
	uint32_t caps3;
	uint32_t caps4;
	uint32_t reserved2;
} DDSHeader;
PL_STATIC_ASSERT( sizeof( DDSHeader ) == 128, "Invalid DDSHeader size!" );

#define DDS_FLAGS_CAPS        0x1 /* always required */
#define DDS_FLAGS_HEIGHT      0x2 /* always required */
#define DDS_FLAGS_WIDTH       0x4 /* always required */
#define DDS_FLAGS_PITCH       0x8
#define DDS_FLAGS_PIXELFORMAT 0x1000 /* always required */
#define DDS_FLAGS_MIPMAPCOUNT 0x20000
#define DDS_FLAGS_LINEARSIZE  0x80000
#define DDS_FLAGS_DEPTH       0x800000

static PLImageFormat GetImageFormat( const DDSPixelFormat *pixelFormat ) {
#define ISBITMASK( r, g, b, a ) ( pixelFormat->rBitMask == ( r ) && pixelFormat->gBitMask == ( g ) && pixelFormat->bBitMask == ( b ) && pixelFormat->aBitMask == ( a ) )

	if ( pixelFormat->flags & DDS_PF_FLAGS_FOURCC ) {
		switch ( pixelFormat->fourCC ) {
			case DDS_MAGIC_DXT1: {
#if 0
				if ( pixelFormat->flags & DDS_PF_FLAGS_ALPHA ||
				     pixelFormat->flags & DDS_PF_FLAGS_ALPHAPIXELS ) {
					return PL_IMAGEFORMAT_RGBA_DXT1;
				}

				return PL_IMAGEFORMAT_RGB_DXT1;
#else
				return PL_IMAGEFORMAT_RGBA_DXT1;
#endif
			}
			case DDS_MAGIC_DXT3: {
				return PL_IMAGEFORMAT_RGBA_DXT3;
			}
			case DDS_MAGIC_DXT5: {
				return PL_IMAGEFORMAT_RGBA_DXT5;
			}
		}
	} else if ( pixelFormat->flags & DDS_PF_FLAGS_LUMINANCE ) {
		if ( pixelFormat->rgbBitCount == 8 ) {
			if ( ISBITMASK( 0x000000ff, 0x00000000, 0x00000000, 0x00000000 ) ) {
				return PL_IMAGEFORMAT_R8;
			}
		}
	} else if ( pixelFormat->flags & DDS_PF_FORMAT_HINT_RGB ) {
		switch ( pixelFormat->rgbBitCount ) {
			case 32: {
				if ( ISBITMASK( 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 ) ) {
					return PL_IMAGEFORMAT_RGBA8;
				} else if ( ISBITMASK( 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 ) ) {
					return PL_IMAGEFORMAT_BGRA8;
				} else if ( ISBITMASK( 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 ) ) {
					return PL_IMAGEFORMAT_BGRX8;
				}
				break;
			}
		}
	}

	return PL_IMAGEFORMAT_UNKNOWN;
}

QmImage *qm_image_dds_parse( QmFsFile *file ) {
	DDSHeader header;
	if ( qm_file_read( file, &header, sizeof( DDSHeader ), 1 ) != 1 ) {
		return NULL;
	}

	if ( header.magic != DDS_MAGIC ) {
		PlReportBasicError( PL_RESULT_FILETYPE );
		return NULL;
	}

	if ( header.size != DDS_HEADER_SIZE ) {
		PlReportBasicError( PL_RESULT_FILETYPE );
		return NULL;
	}

	if ( header.format.size != DDS_HEADER_PF_SIZE ) {
		PlReportBasicError( PL_RESULT_FILETYPE );
		return NULL;
	}

	if ( !( header.flags &
	        ( DDS_FLAGS_CAPS |
	          DDS_FLAGS_HEIGHT |
	          DDS_FLAGS_WIDTH |
	          DDS_FLAGS_PIXELFORMAT ) ) ) {
		PlReportBasicError( PL_RESULT_FILETYPE );
		return NULL;
	}

	QmImage image = {};
	image.width = header.width;
	image.height = header.height;
	image.levels = header.numMipMaps ? header.numMipMaps : 1;

	image.format = GetImageFormat( &header.format );
	if ( image.format == PL_IMAGEFORMAT_UNKNOWN ) {
		PlReportBasicError( PL_RESULT_UNSUPPORTED );
		return NULL;
	}

	unsigned int bytesPerBlock;
	switch ( image.format ) {
		case PL_IMAGEFORMAT_RGBA_DXT1:
			bytesPerBlock = 8;
			break;
		case PL_IMAGEFORMAT_RGBA_DXT3:
		case PL_IMAGEFORMAT_RGBA_DXT5:
			bytesPerBlock = 16;
			break;
		default:
			PlReportBasicError( PL_RESULT_UNSUPPORTED );
			return NULL;
	}

	unsigned int mipW = image.width;
	unsigned int mipH = image.height;

	image.data = QM_OS_MEMORY_CALLOC( image.levels, sizeof( uint8_t * ) );
	for ( unsigned int i = 0; i < image.levels; ++i ) {
		unsigned int size = ( ( mipW + 3 ) / 4 ) * ( ( mipH + 3 ) / 4 ) * bytesPerBlock;
		if ( i == 0 ) {
			image.size = size;
		}

		image.data[ i ] = QM_OS_MEMORY_MALLOC_( size );
		if ( qm_file_read( file, image.data[ i ], sizeof( uint8_t ), size ) != size ) {
			PlFreeImage( &image );
			return NULL;
		}

		mipW /= 2;
		mipH /= 2;
	}

	QmImage *out = QM_OS_MEMORY_MALLOC_( sizeof( QmImage ) );
	memcpy( out, &image, sizeof( QmImage ) );

	snprintf( out->path, sizeof( out->path ), "%s", qm_fs_file_get_path( file ) );

	return out;
}
