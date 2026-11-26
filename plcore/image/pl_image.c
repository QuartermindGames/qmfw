// SPDX-License-Identifier: MIT
// Hei Platform Library
// Copyright © 2017-2024 Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_filesystem.h>
#include <plcore/pl_image.h>

#include <errno.h>

#include "filesystem_private.h"
#include "image_private.h"
#include "qmos/public/qm_os_memory.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#if defined( STB_IMAGE_WRITE_IMPLEMENTATION )
#	define STBIW_MALLOC( sz )        QM_OS_MEMORY_MALLOC_( sz )
#	define STBIW_REALLOC( p, newsz ) qm_os_memory_realloc( p, newsz )
#	define STBIW_FREE( p )           qm_os_memory_free( p )

#	include "stb_image_write.h"
#endif

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#if defined( STB_IMAGE_RESIZE_IMPLEMENTATION )
#	define STBIR_MALLOC( sz, ... ) QM_OS_MEMORY_MALLOC_( sz )
#	define STBIR_FREE( p, ... )    qm_os_memory_free( p )

#	include "stb_image_resize.h"
#endif

#define STB_IMAGE_IMPLEMENTATION
#if defined( STB_IMAGE_IMPLEMENTATION )
#	define STBI_MALLOC( sz )        QM_OS_MEMORY_MALLOC_( sz )
#	define STBI_REALLOC( p, newsz ) qm_os_memory_realloc( p, newsz )
#	define STBI_FREE( p )           qm_os_memory_free( p )

#	define STB_IMAGE_WRITE_STATIC
#	include "stb_image.h"

static QmImage *LoadStbImage( QmFsFile *file ) {
	size_t s = qm_fs_file_get_size( file );
	if ( s >= INT32_MAX ) {
		PlReportBasicError( PL_RESULT_FILESIZE );
		return NULL;
	}

	void *tmp = QM_OS_MEMORY_MALLOC_( s );
	if ( tmp == NULL )
	{
		return NULL;
	}

	qm_file_read( file, tmp, sizeof( char ), s );

	int x, y, component;
	unsigned char *data = stbi_load_from_memory( tmp, ( int ) s, &x, &y, &component, 4 );

	qm_os_memory_free( tmp );

	if ( data == NULL ) {
		PlReportErrorF( PL_RESULT_FILEREAD, "failed to read in image (%s)", stbi_failure_reason() );
		return NULL;
	}

	QmImage *image = QM_OS_MEMORY_NEW( QmImage );
	if ( image != NULL )
	{
		image->colour_format = PL_COLOURFORMAT_RGBA;
		image->format        = PL_IMAGEFORMAT_RGBA8;
		image->width         = ( unsigned int ) x;
		image->height        = ( unsigned int ) y;

		image->levels = 1;
		image->data   = QM_OS_MEMORY_NEW_( uint8_t *, image->levels );

		image->size      = PlGetImageSize( image->format, image->width, image->height );
		image->data[ 0 ] = data;
		data             = NULL;
	}

	qm_os_memory_free( data );

	return image;
}

#endif

#define MAX_IMAGE_LOADERS 4096

typedef struct PLImageLoader {
	const char *extension;
	QmImage *( *ParseFile )( QmFsFile *file );
} PLImageLoader;

static PLImageLoader imageLoaders[ MAX_IMAGE_LOADERS ];
static unsigned int numImageLoaders = 0;

void PlRegisterImageLoader( const char *extension, QmImage *( *ParseFile )( QmFsFile *file ) ) {
	if ( numImageLoaders >= MAX_IMAGE_LOADERS ) {
		PlReportBasicError( PL_RESULT_MEMORY_EOA );
		return;
	}

	assert( ParseFile != NULL );

	imageLoaders[ numImageLoaders ].extension = extension;
	imageLoaders[ numImageLoaders ].ParseFile = ParseFile;

	numImageLoaders++;
}

