/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

#include <plcore/pl_image.h>

// Texture Environment Modes
typedef enum PLGTextureEnvironmentMode : uint8_t
{
	PLG_TEXTUREMODE_ADD,
	PLG_TEXTUREMODE_MODULATE,
	PLG_TEXTUREMODE_DECAL,
	PLG_TEXTUREMODE_BLEND,
	PLG_TEXTUREMODE_REPLACE,
	PLG_TEXTUREMODE_COMBINE
} PLGTextureEnvironmentMode;

typedef enum QmGfxTextureWrapMode : uint8_t
{
	PLG_TEXTURE_WRAP_MODE_REPEAT,
	PLG_TEXTURE_WRAP_MODE_MIRRORED_REPEAT,
	PLG_TEXTURE_WRAP_MODE_CLAMP_EDGE,
	PLG_TEXTURE_WRAP_MODE_CLAMP_BORDER,

	PLG_MAX_TEXTURE_WRAP_MODES
} QmGfxTextureWrapMode;

typedef enum QmGfxTextureFilter : uint8_t
{
	PLG_TEXTURE_FILTER_MIPMAP_NEAREST,
	PLG_TEXTURE_FILTER_MIPMAP_LINEAR,
	PLG_TEXTURE_FILTER_MIPMAP_LINEAR_NEAREST,
	PLG_TEXTURE_FILTER_MIPMAP_NEAREST_LINEAR,
	PLG_TEXTURE_FILTER_NEAREST,// Nearest
	PLG_TEXTURE_FILTER_LINEAR, // Linear

	PLG_MAX_TEXTURE_FILTER_MODES
} QmGfxTextureFilter;

typedef enum PLGTextureTarget
{
	PLG_TEXTURE_1D,
	PLG_TEXTURE_2D,
	PLG_TEXTURE_3D
} PLGTextureTarget;

enum PLGTextureFlag
{
	PLG_TEXTURE_FLAG_PRESERVE = ( 1 << 1 ),
	PLG_TEXTURE_FLAG_NOMIPS   = ( 1 << 2 ),
};

typedef struct PLGTextureMappingUnit
{
	bool active;

	unsigned int current_texture;
	unsigned int current_capabilities;

	PLGTextureEnvironmentMode current_envmode;
} PLGTextureMappingUnit;

typedef enum QmGfxTextureType
{
	QM_GFX_TEXTURE_TYPE_1D,
	QM_GFX_TEXTURE_TYPE_2D,
	QM_GFX_TEXTURE_TYPE_2D_MULTISAMPLE,
	QM_GFX_TEXTURE_TYPE_3D,
	QM_GFX_TEXTURE_TYPE_CUBEMAP,
} QmGfxTextureType;

static constexpr uint8_t QM_GFX_TEXTURE_MAX_CUBEMAP_FACES = 6U;

typedef struct QmGfxTexture
{
	unsigned int flags;

	unsigned int x, y;
	unsigned int w, h;

	size_t       size;
	unsigned int levels;
	unsigned int crc;

	QmGfxTextureType     type;
	QmGfxTextureFilter   filter;
	QmGfxTextureWrapMode wrapMode;

	PLImageFormat  format;
	PLColourFormat pixel;

	void *driver;// driver specific data
} QmGfxTexture;

PL_EXTERN_C

#if !defined( PL_COMPILE_PLUGIN )

PL_EXTERN QmGfxTexture *qm_gfx_texture_create( QmGfxTextureType type );

PL_EXTERN bool qm_gfx_texture_upload( QmGfxTexture *self, const QmImage *upload );

PL_EXTERN unsigned int qm_gfx_get_max_texture_size();
PL_EXTERN unsigned int qm_gfx_get_max_texture_units();

PL_EXTERN void qm_gfx_texture_set_anisotropy( QmGfxTexture *texture, unsigned int amount );
PL_EXTERN void qm_gfx_texture_set_filter( QmGfxTexture *self, QmGfxTextureFilter filter );
void           qm_gfx_texture_set_wrap_mode( QmGfxTexture *self, QmGfxTextureWrapMode wrapMode );

PL_EXTERN void qm_gfx_texture_set( const QmGfxTexture *self, unsigned int tmu );
PL_EXTERN void qm_gfx_texture_set_flags( QmGfxTexture *self, unsigned int flags );

#endif /* !defined( PL_COMPILE_PLUGIN ) */

PL_EXTERN_C_END
