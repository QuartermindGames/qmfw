// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "qmos/public/qm_os.h"

#include <plcore/pl_filesystem.h>
#include <plcore/pl_math.h>

static constexpr unsigned int QM_IMAGE_MAX_CHANNELS = 4;

typedef enum QmImageChannelFormat
{
	QM_IMAGE_DATA_FORMAT_U8,
	QM_IMAGE_DATA_FORMAT_F16,
	QM_IMAGE_DATA_FORMAT_F32,
} QmImageChannelFormat;

typedef enum QmImageChannelType
{
	QM_IMAGE_CHANNEL_TYPE_RED,
	QM_IMAGE_CHANNEL_TYPE_GREEN,
	QM_IMAGE_CHANNEL_TYPE_BLUE,
	QM_IMAGE_CHANNEL_TYPE_ALPHA,
	QM_IMAGE_CHANNEL_TYPE_IGNORED,
} QmImageChannelType;

typedef struct QmImageChannelFormatDescriptor
{
	QmImageChannelType   type;
	QmImageChannelFormat format;
} QmImageChannelFormatDescriptor;

typedef struct QmImagePixelFormatDescriptor
{
	uint8_t                        numChannels;
	QmImageChannelFormatDescriptor channels[ QM_IMAGE_MAX_CHANNELS ];
} QmImagePixelFormatDescriptor;

#define QM_IMAGE_FORMAT_RGBA8_DESC()                                                                                       \
	( QmImagePixelFormatDescriptor ) { .numChannels = 4,                                                                   \
	                                   .channels    = {                                                                    \
		                                       {.format = QM_IMAGE_DATA_FORMAT_U8, .type = QM_IMAGE_CHANNEL_TYPE_RED  },   \
		                                       {.format = QM_IMAGE_DATA_FORMAT_U8, .type = QM_IMAGE_CHANNEL_TYPE_GREEN},   \
		                                       {.format = QM_IMAGE_DATA_FORMAT_U8, .type = QM_IMAGE_CHANNEL_TYPE_BLUE },   \
		                                       {.format = QM_IMAGE_DATA_FORMAT_U8, .type = QM_IMAGE_CHANNEL_TYPE_ALPHA},   \
 }, }
#define QM_IMAGE_FORMAT_RGB8_DESC()                                                                                        \
	( QmImagePixelFormatDescriptor ) { .numChannels = 3,                                                                   \
	                                   .channels    = {                                                                    \
		                                       {.format = QM_IMAGE_DATA_FORMAT_U8, .type = QM_IMAGE_CHANNEL_TYPE_RED  },   \
		                                       {.format = QM_IMAGE_DATA_FORMAT_U8, .type = QM_IMAGE_CHANNEL_TYPE_GREEN},   \
		                                       {.format = QM_IMAGE_DATA_FORMAT_U8, .type = QM_IMAGE_CHANNEL_TYPE_BLUE } \
 }, }
#define QM_IMAGE_FORMAT_RGB16F_DESC()                                                                                       \
	( QmImagePixelFormatDescriptor ) { .numChannels = 3,                                                                    \
	                                   .channels    = {                                                                     \
		                                       {.format = QM_IMAGE_DATA_FORMAT_F16, .type = QM_IMAGE_CHANNEL_TYPE_RED  },   \
		                                       {.format = QM_IMAGE_DATA_FORMAT_F16, .type = QM_IMAGE_CHANNEL_TYPE_GREEN},   \
		                                       {.format = QM_IMAGE_DATA_FORMAT_F16, .type = QM_IMAGE_CHANNEL_TYPE_BLUE } \
 }, }
#define QM_IMAGE_FORMAT_RGB32F_DESC()                                                                                       \
	( QmImagePixelFormatDescriptor ) { .numChannels = 3,                                                                    \
	                                   .channels    = {                                                                     \
		                                       {.format = QM_IMAGE_DATA_FORMAT_F32, .type = QM_IMAGE_CHANNEL_TYPE_RED  },   \
		                                       {.format = QM_IMAGE_DATA_FORMAT_F32, .type = QM_IMAGE_CHANNEL_TYPE_GREEN},   \
		                                       {.format = QM_IMAGE_DATA_FORMAT_F32, .type = QM_IMAGE_CHANNEL_TYPE_BLUE } \
 }, }

