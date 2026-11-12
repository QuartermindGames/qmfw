// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Zip file loader.
// Author:  Mark E. Sowden

#include <plcore/pl_linkedlist.h>

#include "package_private.h"

/*
 * todo:
 * 	- zip64 support is currently missing
 */

static constexpr unsigned int ZIP_FILE_MAGIC   = QM_OS_MAGIC_TO_NUM( 'P', 'K', '\3', '\4' );
static constexpr unsigned int ZIP_CD_MAGIC     = 0x02014b50;
static constexpr unsigned int ZIP_CD_END_MAGIC = QM_OS_MAGIC_TO_NUM( 'P', 'K', '\5', '\6' );

#define ZIP_EXCLUDE_DIRS

typedef enum ZipCompression
{
	ZIP_COMPRESSION_NONE     = 0,
	ZIP_COMPRESSION_IMPLODED = 6,
	ZIP_COMPRESSION_DEFLATED = 8,
	ZIP_COMPRESSION_LZMA     = 14,
} ZipCompression;

#pragma pack( push, 1 )

typedef struct ZipFileHeader
{
	uint32_t magic;
	uint16_t version;
	uint16_t flags;
	uint16_t compression;
	uint16_t modificationTime;
	uint16_t modificationDate;
	uint32_t checksum;
	uint32_t compressedSize;
	uint32_t uncompressedSize;
	uint16_t nameSize;
	uint16_t extraSize;
} ZipFileHeader;

typedef struct ZipCDHeader
{
	uint32_t magic;
	uint16_t versionBy;
	uint16_t versionReq;
	uint16_t flags;
	uint16_t compression;
	uint16_t modificationTime;
	uint16_t modificationDate;
	uint32_t checksum;
	uint32_t compressedSize;
	uint32_t uncompressedSize;
	uint16_t nameSize;
	uint16_t extraSize;
	uint16_t commentSize;
	uint16_t diskNoStart;
	uint16_t fileAttributes;
	uint32_t externalFileAttributes;
	uint32_t offset;
} ZipCDHeader;

typedef struct ZipCDEndHeader
{
	uint32_t magic;
	uint16_t numOfDisk;
	uint16_t numOfStart;
	uint16_t totalEntriesDisk;
	uint16_t totalEntries;
	uint32_t size;
	uint32_t offset;
	uint16_t commentSize;
} ZipCDEndHeader;

#pragma pack( pop )

QmFsPackage *PlParseZipPackage( QmFsFile *file )
{
	/* check this first, just to be sure it's indicated
	 * that it's actually a zip file */
	uint32_t magic = qm_fs_file_read_int32( file, false, NULL );
	if ( magic != ZIP_FILE_MAGIC )
	{
		PlReportErrorF( PL_RESULT_FILETYPE, "invalid magic: %X", magic );
		return NULL;
	}

	// try to iterate backwards to find the dumb end CD thing
	PLFileOffset offset = qm_fs_file_get_size( file ) - 4;
	for ( ;; offset-- )
	{
		if ( !qm_fs_file_seek( file, offset, QM_FS_SEEK_SET ) )
		{
			return NULL;
		}

		magic = QM_FS_FILE_READ_UINT32( file, false, NULL );
		if ( magic == ZIP_CD_END_MAGIC )
		{
			break;
		}
	}

	qm_fs_file_seek( file, offset, QM_FS_SEEK_SET );

	ZipCDEndHeader endHeader;
	qm_file_read( file, &endHeader, sizeof( endHeader ), 1 );
	if ( !qm_fs_file_seek( file, endHeader.offset, QM_FS_SEEK_SET ) )
	{
		return NULL;
	}

	ZipCDHeader *cdHeaders = QM_OS_MEMORY_NEW_( ZipCDHeader, endHeader.totalEntries );
	if ( cdHeaders == NULL )
	{
		return NULL;
	}

	QmFsPackage *package = PlCreatePackageHandle( qm_fs_file_get_path( file ), endHeader.totalEntries, NULL );
	if ( package != NULL )
	{
		unsigned int pi = 0;
		for ( unsigned int i = 0; i < endHeader.totalEntries; ++i )
		{
			ZipCDHeader *cdHeader = &cdHeaders[ i ];
			qm_file_read( file, cdHeader, sizeof( ZipCDHeader ), 1 );

			char *name = QM_OS_MEMORY_NEW_( char, cdHeader->nameSize + 1 );
			qm_file_read( file, name, sizeof( char ), cdHeader->nameSize );

			/* couldn't see any defined way to determine type for a zip
			 * 'file' - and some zips store indices representing directories
			 * which is great, but we're not specifically interested in these */
			if ( !( cdHeader->compressedSize == 0 && cdHeader->uncompressedSize == 0 ) )
			{
				// copy everything into the new package table
				snprintf( package->files[ pi ].name, sizeof( package->files[ pi ].name ), "%s", name );
				package->files[ pi ].compressedSize = cdHeader->compressedSize;
				package->files[ pi ].size           = cdHeader->uncompressedSize;

				switch ( cdHeader->compression )
				{
					case ZIP_COMPRESSION_DEFLATED:
						package->files[ pi ].compressionType = PL_COMPRESSION_GZIP;
						break;
					case ZIP_COMPRESSION_IMPLODED:
						package->files[ pi ].compressionType = PL_COMPRESSION_IMPLODE;
						break;
					case ZIP_COMPRESSION_NONE:
						package->files[ pi ].compressionType = PL_COMPRESSION_NONE;
						break;
					default:
						package->files[ pi ].compressionType = PL_COMPRESSION_UNKNOWN;
						break;
				}

				// did I mention how much I hate this format?
				// apparently the extra size isn't guaranteed to be the same between the cd table and the files...

				offset = qm_fs_file_get_offset( file );
				qm_fs_file_seek( file, cdHeader->offset, QM_FS_SEEK_SET );

				ZipFileHeader fileHeader;
				qm_file_read( file, &fileHeader, sizeof( ZipFileHeader ), 1 );
				package->files[ pi ].offset = cdHeader->offset + sizeof( ZipFileHeader ) + fileHeader.nameSize + fileHeader.extraSize;

				qm_fs_file_seek( file, offset, QM_FS_SEEK_SET );
				pi++;
			}

			qm_os_memory_free( name );

			// we don't support these others, so just skip 'em
			qm_fs_file_seek( file, cdHeader->extraSize + cdHeader->commentSize, QM_FS_SEEK_CUR );
		}

		// we may have excluded directories from the target, so amend
		//TODO: do we want to bother resizing the table?
		package->numFiles = pi;
	}

	qm_os_memory_free( cdHeaders );

	return package;
}

QmFsPackage *PlLoadZipPackage( const char *path )
{
	QmFsFile *file = qm_fs_file_open( path, false );
	if ( file == NULL )
	{
		return NULL;
	}

	QmFsPackage *package = PlParseZipPackage( file );
	PlCloseFile( file );
	return package;
}