void PlRegisterStandardImageLoaders( unsigned int flags ) {
	typedef struct SImageLoader {
		unsigned int flag;
		const char *extension;
		QmImage *( *LoadFunction )( QmFsFile *file );
	} SImageLoader;

	static const SImageLoader loaderList[] = {
	        {PL_IMAGE_FILEFORMAT_TGA,       "tga",  LoadStbImage         },
	        {PL_IMAGE_FILEFORMAT_PNG,       "png",  LoadStbImage         },
	        {PL_IMAGE_FILEFORMAT_JPG,       "jpg",  LoadStbImage         },
	        {PL_IMAGE_FILEFORMAT_JPG,       "jpeg", LoadStbImage         },
	        {PL_IMAGE_FILEFORMAT_JPG,       "jfif", LoadStbImage         },
	        {PL_IMAGE_FILEFORMAT_BMP,       "bmp",  LoadStbImage         },
	        {PL_IMAGE_FILEFORMAT_PSD,       "psd",  LoadStbImage         },
	        {PL_IMAGE_FILEFORMAT_GIF,       "gif",  LoadStbImage         },
	        {PL_IMAGE_FILEFORMAT_HDR,       "hdr",  LoadStbImage         },
	        {PL_IMAGE_FILEFORMAT_PIC,       "pic",  LoadStbImage         },
	        {PL_IMAGE_FILEFORMAT_PNM,       "pnm",  LoadStbImage         },
	        {PL_IMAGE_FILEFORMAT_FTX,       "ftx",  qm_image_ftx_parse      },
	        {PL_IMAGE_FILEFORMAT_3DF,       "3df",  qm_image_3df_parse      },
	        {PL_IMAGE_FILEFORMAT_TIM,       "tim",  qm_image_tim_parse      },
	        {PL_IMAGE_FILEFORMAT_SWL,       "swl",  qm_image_swl_parse      },
	        {PL_IMAGE_FILEFORMAT_QOI,       "qoi",  qm_image_qoi_parse      },
	        {PL_IMAGE_FILEFORMAT_DDS,       "dds",  qm_image_dds_parse      },
	        {PL_IMAGE_FILEFORMAT_RSB,       "rsb",  qm_image_rsb_parse     },
	        {PL_IMAGE_FILEFORMAT_TEX,       "tex",  qm_image_3dr_parse  },
	        {PL_IMAGE_FILEFORMAT_ANGEL_TEX, "tex",  qm_image_angel_tex_parse},
	        {PL_IMAGE_FILEFORMAT_DTX,       "dtx",  qm_image_dtx_parse     },
	        {PL_IMAGE_FILEFORMAT_HSM,       "hsm",  qm_image_hsm_parse      },
	};

	for ( unsigned int i = 0; i < QM_OS_ARRAY_ELEMENTS( loaderList ); ++i ) {
		if ( flags != PL_IMAGE_FILEFORMAT_ALL && !( flags & loaderList[ i ].flag ) ) {
			continue;
		}

		PlRegisterImageLoader( loaderList[ i ].extension, loaderList[ i ].LoadFunction );
	}
}

void PlClearImageLoaders( void ) {
	numImageLoaders = 0;
}

QmImage *PlCreateImage( void *buf, unsigned int w, unsigned int h, unsigned int numFrames, PLColourFormat col, PLImageFormat dat ) {
	QmImage *image = QM_OS_MEMORY_MALLOC_( sizeof( QmImage ) );
	if ( image == NULL ) {
		return NULL;
	}

	image->width = w;
	image->height = h;
	image->colour_format = col;
	image->format = dat;
	image->size = PlGetImageSize( image->format, image->width, image->height );
	image->levels = 1;

	if ( numFrames > 0 ) {
		image->numFrames = numFrames;
		image->frames = QM_OS_MEMORY_NEW_( PLImageFrame, image->numFrames );
		for ( unsigned int i = 0; i < image->numFrames; ++i ) {
			image->frames[ i ].data = QM_OS_MEMORY_NEW_( void *, image->levels );
		}
	} else { /* todo: kill this */
		image->data = QM_OS_MEMORY_CALLOC( image->levels, sizeof( uint8_t * ) );
		if ( image->data == NULL ) {
			PlDestroyImage( image );
			return NULL;
		}

		image->data[ 0 ] = QM_OS_MEMORY_CALLOC( image->size, sizeof( uint8_t ) );
		if ( image->data[ 0 ] == NULL ) {
			PlDestroyImage( image );
			return NULL;
		}

		if ( buf != NULL ) {
			memcpy( image->data[ 0 ], buf, image->size );
		}
	}

	return image;
}

