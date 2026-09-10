// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Vector math methods.
// Author:  Mark E. Sowden

#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <float.h>

#include "qmmath/public/qm_math_vector.h"

float qm_math_compute_radius( const QmMathVector3f *vertices, unsigned int numVertices )
{
	float maxRadius = 0.0f;
	for ( unsigned int i = 0; i < numVertices; ++i )
	{
		float radius = qm_math_vector3f_length( vertices[ i ] );
		if ( radius > maxRadius )
		{
			maxRadius = radius;
		}
	}

	return maxRadius;
}

void qm_math_compute_min_max( const QmMathVector3f *vertices, const unsigned int numVertices, QmMathVector3f *minsDst, QmMathVector3f *maxsDst, const bool absolute )
{
	assert( minsDst != nullptr && maxsDst != nullptr );

	*minsDst = ( QmMathVector3f ) {};
	*maxsDst = ( QmMathVector3f ) {};

	if ( absolute )
	{
		*maxsDst = qm_math_vector3f( vertices[ 0 ].x, vertices[ 0 ].y, vertices[ 0 ].z );
		*minsDst = qm_math_vector3f( vertices[ 0 ].x, vertices[ 0 ].y, vertices[ 0 ].z );

		for ( unsigned int i = 0; i < numVertices; ++i )
		{
			if ( maxsDst->x < vertices[ i ].x ) { maxsDst->x = vertices[ i ].x; }
			if ( maxsDst->y < vertices[ i ].y ) { maxsDst->y = vertices[ i ].y; }
			if ( maxsDst->z < vertices[ i ].z ) { maxsDst->z = vertices[ i ].z; }
			if ( minsDst->x > vertices[ i ].x ) { minsDst->x = vertices[ i ].x; }
			if ( minsDst->y > vertices[ i ].y ) { minsDst->y = vertices[ i ].y; }
			if ( minsDst->z > vertices[ i ].z ) { minsDst->z = vertices[ i ].z; }
		}
	}
	else
	{
		// this technically still doesn't, but it's better
		float max = FLT_MIN;
		float min = FLT_MAX;
		for ( unsigned int i = 0; i < numVertices; ++i )
		{
			if ( vertices[ i ].x > max ) { max = vertices[ i ].x; }
			if ( vertices[ i ].y > max ) { max = vertices[ i ].y; }
			if ( vertices[ i ].z > max ) { max = vertices[ i ].z; }
			if ( vertices[ i ].x < min ) { min = vertices[ i ].x; }
			if ( vertices[ i ].y < min ) { min = vertices[ i ].y; }
			if ( vertices[ i ].z < min ) { min = vertices[ i ].z; }
		}

		if ( min < 0 ) { min *= -1; }
		if ( max < 0 ) { max *= -1; }

		float abs = min > max ? min : max;
		*maxsDst  = qm_math_vector3f( abs, abs, abs );
		*minsDst  = qm_math_vector3f( -abs, -abs, -abs );
	}
}

QmMathVector3f qm_math_compute_triangle_normal( const QmMathVector3f a, const QmMathVector3f b, const QmMathVector3f c )
{
	const QmMathVector3f x = qm_math_vector3f( c.x - b.x, c.y - b.y, c.z - b.z );
	const QmMathVector3f y = qm_math_vector3f( a.x - b.x, a.y - b.y, a.z - b.z );
	return qm_math_vector3f_normalize( qm_math_vector3f_cross_product( x, y ) );
}

QmMathVector3f qm_math_compute_polygon_normal( const QmMathVector3f *vertices, unsigned int numVertices )
{
	QmMathVector3f normal = {};
	for ( unsigned int i = 0; i < numVertices; i += 3 )
	{
		const QmMathVector3f a = vertices[ i ];
		const QmMathVector3f b = vertices[ i + 1 ];
		const QmMathVector3f c = vertices[ i + 2 ];
		const QmMathVector3f n = qm_math_compute_triangle_normal( a, b, c );

		normal = qm_math_vector3f_add( normal, n );
	}

	return qm_math_vector3f_normalize( normal );
}

bool qm_math_is_polygon_convex( const QmMathVector2f *vertices, const unsigned int numVertices )
{
	if ( numVertices < 4 )
	{
		return true;
	}

	bool sign = false;
	for ( unsigned int i = 0; i < numVertices; ++i )
	{
		QmMathVector2f a;
		a.x = vertices[ ( i + 2 ) % numVertices ].x - vertices[ ( i + 1 ) % numVertices ].x;
		a.y = vertices[ ( i + 2 ) % numVertices ].y - vertices[ ( i + 1 ) % numVertices ].y;

		QmMathVector2f b;
		b.x = vertices[ i ].x - vertices[ ( i + 1 ) % numVertices ].x;
		b.y = vertices[ i ].y - vertices[ ( i + 1 ) % numVertices ].y;

		float cp = a.x * b.y - a.y * b.x;
		if ( i == 0 )
		{
			sign = cp > 0.0f;
		}
		else if ( sign != ( cp > 0 ) )
		{
			return false;
		}
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// Print
/////////////////////////////////////////////////////////////////////////////////////

const char *qm_math_vector2f_print( const QmMathVector2f src, char *dst, const unsigned int dstSize )
{
	snprintf( dst, dstSize, "%f %f", src.x, src.y );
	return dst;
}

const char *qm_math_vector3f_print( const QmMathVector3f src, char *dst, const unsigned int dstSize )
{
	snprintf( dst, dstSize, "%f %f %f", src.x, src.y, src.z );
	return dst;
}

const char *qm_math_vector4f_print( const QmMathVector4f src, char *dst, const unsigned int dstSize )
{
	snprintf( dst, dstSize, "%f %f %f %f", src.x, src.y, src.z, src.z );
	return dst;
}

/////////////////////////////////////////////////////////////////////////////////////
// Length
/////////////////////////////////////////////////////////////////////////////////////

float qm_math_vector2f_length( const QmMathVector2f src )
{
	return sqrtf( qm_math_vector2f_dot_product( src, src ) );
}

float qm_math_vector3f_length( const QmMathVector3f src )
{
	return sqrtf( qm_math_vector3f_dot_product( src, src ) );
}