typedef enum PLImageFormat
{
	PL_IMAGEFORMAT_UNKNOWN,

	PL_IMAGEFORMAT_R8,
	PL_IMAGEFORMAT_RGB4,  // 4 4 4
	PL_IMAGEFORMAT_RGBA4, // 4 4 4 4
	PL_IMAGEFORMAT_RGB5,  // 5 5 5
	PL_IMAGEFORMAT_RGB5A1,// 5 5 5 1
	PL_IMAGEFORMAT_RGB565,// 5 6 5
	PL_IMAGEFORMAT_RGB8,  // 8 8 8
	PL_IMAGEFORMAT_BGR8,  // 8 8 8
	PL_IMAGEFORMAT_RGBA8, // 8 8 8 8
	PL_IMAGEFORMAT_BGRA8, // 8 8 8 8
	PL_IMAGEFORMAT_BGRX8, // 8 8 8 0
	PL_IMAGEFORMAT_RGBA12,// 12 12 12 12
	PL_IMAGEFORMAT_RGBA16,// 16 16 16 16

	PL_IMAGEFORMAT_RGB16F,
	PL_IMAGEFORMAT_RGBA16F,// 16 16 16 16

	PL_IMAGEFORMAT_RGB32F,
	PL_IMAGEFORMAT_RGBA32F,

	PL_IMAGEFORMAT_RGBA_DXT1,
	PL_IMAGEFORMAT_RGB_DXT1,
	PL_IMAGEFORMAT_RGBA_DXT3,
	PL_IMAGEFORMAT_RGBA_DXT5,

	PL_IMAGEFORMAT_RGB_FXT1
} PLImageFormat;

/* todo: deprecate this */
typedef enum PLColourFormat
{
	PL_COLOURFORMAT_RGB,
	PL_COLOURFORMAT_BGR,
	PL_COLOURFORMAT_RGBA,
	PL_COLOURFORMAT_BGRA,
} PLColourFormat;

typedef struct PLImageFrame
{
	void       **data;
	unsigned int numMips;
} PLImageFrame;

typedef struct QmImage
{
#if 1
	uint8_t **data; /* todo: kill this */
#else
	uint8_t *data; /* todo: kill this */
#endif

	PLImageFrame *frames;
	unsigned int  numFrames;

	unsigned int   x, y;
	unsigned int   width, height;
	size_t         size;
	unsigned int   levels;
	char           path[ PL_SYSTEM_MAX_PATH ];
	PLImageFormat  format;
	PLColourFormat colour_format;
	unsigned int   flags;
} QmImage;

enum
{
	PL_IMAGE_FILEFORMAT_ALL = 0,

	QM_OS_BIT_FLAG( PL_IMAGE_FILEFORMAT_TGA, 0 ),
	QM_OS_BIT_FLAG( PL_IMAGE_FILEFORMAT_PNG, 1 ),
	QM_OS_BIT_FLAG( PL_IMAGE_FILEFORMAT_JPG, 2 ),
	QM_OS_BIT_FLAG( PL_IMAGE_FILEFORMAT_BMP, 3 ),
	QM_OS_BIT_FLAG( PL_IMAGE_FILEFORMAT_PSD, 4 ),
	QM_OS_BIT_FLAG( PL_IMAGE_FILEFORMAT_GIF, 5 ),
	QM_OS_BIT_FLAG( PL_IMAGE_FILEFORMAT_HDR, 6 ),
	QM_OS_BIT_FLAG( PL_IMAGE_FILEFORMAT_PIC, 7 ),
	QM_OS_BIT_FLAG( PL_IMAGE_FILEFORMAT_PNM, 8 ),
	QM_OS_BIT_FLAG( PL_IMAGE_FILEFORMAT_FTX, 9 ),
	QM_OS_BIT_FLAG( PL_IMAGE_FILEFORMAT_3DF, 10 ),
	QM_OS_BIT_FLAG( PL_IMAGE_FILEFORMAT_TIM, 11 ),
	QM_OS_BIT_FLAG( PL_IMAGE_FILEFORMAT_SWL, 12 ),
	QM_OS_BIT_FLAG( PL_IMAGE_FILEFORMAT_QOI, 13 ),
	QM_OS_BIT_FLAG( PL_IMAGE_FILEFORMAT_DDS, 14 ),
	QM_OS_BIT_FLAG( PL_IMAGE_FILEFORMAT_RSB, 15 ),
	QM_OS_BIT_FLAG( PL_IMAGE_FILEFORMAT_TEX, 16 ),
	QM_OS_BIT_FLAG( PL_IMAGE_FILEFORMAT_ANGEL_TEX, 17 ),
	QM_OS_BIT_FLAG( PL_IMAGE_FILEFORMAT_DTX, 18 ),
	QM_OS_BIT_FLAG( PL_IMAGE_FILEFORMAT_HSM, 19 ),
};