void PlDestroyImage( QmImage *image ) {
	if ( image == NULL ) {
		return;
	}

	/* todo: kill this */
	for ( unsigned int levels = 0; levels < image->levels; ++levels ) {
		qm_os_memory_free( image->data[ levels ] );
	}

	for ( unsigned int i = 0; i < image->numFrames; ++i ) {
		for ( unsigned int j = 0; j < image->frames[ i ].numMips; ++j ) {
			qm_os_memory_free( image->frames[ i ].data[ j ] );
		}
		qm_os_memory_free( image->frames[ i ].data );
	}
	qm_os_memory_free( image->frames );

	qm_os_memory_free( image->data );
	qm_os_memory_free( image );
}

/**
 * Load an image by it's given pass.
 * Returns null on fail.
 */
QmImage *qm_image_load( const char *path ) {
	if ( !qm_fs_check_file_exists( path ) ) {
		PlReportBasicError( PL_RESULT_FILEPATH );
		return NULL;
	}

	const char *extension = PlGetFileExtension( path );
	for ( unsigned int i = 0; i < numImageLoaders; ++i ) {
		if ( pl_strcasecmp( extension, imageLoaders[ i ].extension ) != 0 ) {
			continue;
		}

		QmImage *image = NULL;
		QmFsFile *file = qm_fs_file_open( path, false );
		if ( file != NULL ) {
			image = imageLoaders[ i ].ParseFile( file );
			PlCloseFile( file );
		}

		if ( image == NULL ) {
			continue;
		}

		snprintf( image->path, sizeof( image->path ), "%s", path );
		return image;
	}

	PlReportBasicError( PL_RESULT_UNSUPPORTED );

	return NULL;
}

/**
 * Load an image by it's virtual file handle.
 */
QmImage *qm_image_parse( QmFsFile *file ) {
	const char *extension = PlGetFileExtension( file->path );
	for ( unsigned int i = 0; i < numImageLoaders; ++i ) {
		if ( extension != NULL && pl_strcasecmp( extension, imageLoaders[ i ].extension ) != 0 ) {
			continue;
		}

		QmImage *image = imageLoaders[ i ].ParseFile( file );
		if ( image == NULL ) {
			continue;
		}

		return image;
	}

	return NULL;
}

bool qm_image_write( const QmImage *image, const char *path, unsigned int quality ) {
	if ( path != NULL && *path == '\0' ) {
		PlReportErrorF( PL_RESULT_FILEPATH, PlGetResultString( PL_RESULT_FILEPATH ) );
		return false;
	}

	int comp = ( int ) PlGetNumImageFormatChannels( image->format );
	if ( comp == 0 ) {
		PlReportErrorF( PL_RESULT_IMAGEFORMAT, "invalid colour format" );
		return false;
	}

	const char *extension = PlGetFileExtension( path );
	if ( extension != NULL && *extension != '\0' ) {
		if ( !pl_strncasecmp( extension, "bmp", 3 ) ) {
			if ( stbi_write_bmp( path, ( int ) image->width, ( int ) image->height, comp, image->data[ 0 ] ) == 1 ) {
				return true;
			}
		} else if ( !pl_strncasecmp( extension, "png", 3 ) ) {
			if ( stbi_write_png( path, ( int ) image->width, ( int ) image->height, comp, image->data[ 0 ], 0 ) == 1 ) {
				return true;
			}
		} else if ( !pl_strncasecmp( extension, "tga", 3 ) ) {
			if ( stbi_write_tga( path, ( int ) image->width, ( int ) image->height, comp, image->data[ 0 ] ) == 1 ) {
				return true;
			}
		} else if ( !pl_strncasecmp( extension, "jpg", 3 ) || !pl_strncasecmp( extension, "jpeg", 3 ) ) {
			if ( stbi_write_jpg( path, ( int ) image->width, ( int ) image->height, comp, image->data[ 0 ], quality ) == 1 ) {
				return true;
			}
		} else if ( !pl_strncasecmp( extension, "qoi", 3 ) ) {
			if ( qm_image_qoi_write( image, path ) ) {
				return true;
			}
		}
	}

	PlReportErrorF( PL_RESULT_FILETYPE, PlGetResultString( PL_RESULT_FILETYPE ) );
	return false;
}

