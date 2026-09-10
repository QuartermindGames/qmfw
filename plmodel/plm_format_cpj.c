/* SPDX-License-Identifier: MIT */
/* Copyright © 2017-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include "plm_private.h"

/**
 * Loader for 3D Realms' CPJ format - it basically just rips
 * any geometry that's available along with their bones. This
 * is also a pretty awful loader... But you get the idea.
 */

#define DEBUG_CPJ_LOADER
#if defined( DEBUG_CPJ_LOADER )
#	define dprint( ... ) printf( __VA_ARGS__ )
#else
#	define dprint( ... )
#endif

/* i know, i know, terrible c-isms */
#define CPJ_MAX_BONE_WEIGHTS 8

#define CPJ_OFFSET_START 12

typedef char CPJName[ 16 ];

typedef struct CPJSurfaceTriangle {
	unsigned int uvIndex[ 3 ];
	unsigned int texture;
	unsigned int smoothingGroup;
} CPJSurfaceTriangle;

typedef struct CPJSurface {
	unsigned int numTextures;
	CPJName *textures;

	unsigned int numUVCoords;
	QmMathVector2f *uvCoords;

	CPJSurfaceTriangle *triangles;
} CPJSurface;

typedef struct CPJBone {
	CPJName name;
	unsigned int parent;
	QmMathVector3f scale;
	PLQuaternion rotation;
	QmMathVector3f transform;
	float length;
} CPJBone;

typedef struct CPJBoneWeight {
	unsigned int boneIndex;
	float weightFactor;
} CPJBoneWeight;

typedef struct CPJVertex {
	uint8_t flags;
	QmMathVector3f position;

	unsigned int numWeights;
	CPJBoneWeight weights[ CPJ_MAX_BONE_WEIGHTS ];
} CPJVertex;

typedef struct CPJTriangle {
	uint16_t x, y, z;
} CPJTriangle;

typedef struct CPJEdge {
	uint16_t x, y;
} CPJEdge;

typedef struct CPJModel {
	unsigned int numVerts;
	CPJVertex *vertices;

	unsigned int numTriangles;
	CPJTriangle *triangles;

	unsigned int numSurfaces;
	CPJSurface *surfaces;

	unsigned int numBones;
	CPJBone *bones;

	unsigned int numSmoothingGroups;
} CPJModel;

static void CPJModel_Free( CPJModel *model ) {
	qm_os_memory_free( model->vertices );
	qm_os_memory_free( model->triangles );
	for ( unsigned int i = 0; i < model->numSurfaces; ++i ) {
		qm_os_memory_free( model->surfaces[ i ].textures );
	}
	qm_os_memory_free( model->surfaces );
	qm_os_memory_free( model->bones );
}

typedef struct CPJChunkInfo {
	PLFileOffset offset;
	uint32_t magic;
	uint32_t length;
	uint32_t version;
	uint32_t timestamp;
	uint32_t nameOffset;
} CPJChunkInfo;

static PLFileOffset SeekChunk( PLFile *file, unsigned int magic, PLFileOffset startOffset, CPJChunkInfo *out ) {
	PlFileSeek( file, startOffset, PL_SEEK_SET );

	while ( !PlIsEndOfFile( file ) ) {
		out->offset = PlGetFileOffset( file );
		out->magic = PlReadInt32( file, false, NULL );
		out->length = PlReadInt32( file, false, NULL );

		PLFileOffset nextOffset = PlGetFileOffset( file ) + out->length;
		if ( ( nextOffset % 2 ) != 0 ) {
			nextOffset += 2 - ( nextOffset % 2 );
		}

		if ( out->magic != magic ) {
			if ( out->length == 0 || out->length >= PlGetFileSize( file ) ) {
				PlReportErrorF( PL_RESULT_FILEREAD, "invalid chunk length (%d - %u)", magic, out->length );
				return 0;
			}

			if ( !PlFileSeek( file, nextOffset, PL_SEEK_SET ) ) {
				return 0;
			}

			continue;
		}

		out->version = PlReadInt32( file, false, NULL );
		out->timestamp = PlReadInt32( file, false, NULL );
		out->nameOffset = PlReadInt32( file, false, NULL );
		return nextOffset;
	}

	PlReportErrorF( PL_RESULT_FILEREAD, "failed to find specified chunk (%d)", magic );
	return 0;
}

