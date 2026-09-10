// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "qm_math.h"

#if defined( __cplusplus )
extern "C"
{
#endif

	/////////////////////////////////////////////////////////////////////////////////////
	// Vector2i
	/////////////////////////////////////////////////////////////////////////////////////

	typedef struct QmMathVector2i
	{
		union
		{
			struct
			{
				int x;
				int y;
			};

			int v[ 2 ];
		};
	} QmMathVector2i;

#define QM_MATH_VECTOR2I( X, Y ) \
	( QmMathVector2i ) { .x = ( X ), .y = ( Y ) }

	static inline QmMathVector2i qm_math_vector2i( const int x, const int y )
	{
		return QM_MATH_VECTOR2I( x, y );
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Vector3i
	/////////////////////////////////////////////////////////////////////////////////////

	typedef struct QmMathVector3i
	{
		union
		{
			struct
			{
				int x;
				int y;
				int z;
			};

			int v[ 3 ];
		};
	} QmMathVector3i;

#define QM_MATH_VECTOR3I( X, Y, Z ) \
	( QmMathVector3i ) { .x = ( X ), .y = ( Y ), .z = ( Z ) }

	static inline QmMathVector3i qm_math_vector3i( const int x, const int y, const int z )
	{
		return QM_MATH_VECTOR3I( x, y, z );
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Vector4i
	/////////////////////////////////////////////////////////////////////////////////////

	typedef struct QmMathVector4i
	{
		union
		{
			struct
			{
				int x;
				int y;
				int z;
				int w;
			};

			int v[ 4 ];
		};
	} QmMathVector4i;

#define QM_MATH_VECTOR4I( X, Y, Z, W ) \
	( QmMathVector4i ) { .x = ( X ), .y = ( Y ), .z = ( Z ), .w = ( W ) }

	static inline QmMathVector4i qm_math_vector4i( const int x, const int y, const int z, const int w )
	{
		return QM_MATH_VECTOR4I( x, y, z, w );
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Vector2f
	/////////////////////////////////////////////////////////////////////////////////////

	typedef struct QmMathVector2f
	{
		union
		{
			struct
			{
				float x;
				float y;
			};

			float v[ 2 ];
		};
	} QmMathVector2f;

#define QM_MATH_VECTOR2F( X, Y ) \
	( QmMathVector2f ) { .x = ( X ), .y = ( Y ) }

	static inline QmMathVector2f qm_math_vector2f( const float x, const float y )
	{
		return QM_MATH_VECTOR2F( x, y );
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Vector3f
	/////////////////////////////////////////////////////////////////////////////////////

	typedef struct QmMathVector3f
	{
		union
		{
			struct
			{
				float x;
				float y;
				float z;
			};

			float v[ 3 ];
		};
	} QmMathVector3f;

#define QM_MATH_VECTOR3F( X, Y, Z ) \
	( QmMathVector3f ) { .x = ( X ), .y = ( Y ), .z = ( Z ) }

	static inline QmMathVector3f qm_math_vector3f( const float x, const float y, const float z )
	{
		return QM_MATH_VECTOR3F( x, y, z );
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Vector4f
	/////////////////////////////////////////////////////////////////////////////////////

	typedef struct QmMathVector4f
	{
		union
		{
			struct
			{
				float x;
				float y;
				float z;
				float w;
			};

			float v[ 4 ];
		};
	} QmMathVector4f;

#define QM_MATH_VECTOR4F( X, Y, Z, W ) \
	( QmMathVector4f ) { .x = ( X ), .y = ( Y ), .z = ( Z ), .w = ( W ) }

	static inline QmMathVector4f qm_math_vector4f( const float x, const float y, const float z, const float w )
	{
		return QM_MATH_VECTOR4F( x, y, z, w );
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Zero
	/////////////////////////////////////////////////////////////////////////////////////

#if ( __STDC_VERSION__ >= 202000L )
	static constexpr QmMathVector2i QM_MATH_VECTOR2I_ZERO = ( QmMathVector2i ) {};
	static constexpr QmMathVector2f QM_MATH_VECTOR2F_ZERO = ( QmMathVector2f ) {};
	static constexpr QmMathVector3f QM_MATH_VECTOR3F_ZERO = ( QmMathVector3f ) {};
	static constexpr QmMathVector4f QM_MATH_VECTOR4F_ZERO = ( QmMathVector4f ) {};
#else
#	define QM_MATH_VECTOR2I_ZERO \
		( QmMathVector2i ) {}
#	define QM_MATH_VECTOR2F_ZERO \
		( QmMathVector2f ) {}
#	define QM_MATH_VECTOR3F_ZERO \
		( QmMathVector3f ) {}
#	define QM_MATH_VECTOR4F_ZERO \
		( QmMathVector4f ) {}
#endif

	/////////////////////////////////////////////////////////////////////////////////////
	// Conversion
	/////////////////////////////////////////////////////////////////////////////////////

#define QM_MATH_VECTOR2F_TO_3F( IN ) QM_MATH_VECTOR3F( ( IN ).x, ( IN ).y, 0.0f )
#define QM_MATH_VECTOR2F_TO_4F( IN ) QM_MATH_VECTOR4F( ( IN ).x, ( IN ).y, 0.0f, 0.0f )

#define QM_MATH_VECTOR3F_TO_2F( IN ) QM_MATH_VECTOR2F( ( IN ).x, ( IN ).y )
#define QM_MATH_VECTOR3F_TO_4F( IN ) QM_MATH_VECTOR4F( ( IN ).x, ( IN ).y, ( IN ).z, 0.0f )

#define QM_MATH_VECTOR4F_TO_2F( IN ) QM_MATH_VECTOR2F( ( IN ).x, ( IN ).y )
#define QM_MATH_VECTOR4F_TO_3F( IN ) QM_MATH_VECTOR3F( ( IN ).x, ( IN ).y, ( IN ).x )

	/////////////////////////////////////////////////////////////////////////////////////
	// Print
	/////////////////////////////////////////////////////////////////////////////////////

	const char *qm_math_vector2f_print( QmMathVector2f src, char *dst, unsigned int dstSize );
	const char *qm_math_vector3f_print( QmMathVector3f src, char *dst, unsigned int dstSize );
	const char *qm_math_vector4f_print( QmMathVector4f src, char *dst, unsigned int dstSize );

	/////////////////////////////////////////////////////////////////////////////////////
	// Clamp
	/////////////////////////////////////////////////////////////////////////////////////

	static inline QmMathVector2f qm_math_vector2f_clamp( const QmMathVector2f src, const float min, const float max )
	{
		return qm_math_vector2f( QM_MATH_CLAMP( min, src.x, max ),
		                         QM_MATH_CLAMP( min, src.y, max ) );
	}

	static inline QmMathVector3f qm_math_vector3f_clamp( const QmMathVector3f src, const float min, const float max )
	{
		return qm_math_vector3f( QM_MATH_CLAMP( min, src.x, max ),
		                         QM_MATH_CLAMP( min, src.y, max ),
		                         QM_MATH_CLAMP( min, src.z, max ) );
	}

	static inline QmMathVector4f qm_math_vector4f_clamp( const QmMathVector4f src, const float min, const float max )
	{
		return qm_math_vector4f( QM_MATH_CLAMP( min, src.x, max ),
		                         QM_MATH_CLAMP( min, src.y, max ),
		                         QM_MATH_CLAMP( min, src.z, max ),
		                         QM_MATH_CLAMP( min, src.w, max ) );
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Compare
	/////////////////////////////////////////////////////////////////////////////////////

	static inline bool qm_math_vector2f_compare( const QmMathVector2f a, const QmMathVector2f b )
	{
		return a.x == b.x && a.y == b.y;
	}

	static inline bool qm_math_vector3f_compare( const QmMathVector3f a, const QmMathVector3f b )
	{
		return a.x == b.x && a.y == b.y && a.z == b.z;
	}

	static inline bool qm_math_vector4f_compare( const QmMathVector4f a, const QmMathVector4f b )
	{
		return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Add
	/////////////////////////////////////////////////////////////////////////////////////

	static inline QmMathVector2f qm_math_vector2f_add( const QmMathVector2f src, const QmMathVector2f add )
	{
		return qm_math_vector2f( src.x + add.x, src.y + add.y );
	}

	static inline QmMathVector2f qm_math_vector2f_add_float( const QmMathVector2f src, const float add )
	{
		return qm_math_vector2f( src.x + add, src.y + add );
	}

	static inline QmMathVector3f qm_math_vector3f_add( const QmMathVector3f src, const QmMathVector3f add )
	{
		return qm_math_vector3f( src.x + add.x, src.y + add.y, src.z + add.z );
	}

	static inline QmMathVector3f qm_math_vector3f_add_float( const QmMathVector3f src, const float add )
	{
		return qm_math_vector3f( src.x + add, src.y + add, src.z + add );
	}

	static inline QmMathVector4f qm_math_vector4f_add( const QmMathVector4f src, const QmMathVector4f add )
	{
		return qm_math_vector4f( src.x + add.x, src.y + add.y, src.z + add.z, src.w + add.w );
	}

	static inline QmMathVector4f qm_math_vector4f_add_float( const QmMathVector4f src, const float add )
	{
		return qm_math_vector4f( src.x + add, src.y + add, src.z + add, src.w + add );
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Invert
	/////////////////////////////////////////////////////////////////////////////////////

	static inline QmMathVector2f qm_math_vector2f_invert( const QmMathVector2f src )
	{
		return qm_math_vector2f( -src.x, -src.y );
	}

	static inline QmMathVector3f qm_math_vector3f_invert( const QmMathVector3f src )
	{
		return qm_math_vector3f( -src.x, -src.y, -src.z );
	}

	static inline QmMathVector4f qm_math_vector4f_invert( const QmMathVector4f src )
	{
		return qm_math_vector4f( -src.x, -src.y, -src.z, -src.z );
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Subtract
	/////////////////////////////////////////////////////////////////////////////////////

	static inline QmMathVector2f qm_math_vector2f_sub( const QmMathVector2f a, const QmMathVector2f b )
	{
		return qm_math_vector2f( a.x - b.x, a.y - b.y );
	}

	static inline QmMathVector3f qm_math_vector3f_sub( const QmMathVector3f a, const QmMathVector3f b )
	{
		return qm_math_vector3f( a.x - b.x, a.y - b.y, a.z - b.z );
	}

	static inline QmMathVector4f qm_math_vector4f_sub( const QmMathVector4f a, const QmMathVector4f b )
	{
		return qm_math_vector4f( a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w );
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Division
	/////////////////////////////////////////////////////////////////////////////////////

	static inline QmMathVector2f qm_math_vector2f_div( const QmMathVector2f src, const QmMathVector2f div )
	{
		return qm_math_vector2f( src.x / div.x, src.y / div.y );
	}

	static inline QmMathVector2f qm_math_vector2f_div_float( const QmMathVector2f src, const float div )
	{
		return qm_math_vector2f( src.x / div, src.y / div );
	}

	static inline QmMathVector3f qm_math_vector3f_div( const QmMathVector3f src, const QmMathVector3f div )
	{
		return qm_math_vector3f( src.x / div.x, src.y / div.y, src.z / div.z );
	}

	static inline QmMathVector3f qm_math_vector3f_div_float( const QmMathVector3f src, const float div )
	{
		return qm_math_vector3f( src.x / div, src.y / div, src.z / div );
	}

	static inline QmMathVector4f qm_math_vector4f_div( const QmMathVector4f src, const QmMathVector4f div )
	{
		return qm_math_vector4f( src.x / div.x, src.y / div.y, src.z / div.z, src.w / div.w );
	}

	static inline QmMathVector4f qm_math_vector4f_div_float( const QmMathVector4f src, const float div )
	{
		return qm_math_vector4f( src.x / div, src.y / div, src.z / div, src.w / div );
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Scale
	/////////////////////////////////////////////////////////////////////////////////////

	static inline QmMathVector2f qm_math_vector2f_scale( const QmMathVector2f src, const QmMathVector2f scale )
	{
		return qm_math_vector2f( src.x * scale.x, src.y * scale.y );
	}

	static inline QmMathVector2f qm_math_vector2f_scale_float( const QmMathVector2f src, const float scale )
	{
		return qm_math_vector2f_scale( src, qm_math_vector2f( scale, scale ) );
	}

	static inline QmMathVector3f qm_math_vector3f_scale( const QmMathVector3f src, const QmMathVector3f scale )
	{
		return qm_math_vector3f( src.x * scale.x, src.y * scale.y, src.z * scale.z );
	}

	static inline QmMathVector3f qm_math_vector3f_scale_float( const QmMathVector3f src, const float scale )
	{
		return qm_math_vector3f_scale( src, qm_math_vector3f( scale, scale, scale ) );
	}

	static inline QmMathVector4f qm_math_vector4f_scale( const QmMathVector4f src, const QmMathVector4f scale )
	{
		return qm_math_vector4f( src.x * scale.x, src.y * scale.y, src.z * scale.z, src.w * scale.w );
	}

	static inline QmMathVector4f qm_math_vector4f_scale_float( const QmMathVector4f src, const float scale )
	{
		return qm_math_vector4f_scale( src, qm_math_vector4f( scale, scale, scale, scale ) );
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Dot Product
	/////////////////////////////////////////////////////////////////////////////////////

	static inline float qm_math_vector2f_dot_product( const QmMathVector2f a, const QmMathVector2f b )
	{
		return a.x * b.x + a.y * b.y;
	}

	static inline float qm_math_vector3f_dot_product( const QmMathVector3f a, const QmMathVector3f b )
	{
		return a.x * b.x + a.y * b.y + a.z * b.z;
	}

	static inline float qm_math_vector4f_dot_product( const QmMathVector4f a, const QmMathVector4f b )
	{
		return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Cross Product
	/////////////////////////////////////////////////////////////////////////////////////

	static inline QmMathVector3f qm_math_vector3f_cross_product( const QmMathVector3f a, const QmMathVector3f b )
	{
		return qm_math_vector3f(
		        a.y * b.z - a.z * b.y,
		        a.z * b.x - a.x * b.z,
		        a.x * b.y - a.y * b.x );
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Min
	/////////////////////////////////////////////////////////////////////////////////////

	static inline QmMathVector3f qm_math_vector3f_min( const QmMathVector3f a, const QmMathVector3f b )
	{
		return qm_math_vector3f(
		        a.x < b.x ? a.x : b.x,
		        a.y < b.y ? a.y : b.y,
		        a.z < b.z ? a.z : b.z );
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Max
	/////////////////////////////////////////////////////////////////////////////////////

	static inline QmMathVector3f qm_math_vector3f_max( const QmMathVector3f a, const QmMathVector3f b )
	{
		return qm_math_vector3f(
		        a.x > b.x ? a.x : b.x,
		        a.y > b.y ? a.y : b.y,
		        a.z > b.z ? a.z : b.z );
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Length
	/////////////////////////////////////////////////////////////////////////////////////

	float qm_math_vector2f_length( QmMathVector2f src );
	float qm_math_vector3f_length( QmMathVector3f src );

	/////////////////////////////////////////////////////////////////////////////////////
	// Distance
	/////////////////////////////////////////////////////////////////////////////////////

	static inline float qm_math_vector2f_distance( const QmMathVector2f a, const QmMathVector2f b )
	{
		return qm_math_vector2f_length( qm_math_vector2f_sub( a, b ) );
	}

	static inline float qm_math_vector3f_distance( const QmMathVector3f a, const QmMathVector3f b )
	{
		return qm_math_vector3f_length( qm_math_vector3f_sub( a, b ) );
	}

	/////////////////////////////////////////////////////////////////////////////////////
	// Normalize
	/////////////////////////////////////////////////////////////////////////////////////

	static inline QmMathVector2f qm_math_vector2f_normalize( const QmMathVector2f src )
	{
		const float length = qm_math_vector2f_length( src );
		return qm_math_vector2f( src.x / length, src.y / length );
	}

	static inline QmMathVector3f qm_math_vector3f_normalize( const QmMathVector3f src )
	{
		const float length = qm_math_vector3f_length( src );
		if ( length != 0.f )
		{
			return qm_math_vector3f( src.x / length, src.y / length, src.z / length );
		}

		return src;
	}

	/////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////

	// A few different helper methods -
	// these should likely go somewhere else, but they can live here for now

	/**
	 * Compute a radius from a given number of vertices.
	 * @param vertices Array of vertices to use for determining radius.
	 * @param numVertices The number of vertices in the array.
	 * @return The radius.
	 */
	float qm_math_compute_radius( const QmMathVector3f *vertices, unsigned int numVertices );

	/**
	 * Compute the min and max volume from a given number of vertices.
	 * @param vertices Array of vertices to use for determining min and max.
	 * @param numVertices The number of vertices in the array.
	 * @param minsDst The destination for the min.
	 * @param maxsDst The destination for the max.
	 * @param absolute
	 */
	void qm_math_compute_min_max( const QmMathVector3f *vertices, unsigned int numVertices, QmMathVector3f *minsDst, QmMathVector3f *maxsDst, bool absolute );

	/**
	 * Compute triangle normal.
	 */
	QmMathVector3f qm_math_compute_triangle_normal( QmMathVector3f a, QmMathVector3f b, QmMathVector3f c );

	/**
	 * Compute the normal based on a given set of vertices.
	 * @param vertices Array of vertices to use for computing the face normal.
	 * @param numVertices The number of vertices in the array.
	 * @return Normalized normal for the face.
	 */
	QmMathVector3f qm_math_compute_polygon_normal( const QmMathVector3f *vertices, unsigned int numVertices );

	/**
	 * Determine whether a polygon is convex or not.
	 * @param vertices Array of vertices.
	 * @param numVertices The number of vertices in the array.
	 * @return Returns true if the given polygon is determined to be convex.
	 */
	bool qm_math_is_polygon_convex( const QmMathVector2f *vertices, unsigned int numVertices );

#if defined( __cplusplus )
};
#endif