static bool RGB8toRGBA8( QmImage *image ) {
	size_t size = PlGetImageSize( PL_IMAGEFORMAT_RGBA8, image->width, image->height );
	if ( size == 0 ) {
		return false;
	}

	typedef struct RGB8 {
		uint8_t r, g, b;
	} RGB8;
	typedef struct RGBA8 {
		uint8_t r, g, b, a;
	} RGBA8;
	size_t num_pixels = image->size / 3;

	RGB8 *src = ( RGB8 * ) image->data[ 0 ];
	RGBA8 *dst = QM_OS_MEMORY_MALLOC_( size );
	for ( size_t i = 0; i < num_pixels; ++i ) {
		dst->r = src->r;
		dst->g = src->g;
		dst->b = src->b;
		dst->a = 255;
		dst++;
		src++;
	}

	qm_os_memory_free( image->data[ 0 ] );

	image->data[ 0 ] = ( uint8_t * ) ( &dst[ 0 ] );
	image->size = size;
	image->format = PL_IMAGEFORMAT_RGBA8;
	image->colour_format = PL_COLOURFORMAT_RGBA;

	return true;
}

#define scale_5to8( i ) ( ( ( ( double ) ( i ) ) / 31 ) * 255 )

static uint8_t *ImageDataRGB5A1toRGBA8( const uint8_t *src, size_t n_pixels ) {
	uint8_t *dst = QM_OS_MEMORY_MALLOC_( n_pixels * 4 );
	if ( dst == NULL ) {
		return NULL;
	}

	uint8_t *dp = dst;

	for ( size_t i = 0; i < n_pixels; ++i ) {
		/* Red */
		*( dp++ ) = scale_5to8( ( src[ 0 ] & 0xF8 ) >> 3 );

		/* Green */
		*( dp++ ) = scale_5to8( ( ( src[ 0 ] & 0x07 ) << 2 ) | ( ( src[ 1 ] & 0xC0 ) >> 6 ) );

		/* Blue */
		*( dp++ ) = scale_5to8( ( src[ 1 ] & 0x3E ) >> 1 );

		/* Alpha */
		*( dp++ ) = ( src[ 1 ] & 0x01 ) ? 255 : 0;

		src += 2;
	}

	return dst;
}