static const char *ReadName( PLFile *file, PLFileOffset offset, char *dst, size_t dstSize ) {
	/* fetch current position */
	PLFileOffset oldOffset = PlGetFileOffset( file );
	PlFileSeek( file, offset, PL_SEEK_SET );

	for ( unsigned int i = 0; i < dstSize - 1; ++i ) {
		bool status;
		char c = PlReadInt8( file, &status );
		if ( c == '\0' || !status ) {
			break;
		}

		dst[ i ] = c;
	}

	/* restore old position */
	PlFileSeek( file, oldOffset, PL_SEEK_SET );

	return dst;
}

static void SetupSkeletalData( const CPJModel *cpjModel, PLMSkeletalModelData *skeletalModelData ) {
	PL_ZERO( skeletalModelData, sizeof( PLMSkeletalModelData ) );

	skeletalModelData->numBones = cpjModel->numBones;
	skeletalModelData->bones = QM_OS_MEMORY_NEW_( PLMBone, skeletalModelData->numBones );
	for ( unsigned int i = 0; i < skeletalModelData->numBones; ++i ) {
		skeletalModelData->bones[ i ].orientation = cpjModel->bones[ i ].rotation;
		skeletalModelData->bones[ i ].position = cpjModel->bones[ i ].transform;
		skeletalModelData->bones[ i ].parent = cpjModel->bones[ i ].parent;
		snprintf( skeletalModelData->bones[ i ].name, sizeof( skeletalModelData->bones[ i ].name ), "%s", cpjModel->bones[ i ].name );
	}

	skeletalModelData->numBoneWeights = cpjModel->numVerts;
	skeletalModelData->weights = QM_OS_MEMORY_NEW_( PLMBoneWeight, skeletalModelData->numBoneWeights );
	for ( unsigned int i = 0; i < skeletalModelData->numBoneWeights; ++i ) {
		unsigned int numWeights = cpjModel->vertices[ i ].numWeights;
		if ( numWeights >= PLM_MAX_BONE_WEIGHTS ) {
			numWeights = PLM_MAX_BONE_WEIGHTS - 1;
		}

		skeletalModelData->weights[ i ].numSubWeights = numWeights;
		for ( unsigned int j = 0; j < numWeights; ++j ) {
			skeletalModelData->weights[ i ].subWeights[ j ].boneIndex = cpjModel->vertices[ i ].weights[ j ].boneIndex;
			skeletalModelData->weights[ i ].subWeights[ j ].factor = cpjModel->vertices[ i ].weights[ j ].weightFactor;
		}
	}
}

/**
 * This basically generates a set of normals depending
 * on the given smoothing group.
 */
