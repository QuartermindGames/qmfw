// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Image compression/decompression methods.
// Author:  Mark E. Sowden

#include "3rdparty/txc_compress_dxtn.c"

#include "image_private.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
// Compression

QmImage *qm_image_compress_s3tc( const QmImage *src, PLImageFormat dstFormat )
{
	unsigned int blockSize, glFormat;
	switch ( dstFormat )
	{
		default:
			return nullptr;
		case PL_IMAGEFORMAT_RGB_DXT1:
		{
			blockSize = 8;
			glFormat  = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
			break;
		}
		case PL_IMAGEFORMAT_RGBA_DXT1:
		{
			blockSize = 8;
			glFormat  = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
			break;
		}
		case PL_IMAGEFORMAT_RGBA_DXT3:
		{
			blockSize = 16;
			glFormat  = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			break;
		}
		case PL_IMAGEFORMAT_RGBA_DXT5:
		{
			blockSize = 16;
			glFormat  = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			break;
		}
	}

	const size_t   totalSize = qm_image_get_format_size( dstFormat, src->width, src->height );
	unsigned char *buf       = QM_OS_MEMORY_NEW_( unsigned char, totalSize );
	if ( buf == nullptr )
	{
		return nullptr;
	}

	tx_compress_dxtn( src->colour_format == PL_COLOURFORMAT_RGBA ? 4 : 3,
	                  src->width, src->height,
	                  src->data[ 0 ],
	                  glFormat, buf,
	                  ( src->width + 3 ) / 4 * blockSize );

	QmImage *image = PlCreateImage( nullptr, src->width, src->height, 0, src->colour_format, PL_IMAGEFORMAT_RGB_DXT1 );
	if ( image != nullptr )
	{
		image->data[ 0 ] = buf;
	}
	else
	{
		qm_os_memory_free( buf );
	}

	return image;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Decompression

void DecompressBlockBC1( uint32_t x, uint32_t y, uint32_t stride, const uint8_t *blockStorage, unsigned char *image );// DXT1
void DecompressBlockBC2( uint32_t x, uint32_t y, uint32_t stride, const uint8_t *blockStorage, unsigned char *image );// DXT3
void DecompressBlockBC3( uint32_t x, uint32_t y, uint32_t stride, const uint8_t *blockStorage, unsigned char *image );// DXT5

void qm_image_compression_decompress_dxt1( unsigned int width, unsigned int height, const unsigned char *blockStorage, unsigned char *image )
{
	const uint32_t stride = width * sizeof( uint32_t );
	for ( uint32_t j = 0; j < height; j += 4 )
	{
		for ( uint32_t i = 0; i < width; i += 4 )
		{
			DecompressBlockBC1( i, j, stride, blockStorage, image );
			blockStorage += 8;
		}
	}
}

void qm_image_compression_decompress_dxt3( unsigned int width, unsigned int height, const unsigned char *blockStorage, unsigned char *image )
{
	const uint32_t stride = width * sizeof( uint32_t );
	for ( uint32_t j = 0; j < height; j += 4 )
	{
		for ( uint32_t i = 0; i < width; i += 4 )
		{
			DecompressBlockBC2( i, j, stride, blockStorage, image );
			blockStorage += 16;
		}
	}
}

void qm_image_compression_decompress_dxt5( unsigned int width, unsigned int height, const unsigned char *blockStorage, unsigned char *image )
{
	const uint32_t stride = width * sizeof( uint32_t );
	for ( uint32_t j = 0; j < height; j += 4 )
	{
		for ( uint32_t i = 0; i < width; i += 4 )
		{
			DecompressBlockBC3( i, j, stride, blockStorage, image );
			blockStorage += 16;
		}
	}
}