bool PlConvertPixelFormat( QmImage *image, PLImageFormat new_format ) {
	if ( image->format == new_format ) {
		return true;
	}

	switch ( image->format )
	{
		case PL_IMAGEFORMAT_RGB8:
		{
			QmMathColour3ub *src          = ( QmMathColour3ub * ) image->data[ 0 ];
			size_t           numSrcPixels = image->size / sizeof( QmMathColour3ub );
			switch ( new_format )
			{
				default:
					break;
				case PL_IMAGEFORMAT_RGBA8:
				{
					return RGB8toRGBA8( image );
				}
				case PL_IMAGEFORMAT_R8:
				{
					size_t dstSize = PlGetImageSize( PL_IMAGEFORMAT_R8, image->width, image->height );
					assert( dstSize != 0 );

					typedef struct R8
					{
						uint8_t r;
					} R8;
					R8 *dst = QM_OS_MEMORY_MALLOC_( dstSize );
					for ( size_t i = 0; i < numSrcPixels; ++i )
					{
						// only copy the red channel, discard the others
						// (do we really want to do it this way? alternatively, could combine all channels...)
						dst[ i ].r = src[ i ].r;
					}

					qm_os_memory_free( image->data[ 0 ] );

					image->data[ 0 ]     = ( uint8_t * ) dst;
					image->size          = dstSize;
					image->format        = PL_IMAGEFORMAT_R8;
					image->colour_format = PL_COLOURFORMAT_RGB;

					return true;
				}
			}
		}
		case PL_IMAGEFORMAT_RGBA8:
		{
			QmMathColour4ub *src          = ( QmMathColour4ub * ) image->data[ 0 ];
			size_t           numSrcPixels = image->size / sizeof( QmMathColour4ub );
			if ( new_format == PL_IMAGEFORMAT_R8 )
			{
				size_t dstSize = PlGetImageSize( PL_IMAGEFORMAT_R8, image->width, image->height );
				assert( dstSize != 0 );

				typedef struct R8
				{
					uint8_t r;
				} R8;
				R8 *dst = QM_OS_MEMORY_NEW_( R8, dstSize );
				for ( size_t i = 0; i < numSrcPixels; ++i )
				{
					// only copy the red channel, discard the others
					// (do we really want to do it this way? alternatively, could combine all channels...)
					dst[ i ].r = src[ i ].r;
				}

				qm_os_memory_free( image->data[ 0 ] );

				image->data[ 0 ]     = ( uint8_t * ) dst;
				image->size          = dstSize;
				image->format        = PL_IMAGEFORMAT_R8;
				image->colour_format = PL_COLOURFORMAT_RGB;

				return true;
			}
		}

		case PL_IMAGEFORMAT_RGB5A1: {
			if ( new_format == PL_IMAGEFORMAT_RGBA8 ) {
				uint8_t **levels = QM_OS_MEMORY_CALLOC( image->levels, sizeof( uint8_t * ) );

				/* Make a new copy of each detail level in the new format. */

				unsigned int lw = image->width;
				unsigned int lh = image->height;

				for ( unsigned int l = 0; l < image->levels; ++l ) {
					levels[ l ] = ImageDataRGB5A1toRGBA8( image->data[ l ], lw * lh );
					if ( levels[ l ] == NULL ) {
						/* Memory allocation failed, ditch any buffers we've created so far. */
						for ( unsigned int m = 0; m < l; ++m ) {
							qm_os_memory_free( levels[ m ] );
						}

						qm_os_memory_free( levels );

						PlReportErrorF( PL_RESULT_MEMORY_ALLOCATION, "couldn't allocate memory for image data" );
						return false;
					}

					lw /= 2;
					lh /= 2;
				}

				/* Now that all levels have been converted, free and replace the old buffers. */

				for ( unsigned int l = 0; l < image->levels; ++l ) {
					qm_os_memory_free( image->data[ l ] );
				}

				qm_os_memory_free( image->data );
				image->data = levels;

				image->format = new_format;
				/* TODO: Update colour_format */

				return true;
			}
		} break;

		default:
			break;
	}

	PlReportErrorF( PL_RESULT_IMAGEFORMAT, "unsupported image format conversion" );
	return false;
}

unsigned int PlGetImageSize( PLImageFormat format, unsigned int width, unsigned int height )
{
	switch ( format )
	{
		case PL_IMAGEFORMAT_RGB_DXT1:
		{
			constexpr unsigned int BLOCK_SIZE = 8;
			return ( width + 3 ) / 4 * BLOCK_SIZE * ( ( height + 3 ) / 4 );
		}
		case PL_IMAGEFORMAT_RGBA_DXT3:
		case PL_IMAGEFORMAT_RGBA_DXT5:
		{
			constexpr unsigned int BLOCK_SIZE = 16;
			return ( width + 3 ) / 4 * BLOCK_SIZE * ( ( height + 3 ) / 4 );
		}
		default:
		{
			unsigned int bytes = PlGetImageFormatPixelSize( format );
			return width * height * bytes;
		}
	}
}

/* Returns the number of BYTES per pixel for the given PLImageFormat.
 *
 * If the format doesn't have a predictable size or the size isn't a multiple
 * of one byte, returns ZERO. */