static QmMathVector3f **GenerateNormalsPerGroup( const CPJModel *cpjModel ) {
	QmMathVector3f **normals = QM_OS_MEMORY_NEW_( QmMathVector3f *, cpjModel->numSmoothingGroups );
	for ( unsigned int i = 0; i < cpjModel->numSmoothingGroups; ++i ) {
		normals[ i ] = QM_OS_MEMORY_NEW_( QmMathVector3f, cpjModel->numVerts );

		for ( unsigned int j = 0; j < cpjModel->numTriangles; ++j ) {
			if ( cpjModel->surfaces[ 0 ].triangles[ j ].smoothingGroup != i ) {
				continue;
			}

			normals[ i ][ cpjModel->triangles[ j ].x ] = qm_math_vector3f_add( normals[ i ][ cpjModel->triangles[ j ].x ],
			                                                           qm_math_compute_triangle_normal( cpjModel->vertices[ cpjModel->triangles[ j ].x ].position,
			                                                                                    cpjModel->vertices[ cpjModel->triangles[ j ].y ].position,
			                                                                                    cpjModel->vertices[ cpjModel->triangles[ j ].z ].position ) );
			normals[ i ][ cpjModel->triangles[ j ].y ] = qm_math_vector3f_add( normals[ i ][ cpjModel->triangles[ j ].y ],
			                                                           qm_math_compute_triangle_normal( cpjModel->vertices[ cpjModel->triangles[ j ].x ].position,
			                                                                                    cpjModel->vertices[ cpjModel->triangles[ j ].y ].position,
			                                                                                    cpjModel->vertices[ cpjModel->triangles[ j ].z ].position ) );
			normals[ i ][ cpjModel->triangles[ j ].z ] = qm_math_vector3f_add( normals[ i ][ cpjModel->triangles[ j ].z ],
			                                                           qm_math_compute_triangle_normal( cpjModel->vertices[ cpjModel->triangles[ j ].x ].position,
			                                                                                    cpjModel->vertices[ cpjModel->triangles[ j ].y ].position,
			                                                                                    cpjModel->vertices[ cpjModel->triangles[ j ].z ].position ) );
		}

		for ( unsigned int j = 0; j < cpjModel->numVerts; ++j ) {
			normals[ i ][ j ] = qm_math_vector3f_normalize( normals[ i ][ j ] );
		}
	}

	return normals;
}