PL_EXTERN_C

#if !defined( PL_COMPILE_PLUGIN )

void PlRegisterImageLoader( const char *extension, QmImage *( *ParseFile )( QmFsFile *path ) );
void PlRegisterStandardImageLoaders( unsigned int flags );
void PlClearImageLoaders( void );

QmImage *PlCreateImage( const void *buf, unsigned int w, unsigned int h, unsigned int numFrames, PLColourFormat col, PLImageFormat dat );
void     PlDestroyImage( QmImage *image );

QmImage *qm_image_load( const char *path );
QmImage *qm_image_parse( QmFsFile *file );
bool     qm_image_write( const QmImage *image, const char *path, unsigned int quality );

bool PlConvertPixelFormat( QmImage *image, PLImageFormat new_format );

void qm_image_invert_colour( QmImage *image );
void PlClearImageAlpha( QmImage *image );
void qm_image_replace_colour( QmImage *image, QmMathColour4ub target, QmMathColour4ub dest );

bool PlFlipImageVertical( QmImage *image );

void   PlFreeImage( QmImage *image );
size_t qm_image_get_format_size( PLImageFormat format, unsigned int width, unsigned int height );

unsigned int PlGetImageFormatPixelSize( PLImageFormat format );
unsigned int PlGetNumImageFormatChannels( PLImageFormat format );

bool PlImageHasAlpha( const QmImage *image );

const char **PlGetSupportedImageFormats( unsigned int *numElements );

unsigned int qm_image_get_width( const QmImage *image );
unsigned int qm_image_get_height( const QmImage *image );

PLImageFormat PlGetImageFormat( const QmImage *image );

void        *PlGetImageData( QmImage *image, unsigned int frame, unsigned int mip );
unsigned int PlGetImageDataSize( const QmImage *image );

QmImage *qm_image_resize( QmImage *image, unsigned int newWidth, unsigned int newHeight );
QmImage *qm_image_crop( QmImage *image, unsigned int newWidth, unsigned int newHeight, unsigned int xOffset, unsigned int yOffset );

///////////////////////////////////////////////////////////////////////////////////////////////////
// Compression

QmImage *qm_image_compress_s3tc( const QmImage *src, PLImageFormat dstFormat );

void qm_image_compression_decompress_dxt1( unsigned int width, unsigned int height, const unsigned char *blockStorage, unsigned char *image );
void qm_image_compression_decompress_dxt3( unsigned int width, unsigned int height, const unsigned char *blockStorage, unsigned char *image );
void qm_image_compression_decompress_dxt5( unsigned int width, unsigned int height, const unsigned char *blockStorage, unsigned char *image );

///////////////////////////////////////////////////////////////////////////////////////////////////

QmImage *qm_image_3df_parse( QmFsFile *file );
QmImage *qm_image_ftx_parse( QmFsFile *file );
QmImage *qm_image_tim_parse( QmFsFile *file );
QmImage *qm_image_swl_parse( QmFsFile *file );
QmImage *qm_image_qoi_parse( QmFsFile *file );
QmImage *qm_image_dds_parse( QmFsFile *file );
QmImage *qm_image_hsm_parse( QmFsFile *file );
QmImage *qm_image_rsb_parse( QmFsFile *file );
QmImage *qm_image_3dr_parse( QmFsFile *file );
QmImage *qm_image_angel_tex_parse( QmFsFile *file );
QmImage *qm_image_dtx_parse( QmFsFile *file );

bool qm_image_qoi_write( const QmImage *image, const char *path );

#endif

PL_EXTERN_C_END