unsigned int PlGetImageFormatPixelSize( PLImageFormat format ) {
	switch ( format ) {
		case PL_IMAGEFORMAT_R8:
			return 1;
		case PL_IMAGEFORMAT_RGBA4:
		case PL_IMAGEFORMAT_RGB5A1:
		case PL_IMAGEFORMAT_RGB565:
			return 2;
		case PL_IMAGEFORMAT_RGB8:
			return 3;
		case PL_IMAGEFORMAT_RGBA_DXT1:
		case PL_IMAGEFORMAT_RGBA8:
			return 4;
		case PL_IMAGEFORMAT_RGBA12:
		case PL_IMAGEFORMAT_RGB16F:
			return 6;
		case PL_IMAGEFORMAT_RGBA16:
		case PL_IMAGEFORMAT_RGBA16F:
			return 8;

		case PL_IMAGEFORMAT_RGB32F:
			return 16;
		case PL_IMAGEFORMAT_RGBA32F:
			return 32;

		default:
			return 0;
	}
}

/**
 * Returns the number of channels for the given format.
 */
unsigned int PlGetNumImageFormatChannels( PLImageFormat format ) {
	switch ( format ) {
		case PL_IMAGEFORMAT_R8:
			return 1;
		case PL_IMAGEFORMAT_RGB4:
		case PL_IMAGEFORMAT_RGB5:
		case PL_IMAGEFORMAT_RGB565:
		case PL_IMAGEFORMAT_RGB8:
		case PL_IMAGEFORMAT_RGB_DXT1:
		case PL_IMAGEFORMAT_RGB_FXT1:
			return 3;
		case PL_IMAGEFORMAT_RGBA4:
		case PL_IMAGEFORMAT_RGB5A1:
		case PL_IMAGEFORMAT_RGBA8:
		case PL_IMAGEFORMAT_RGBA12:
		case PL_IMAGEFORMAT_RGBA16:
		case PL_IMAGEFORMAT_RGBA16F:
			return 4;
		default:
			return 0;
	}
}

bool PlImageHasAlpha( const QmImage *image ) {
	return ( PlGetNumImageFormatChannels( image->format ) >= 4 );
}

void qm_image_invert_colour( QmImage *image ) {
	unsigned int num_colours = PlGetNumImageFormatChannels( image->format );
	switch ( image->format ) {
		case PL_IMAGEFORMAT_RGB8:
		case PL_IMAGEFORMAT_RGBA8: {
			for ( size_t i = 0; i < image->size; i += num_colours ) {
				uint8_t *pixel = &image->data[ 0 ][ i ];
				pixel[ 0 ] = ~pixel[ 0 ];
				pixel[ 1 ] = ~pixel[ 1 ];
				pixel[ 2 ] = ~pixel[ 2 ];
			}
		} break;

		default:
			break;
	}

	PlReportErrorF( PL_RESULT_IMAGEFORMAT, "unsupported image format" );
}

/* utility function */
void PlGenerateStipplePattern( QmImage *image, unsigned int depth ) {
#if 0
    unsigned int p = 0;
    unsigned int num_colours = plGetSamplesPerPixel(image->colour_format);
    switch(image->format) {
        case PL_IMAGEFORMAT_RGB8: break;
        case PL_IMAGEFORMAT_RGBA8: {
            for(unsigned int i = 0; i < image->size; i += num_colours) {
                uint8_t *pixel = &image->data[0][i];
                if(num_colours == 4) {
                    //if()
                }
            }
        } break;

        default:break;
    }

    ReportError(PL_RESULT_IMAGEFORMAT, "unsupported image format");
#else
	/* todo */
#endif
}

void PlClearImageAlpha( QmImage *image ) {
	if ( image->format != PL_IMAGEFORMAT_RGBA8 ) {
		PlReportErrorF( PL_RESULT_IMAGEFORMAT, "unsupported image format" );
		return;
	}

	for ( unsigned int i = 0; i < image->size; i += 4 ) {
		image->data[ 0 ][ i + 3 ] = 255;
	}
}