PLMModel *PlmParseCpjModel( PLFile *file ) {
	unsigned int magic = PlReadInt32( file, false, NULL );
	if ( magic != QM_OS_MAGIC_TO_NUM( 'R', 'I', 'F', 'F' ) ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "unexpected magic" );
		return NULL;
	}

	unsigned int fileSize = PlReadInt32( file, false, NULL );
	if ( fileSize != ( PlGetFileSize( file ) - PlGetFileOffset( file ) ) ) {
		PlReportErrorF( PL_RESULT_FILETYPE, "unexpected header file length" );
		return NULL;
	}

	PlReadInt32( file, false, NULL ); /* sub magic */

	CPJChunkInfo chunkInfo;
	PL_ZERO_( chunkInfo );

	CPJModel cpjModel;
	PL_ZERO_( cpjModel );
	cpjModel.numSmoothingGroups = 1;

	/* first fetch the geometry description - there's only one of these it seems per model */
	if ( SeekChunk( file, QM_OS_MAGIC_TO_NUM( 'G', 'E', 'O', 'B' ), CPJ_OFFSET_START, &chunkInfo ) ) {
		static const unsigned int version = 1;
		if ( chunkInfo.version != version ) {
			CPJModel_Free( &cpjModel );
			PlReportErrorF( PL_RESULT_FILEVERSION, "unexpected GEO version (%u != %u)", chunkInfo.version, version );
			return NULL;
		}

		cpjModel.numVerts = PlReadInt32( file, false, NULL );
		uint32_t ofsVerts = PlReadInt32( file, false, NULL );
		dprint( "Num verts = %d\n", cpjModel.numVerts );

		uint32_t numEdges = PlReadInt32( file, false, NULL );
		uint32_t ofsEdges = PlReadInt32( file, false, NULL );

		cpjModel.numTriangles = PlReadInt32( file, false, NULL );
		uint32_t ofsTriangles = PlReadInt32( file, false, NULL );
		dprint( "Num triangles = %d\n", cpjModel.numTriangles );

		/* mounts */
		PlReadInt32( file, false, NULL );
		PlReadInt32( file, false, NULL );

		/* object links */
		PlReadInt32( file, false, NULL );
		PlReadInt32( file, false, NULL );

		PLFileOffset baseOffset = PlGetFileOffset( file );

		/* fetch the vertices */
		PlFileSeek( file, baseOffset + ofsVerts, PL_SEEK_SET );
		cpjModel.vertices = QM_OS_MEMORY_NEW_( CPJVertex, cpjModel.numVerts );
		for ( unsigned int i = 0; i < cpjModel.numVerts; ++i ) {
			cpjModel.vertices[ i ].flags = PlReadInt8( file, NULL ); /* flags */
			PlReadInt8( file, NULL );                                /* group */
			PlReadInt16( file, false, NULL );                        /* unused */
			PlReadInt16( file, false, NULL );                        /* edge links */
			PlReadInt16( file, false, NULL );                        /* tri links */
			PlReadInt32( file, false, NULL );                        /* first edge link */
			PlReadInt32( file, false, NULL );                        /* first tri link */

			cpjModel.vertices[ i ].position.x = PlReadFloat32( file, false, NULL );
			cpjModel.vertices[ i ].position.y = PlReadFloat32( file, false, NULL );
			cpjModel.vertices[ i ].position.z = PlReadFloat32( file, false, NULL );
		}

		/* boy this format is weird... so next, rather than fetching the triangles, we need
		 * to fetch the edges as the triangles are edge-based... and then we can get the true
		 * vertices that each triangle is *actually* using */
		PlFileSeek( file, baseOffset + ofsEdges, PL_SEEK_SET );
		CPJEdge *edges = QM_OS_MEMORY_NEW_( CPJEdge, numEdges );
		for ( unsigned int i = 0; i < numEdges; ++i ) {
			edges[ i ].x = ( uint16_t ) PlReadInt16( file, false, NULL );
			edges[ i ].y = ( uint16_t ) PlReadInt16( file, false, NULL );
			/* don't believe we need these */
			PlReadInt16( file, false, NULL );
			PlReadInt16( file, false, NULL );
			PlReadInt32( file, false, NULL );
		}

		/* and now fetch the triangles */
		PlFileSeek( file, baseOffset + ofsTriangles, PL_SEEK_SET );
		CPJTriangle *triangle = cpjModel.triangles = QM_OS_MEMORY_NEW_( CPJTriangle, cpjModel.numTriangles );
		for ( unsigned int i = 0; i < cpjModel.numTriangles; ++i, ++triangle ) {
			triangle->x = edges[ ( uint16_t ) PlReadInt16( file, false, NULL ) ].y;
			triangle->y = edges[ ( uint16_t ) PlReadInt16( file, false, NULL ) ].y;
			triangle->z = edges[ ( uint16_t ) PlReadInt16( file, false, NULL ) ].y;
			PlReadInt16( file, false, NULL ); /* unused */
		}

		qm_os_memory_free( edges );
	} else {
		CPJModel_Free( &cpjModel );
		return NULL;
	}

	/* surface chunk - there are multiple of these, per texture */
	PLFileOffset subOffset = CPJ_OFFSET_START;
	while ( ( subOffset = SeekChunk( file, QM_OS_MAGIC_TO_NUM( 'S', 'R', 'F', 'B' ), subOffset, &chunkInfo ) ) ) {
		static const unsigned int version = 1;
		if ( chunkInfo.version != version ) {
			CPJModel_Free( &cpjModel );
			PlReportErrorF( PL_RESULT_FILEVERSION, "unexpected SRF version (%u != %u)", chunkInfo.version, version );
			return NULL;
		}

		cpjModel.numSurfaces++;
		cpjModel.surfaces = qm_os_memory_realloc( cpjModel.surfaces, sizeof( CPJSurface ) * cpjModel.numSurfaces );
		CPJSurface *surface = &cpjModel.surfaces[ cpjModel.numSurfaces - 1 ];

		dprint( "Parsing surface\n" );

		surface->numTextures = PlReadInt32( file, false, NULL );
		uint32_t ofsTextures = PlReadInt32( file, false, NULL );
		dprint( "\tSurface textures = %d\n", surface->numTextures );

		/* according to the spec, this should be the same as the geo's number of tris */
		uint32_t numTriangles = PlReadInt32( file, false, NULL );
		if ( numTriangles != cpjModel.numTriangles ) {
			CPJModel_Free( &cpjModel );
			PlReportErrorF( PL_RESULT_FILEERR, "invalid number of triangles in SRFB (%u != %u)", numTriangles, cpjModel.numTriangles );
			return NULL;
		}
		dprint( "\tSurface triangles = %u\n", numTriangles );

		uint32_t ofsTriangles = PlReadInt32( file, false, NULL );

		surface->numUVCoords = PlReadInt32( file, false, NULL );
		uint32_t ofsUVCoords = PlReadInt32( file, false, NULL );

		PLFileOffset baseOffset = PlGetFileOffset( file );

		PlFileSeek( file, baseOffset + ofsTextures, PL_SEEK_SET );
		surface->textures = QM_OS_MEMORY_NEW_( CPJName, surface->numTextures );
		for ( unsigned int i = 0; i < surface->numTextures; ++i ) {
			uint32_t ofsName = PlReadInt32( file, false, NULL );
			ReadName( file, baseOffset + ofsName, surface->textures[ i ], sizeof( surface->textures[ i ] ) );
			PlReadInt32( file, false, NULL ); /* optional name */
			dprint( "\tTexture %u = %s\n", i, surface->textures[ i ] );
		}

		PlFileSeek( file, baseOffset + ofsUVCoords, PL_SEEK_SET );
		surface->uvCoords = QM_OS_MEMORY_NEW_( QmMathVector2f, surface->numUVCoords );
		for ( unsigned int i = 0; i < surface->numUVCoords; ++i ) {
			surface->uvCoords[ i ].x = PlReadFloat32( file, false, NULL );
			surface->uvCoords[ i ].y = -PlReadFloat32( file, false, NULL );
		}

		PlFileSeek( file, baseOffset + ofsTriangles, PL_SEEK_SET );
		surface->triangles = QM_OS_MEMORY_NEW_( CPJSurfaceTriangle, numTriangles );
		for ( unsigned int i = 0; i < numTriangles; ++i ) {
			for ( unsigned int j = 0; j < 3; ++j ) {
				surface->triangles[ i ].uvIndex[ j ] = PlReadInt16( file, false, NULL );
			}
			surface->triangles[ i ].texture = ( unsigned char ) PlReadInt8( file, NULL );
			PlReadInt8( file, NULL );
			PlReadInt32( file, false, NULL );

			surface->triangles[ i ].smoothingGroup = ( unsigned char ) PlReadInt8( file, NULL );
			if ( ( surface->triangles[ i ].smoothingGroup + 1 ) > cpjModel.numSmoothingGroups ) {
				cpjModel.numSmoothingGroups = surface->triangles[ i ].smoothingGroup + 1;
			}

			PlReadInt8( file, NULL );
			PlReadInt8( file, NULL );
			PlReadInt8( file, NULL );
		}
	}

	/* mesh generation is based upon the surfaces we retrieve, so if there aren't any
	 * then it could be that the cpj is buggered */
	if ( cpjModel.numSurfaces == 0 ) {
		CPJModel_Free( &cpjModel );
		PlReportErrorF( PL_RESULT_FILEERR, "no surfaces in cpj" );
		return NULL;
	}

	/* skeleton chunk, not a problem if it doesn't exist as it likely just means it's static */
	if ( SeekChunk( file, QM_OS_MAGIC_TO_NUM( 'S', 'K', 'L', 'B' ), CPJ_OFFSET_START, &chunkInfo ) ) {
		static const unsigned int version = 1;
		if ( chunkInfo.version != version ) {
			CPJModel_Free( &cpjModel );
			PlReportErrorF( PL_RESULT_FILEVERSION, "unexpected SKL version (%u != %u)", chunkInfo.version, version );
			return NULL;
		}

		cpjModel.numBones = PlReadInt32( file, false, NULL );
		uint32_t ofsBones = PlReadInt32( file, false, NULL );

		uint32_t numVerts = PlReadInt32( file, false, NULL );
		if ( numVerts != cpjModel.numVerts ) {
			CPJModel_Free( &cpjModel );
			PlReportErrorF( PL_RESULT_FILEERR, "invalid number of vertices in SKLB (%u != %u)", numVerts, cpjModel.numVerts );
			return NULL;
		}

		uint32_t ofsVerts = PlReadInt32( file, false, NULL );

		uint32_t numWeights = PlReadInt32( file, false, NULL );
		uint32_t ofsWeights = PlReadInt32( file, false, NULL );

		/* mounts */
		PlReadInt32( file, false, NULL );
		PlReadInt32( file, false, NULL );

		PLFileOffset baseOffset = PlGetFileOffset( file );

		/* and now fetch the booones */
		PlFileSeek( file, baseOffset + ofsBones, PL_SEEK_SET );
		cpjModel.bones = QM_OS_MEMORY_NEW_( CPJBone, cpjModel.numBones );
		for ( unsigned int i = 0; i < cpjModel.numBones; ++i ) {
			uint32_t ofsName = PlReadInt32( file, false, NULL );
			ReadName( file, baseOffset + ofsName, cpjModel.bones[ i ].name, sizeof( cpjModel.bones[ i ].name ) );

			cpjModel.bones[ i ].parent = ( unsigned int ) PlReadInt32( file, false, NULL );

			cpjModel.bones[ i ].scale.x = PlReadFloat32( file, false, NULL );
			cpjModel.bones[ i ].scale.y = PlReadFloat32( file, false, NULL );
			cpjModel.bones[ i ].scale.z = PlReadFloat32( file, false, NULL );

			cpjModel.bones[ i ].rotation.x = PlReadFloat32( file, false, NULL );
			cpjModel.bones[ i ].rotation.y = PlReadFloat32( file, false, NULL );
			cpjModel.bones[ i ].rotation.z = PlReadFloat32( file, false, NULL );
			cpjModel.bones[ i ].rotation.w = PlReadFloat32( file, false, NULL );

			cpjModel.bones[ i ].transform.x = PlReadFloat32( file, false, NULL );
			cpjModel.bones[ i ].transform.y = PlReadFloat32( file, false, NULL );
			cpjModel.bones[ i ].transform.z = PlReadFloat32( file, false, NULL );

			cpjModel.bones[ i ].length = PlReadFloat32( file, false, NULL );
		}

		PlFileSeek( file, baseOffset + ofsWeights, PL_SEEK_SET );
		CPJBoneWeight *weights = QM_OS_MEMORY_NEW_( CPJBoneWeight, numWeights );
		for ( unsigned int i = 0; i < numWeights; ++i ) {
			weights[ i ].boneIndex = PlReadInt32( file, false, NULL );
			weights[ i ].weightFactor = PlReadFloat32( file, false, NULL );
			/* offset... hm */
			PlReadFloat32( file, false, NULL );
			PlReadFloat32( file, false, NULL );
			PlReadFloat32( file, false, NULL );
		}

		PlFileSeek( file, baseOffset + ofsVerts, PL_SEEK_SET );
		for ( unsigned int i = 0; i < numVerts; ++i ) {
			cpjModel.vertices[ i ].numWeights = PlReadInt16( file, false, NULL );

			uint16_t vertexWeightIndex = PlReadInt16( file, false, NULL );
			for ( unsigned int j = 0; j < cpjModel.vertices[ i ].numWeights; ++j ) {
				cpjModel.vertices[ i ].weights[ j ].boneIndex = weights[ vertexWeightIndex + j ].boneIndex;
				cpjModel.vertices[ i ].weights[ j ].weightFactor = weights[ vertexWeightIndex + j ].weightFactor;
			}
		}

		qm_os_memory_free( weights );
	}

	const CPJSurface *surface = &cpjModel.surfaces[ 0 ];

	/* generate normals per smoothing group */
	QmMathVector3f **normals = GenerateNormalsPerGroup( &cpjModel );

	PLGMesh **meshes = QM_OS_MEMORY_NEW_( PLGMesh *, surface->numTextures );
	for ( unsigned int i = 0; i < surface->numTextures; ++i ) {
		meshes[ i ] = PlgCreateMesh( PLG_MESH_TRIANGLES, PLG_DRAW_STATIC, cpjModel.numTriangles, cpjModel.numVerts );
		meshes[ i ]->materialIndex = i;
		for ( unsigned int j = 0; j < cpjModel.numTriangles; ++j ) {
			if ( surface->triangles[ j ].texture != i ) {
				continue;
			}

			unsigned int x = PlgAddMeshVertex( meshes[ i ],
			                                   &cpjModel.vertices[ cpjModel.triangles[ j ].x ].position,
			                                   &normals[ surface->triangles[ j ].smoothingGroup ][ cpjModel.triangles[ j ].x ],
			                                   &PL_COLOUR_WHITE,
			                                   &surface->uvCoords[ surface->triangles[ j ].uvIndex[ 0 ] ] );
			unsigned int y = PlgAddMeshVertex( meshes[ i ],
			                                   &cpjModel.vertices[ cpjModel.triangles[ j ].y ].position,
			                                   &normals[ surface->triangles[ j ].smoothingGroup ][ cpjModel.triangles[ j ].y ],
			                                   &PL_COLOUR_WHITE,
			                                   &surface->uvCoords[ surface->triangles[ j ].uvIndex[ 1 ] ] );
			unsigned int z = PlgAddMeshVertex( meshes[ i ],
			                                   &cpjModel.vertices[ cpjModel.triangles[ j ].z ].position,
			                                   &normals[ surface->triangles[ j ].smoothingGroup ][ cpjModel.triangles[ j ].z ],
			                                   &PL_COLOUR_WHITE,
			                                   &surface->uvCoords[ surface->triangles[ j ].uvIndex[ 2 ] ] );

			PlgAddMeshTriangle( meshes[ i ], x, y, z );
		}
	}

	for ( unsigned int i = 0; i < cpjModel.numSmoothingGroups; ++i ) {
		qm_os_memory_free( normals[ i ] );
	}
	qm_os_memory_free( normals );

	PLMModel *model;
	if ( cpjModel.numBones > 0 ) {
		PLMSkeletalModelData skeletalModelData;
		SetupSkeletalData( &cpjModel, &skeletalModelData );
		model = PlmCreateSkeletalModel( meshes, surface->numTextures,
		                                skeletalModelData.bones, skeletalModelData.numBones,
		                                skeletalModelData.weights, skeletalModelData.numBoneWeights );

		// a little awkward, but weight indices need to be setup here,
		// rather than as they were earlier...as skeletal model setup allocates storage *sigh*
		// in hindsight this should've been setup before - and you instead
		// pass SkeletalModelData into the create method, maybe... ??? *bangs head on desk*

		for ( unsigned int i = 0; i < surface->numTextures; ++i ) {
			PLMSkeletalVertex *vertices = model->internal.skeletal_data.vertices[ i ];
			for ( unsigned int j = 0, k = 0; j < cpjModel.numTriangles; ++j ) {
				if ( surface->triangles[ j ].texture != model->meshes[ i ]->materialIndex ) {
					continue;
				}

				vertices[ k++ ].weightIndex = &cpjModel.vertices[ cpjModel.triangles[ j ].x ] - cpjModel.vertices;
				vertices[ k++ ].weightIndex = &cpjModel.vertices[ cpjModel.triangles[ j ].y ] - cpjModel.vertices;
				vertices[ k++ ].weightIndex = &cpjModel.vertices[ cpjModel.triangles[ j ].z ] - cpjModel.vertices;
			}
		}
	} else {
		model = PlmCreateStaticModel( meshes, surface->numTextures );
	}

	model->numMaterials = surface->numTextures;
	model->materials = QM_OS_MEMORY_NEW_( PLPath, model->numMaterials );
	for ( unsigned int i = 0; i < cpjModel.surfaces[ 0 ].numTextures; ++i ) {
		snprintf( model->materials[ i ], sizeof( PLPath ), "%s.bmp", cpjModel.surfaces[ 0 ].textures[ i ] );
	}

	CPJModel_Free( &cpjModel );

	return model;
}
