/*
MIT License

Copyright (c) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_package.h>
#include <plcore/pl_image.h>

typedef uint16_t PLPluginInterfaceVersion[ 2 ];

/**
 * Functions exported from the platform library.
 */
typedef struct PLPluginExportTable {
	PLPluginInterfaceVersion version;

	void ( *ReportError )( PLFunctionResult resultType, const char *function, const char *message, ... );
	const char *( *GetError )( void );

	void *( *MAlloc )( size_t size, bool abortOnFail );
	void *( *CAlloc )( size_t num, size_t size, bool abortOnFail );
	void *( *ReAlloc )( void *ptr, size_t newSize, bool abortOnFail );
	void ( *Free )( void *ptr );

	/**
	 * FILE API
	 **/

	QmFsFile *( *OpenFile )( const char *path, bool cache );
	void ( *CloseFile )( QmFsFile *file );

	const void *( *GetFileData )( const QmFsFile *file );
	size_t ( *GetFileSize )( const QmFsFile *file );

	/**
	 * IMAGE API
	 **/

	size_t ( *GetImageSize )( PLImageFormat format, unsigned int width, unsigned int height );

	/** v2.0 ************************************************/

	/**
	 * SCRIPT API
	 **/

	void ( *SkipWhitespace )( const char **p );
	void ( *SkipLine )( const char **p );

	const char *( *ParseEnclosedString )( const char **p, char *dest, size_t size );
	const char *( *ParseToken )( const char **p, char *dest, size_t size );

	/** v9.2 ************************************************/
	const char *( *SetupPath )( PLPath dst, bool truncate, const char *msg, ... );
} PLPluginExportTable;

/* be absolutely sure to change this whenever the API is updated! */
#define PL_PLUGIN_INTERFACE_VERSION_MAJOR 10
#define PL_PLUGIN_INTERFACE_VERSION_MINOR 0
#define PL_PLUGIN_INTERFACE_VERSION \
	( uint16_t[ 2 ] ) { PL_PLUGIN_INTERFACE_VERSION_MAJOR, PL_PLUGIN_INTERFACE_VERSION_MINOR }

/* 2023-04-10
 * - Added Path API
 *
 * 2023-01-01
 * - Exposed missing pl_image API
 *
 * 2022-11-12
 * - GenerateChecksumCRC32 calling convention has been altered
 *
 * 2022-11-01
 * - Strings API is now exposed
 * - CreateImage2, which now provides an option for storing multiple frames
 *
 * 2022-07-02
 * - Now possible to attach a variable to a console var
 *
 * 2022-06-30
 * - RegisterConsoleVariable and RegisterConsoleCommand have been altered
 * - Layout of the PLConsoleVariable struct was changed
 *
 * 2022-01-12
 * - Expose CacheFile functionality
 *
 * 2022-01-10
 * - RegisterImageLoader now takes a File handle
 *
 * 2021-10-03
 * - Memory allocation functions now take an 'abortOnFail' parameter
 *
 * 2021-04-22
 * - Removed some functions from the default interface
 *
 * 2021-03-29;
 * - changed how version is queried and introduced minor
 * - plugins can now push log messages
 * - plugins can now create and execute commands via console api
 * - introduced graphics api interface
 * */