void qm_image_replace_colour( QmImage *image, QmMathColour4ub target, QmMathColour4ub dest ) {
	unsigned int num_colours = PlGetNumImageFormatChannels( image->format );
	switch ( image->format ) {
		case PL_IMAGEFORMAT_RGB8:
		case PL_IMAGEFORMAT_RGBA8: {
			for ( unsigned int i = 0; i < image->size; i += num_colours ) {
				uint8_t *pixel = &image->data[ 0 ][ i ];
				if ( num_colours == 4 ) {
					if ( qm_math_colour4ub_compare( qm_math_colour4ub( pixel[ 0 ], pixel[ 1 ], pixel[ 2 ], pixel[ 3 ] ), target ) ) {
						pixel[ 0 ] = dest.r;
						pixel[ 1 ] = dest.g;
						pixel[ 2 ] = dest.b;
						pixel[ 3 ] = dest.a;
					}
				} else {
					if ( qm_math_colour4ub_compare( QM_MATH_COLOUR4UB_RGB( pixel[ 0 ], pixel[ 1 ], pixel[ 2 ] ), target ) ) {
						pixel[ 0 ] = dest.r;
						pixel[ 1 ] = dest.g;
						pixel[ 2 ] = dest.b;
					}
				}
			}
		} break;

		default:
			break;
	}

	PlReportErrorF( PL_RESULT_IMAGEFORMAT, "unsupported image format" );
}

void PlFreeImage( QmImage *image ) {
	FunctionStart();

	if ( image == NULL || image->data == NULL ) {
		return;
	}

	for ( unsigned int levels = 0; levels < image->levels; ++levels ) {
		qm_os_memory_free( image->data[ levels ] );
	}

	qm_os_memory_free( image->data );
}

bool PlFlipImageVertical( QmImage *image ) {
	unsigned int width = image->width;
	unsigned int height = image->height;

	unsigned int bytes_per_pixel = PlGetImageFormatPixelSize( image->format );
	if ( bytes_per_pixel == 0 ) {
		PlReportErrorF( PL_RESULT_IMAGEFORMAT, "cannot flip images in this format" );
		return false;
	}

	unsigned int bytes_per_row = width * bytes_per_pixel;

	unsigned char *swap = QM_OS_MEMORY_MALLOC_( bytes_per_row );
	if ( swap == NULL ) {
		return false;
	}

	for ( unsigned int l = 0; l < image->levels; ++l ) {
		for ( unsigned int r = 0; r < height / 2; ++r ) {
			unsigned char *tr = image->data[ l ] + ( r * bytes_per_row );
			unsigned char *br = image->data[ l ] + ( ( ( height - 1 ) - r ) * bytes_per_row );

			memcpy( swap, tr, bytes_per_row );
			memcpy( tr, br, bytes_per_row );
			memcpy( br, swap, bytes_per_row );
		}

		bytes_per_row /= 2;
		height /= 2;
	}

	qm_os_memory_free( swap );

	return true;
}

/**
 * Returns a list of file extensions representing all
 * the formats supported by the image loader.
 */
const char **PlGetSupportedImageFormats( unsigned int *numElements ) {
	static const char *imageFormats[ MAX_IMAGE_LOADERS ];
	for ( unsigned int i = 0; i < numImageLoaders; ++i ) {
		imageFormats[ i ] = imageLoaders[ i ].extension;
	}

	*numElements = numImageLoaders;

	return imageFormats;
}

/* getters */
unsigned int qm_image_get_width( const QmImage *image ) { return image->width; }
unsigned int qm_image_get_height( const QmImage *image ) { return image->height; }
PLImageFormat PlGetImageFormat( const QmImage *image ) { return image->format; }

void *PlGetImageData( QmImage *image, unsigned int frame, unsigned int mip ) {
	if ( mip >= image->levels ) {
		PlReportErrorF( PL_RESULT_MEMORY_EOA, "invalid mip" );
		return NULL;
	}

	if ( image->numFrames > 0 ) {
		if ( frame >= image->numFrames ) {
			PlReportErrorF( PL_RESULT_MEMORY_EOA, "invalid frame" );
			return NULL;
		}

		return image->frames[ frame ].data[ mip ];
	}

	/* todo: kill this */
	return image->data[ mip ];
}

unsigned int PlGetImageDataSize( const QmImage *image ) {
	return PlGetImageSize( image->format, image->width, image->height );
}

QmImage *qm_image_resize( QmImage *image, unsigned int newWidth, unsigned int newHeight ) {
	if ( image->width == newWidth && image->height == newHeight ) {
		PlReportErrorF( PL_RESULT_INVALID_PARM1, "image already of width/height" );
		return NULL;
	}

	QmImage *newImage = PlCreateImage( NULL, newWidth, newHeight, 0, image->colour_format, image->format );
	if ( newImage == NULL ) {
		return NULL;
	}

	uint8_t *src = PlGetImageData( image, 0, 0 );
	uint8_t *dst = PlGetImageData( newImage, 0, 0 );
	if ( stbir_resize_uint8( src, ( int ) image->width, ( int ) image->height, 0,
	                         dst, ( int ) newImage->width, ( int ) newImage->height, 0,
	                         ( int ) PlGetNumImageFormatChannels( newImage->format ) ) == 0 ) {
		PlReportErrorF( PL_RESULT_FAIL, "image resize failed" );
		return NULL;
	}

	return newImage;
}

QmImage *qm_image_crop( QmImage *image, unsigned int newWidth, unsigned int newHeight, unsigned int xOffset, unsigned int yOffset ) {
	if ( image->width == newWidth && image->height == newHeight ) {
		PlReportErrorF( PL_RESULT_INVALID_PARM1, "image already of width/height" );
		return NULL;
	} else if ( newWidth + xOffset > image->width || newHeight + yOffset > image->height ) {
		PlReportErrorF( PL_RESULT_INVALID_PARM1, "destination must be smaller than source image" );
		return NULL;
	}

	QmImage *newImage = PlCreateImage( NULL, newWidth, newHeight, 0, image->colour_format, image->format );
	if ( newImage == NULL ) {
		return NULL;
	}

	unsigned int numChannels = PlGetNumImageFormatChannels( newImage->format );
	uint8_t *src = PlGetImageData( image, 0, 0 );
	uint8_t *dst = PlGetImageData( newImage, 0, 0 );

	for ( int y = 0; y < newHeight; y++ ) {
		for ( int x = 0; x < newWidth; x++ ) {
			unsigned int srcPixelIndex = ( ( ( y + yOffset ) * image->width ) + ( x + xOffset ) ) * numChannels;
			unsigned int dstPixelIndex = ( ( y * newWidth ) + x ) * numChannels;
			for ( unsigned int channel = 0; channel < numChannels; ++channel ) {
				dst[ dstPixelIndex + channel ] = src[ srcPixelIndex + channel ];
			}
		}
	}

	return newImage;
}

/////////////////////////////////////////////////////////////////////////////////////
// S3TC Library Interface
/////////////////////////////////////////////////////////////////////////////////////

void DecompressBlockBC1( uint32_t x, uint32_t y, uint32_t stride, const uint8_t *blockStorage, unsigned char *image );// DXT1
void DecompressBlockBC2( uint32_t x, uint32_t y, uint32_t stride, const uint8_t *blockStorage, unsigned char *image );// DXT3
void DecompressBlockBC3( uint32_t x, uint32_t y, uint32_t stride, const uint8_t *blockStorage, unsigned char *image );// DXT5

void PlBlockDecompressImageDXT1( unsigned int width, unsigned int height, const unsigned char *blockStorage, unsigned char *image ) {
	const uint32_t stride = width * sizeof( uint32_t );
	for ( uint32_t j = 0; j < height; j += 4 ) {
		for ( uint32_t i = 0; i < width; i += 4 ) {
			DecompressBlockBC1( i, j, stride, blockStorage, image );
			blockStorage += 8;
		}
	}
}

void PlBlockDecompressImageDXT3( unsigned int width, unsigned int height, const unsigned char *blockStorage, unsigned char *image ) {
	const uint32_t stride = width * sizeof( uint32_t );
	for ( uint32_t j = 0; j < height; j += 4 ) {
		for ( uint32_t i = 0; i < width; i += 4 ) {
			DecompressBlockBC2( i, j, stride, blockStorage, image );
			blockStorage += 16;
		}
	}
}

void PlBlockDecompressImageDXT5( unsigned int width, unsigned int height, const unsigned char *blockStorage, unsigned char *image ) {
	const uint32_t stride = width * sizeof( uint32_t );
	for ( uint32_t j = 0; j < height; j += 4 ) {
		for ( uint32_t i = 0; i < width; i += 4 ) {
			DecompressBlockBC3( i, j, stride, blockStorage, image );
			blockStorage += 16;
		}
	}
}
