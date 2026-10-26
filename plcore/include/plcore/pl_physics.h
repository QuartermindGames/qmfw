/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_math.h>

/**
 * References:
 *	http://www.3dkingdoms.com/weekly/weekly.php?a=21
 */

PL_EXTERN_C

typedef struct PLCollisionRay
{
	QmMathVector3f origin, direction;
	float          range;
} PLCollisionRay;

typedef struct PLCollisionAABB {
	QmMathVector3f origin, mins, maxs, absOrigin;
} PLCollisionAABB;

typedef struct PLCollisionSphere {
	QmMathVector3f origin;
	float radius;
} PLCollisionSphere;

typedef struct PLCollisionPlane {
	QmMathVector3f origin, normal;
} PLCollisionPlane;

typedef struct PLCollision {
	QmMathVector3f contactNormal;
	QmMathVector3f contactPoint;
	float penetration;
} PLCollision;

#define PL_COLLISION_AABB( ORIGIN, MINS, MAXS ) \
	( PLCollisionAABB ) { ( ORIGIN ), ( MINS ), ( MAXS ) }
#define PlSetupCollisionSphere( ORIGIN, RADIUS ) \
	( PLCollisionSphere ) { ( ORIGIN ), ( RADIUS ) }

QmMathVector3f PlGetAabbAbsOrigin( const PLCollisionAABB *bounds, QmMathVector3f origin );

bool PlIsAabbIntersecting( const PLCollisionAABB *aBounds, const PLCollisionAABB *bBounds );
bool PlIsPointIntersectingAabb( const PLCollisionAABB *bounds, QmMathVector3f point );
bool PlIsSphereIntersectingAabb( const PLCollisionSphere *sphere, const PLCollisionAABB *bounds );

/**
 * Get which side of a line a point is on.
 */
inline static float PlTestPointLinePosition( const QmMathVector2f *position, const QmMathVector2f *lineStart, const QmMathVector2f *lineEnd ) {
	return ( lineEnd->x - lineStart->x ) * ( position->y - lineStart->y ) - ( lineEnd->y - lineStart->y ) * ( position->x - lineStart->x );
}

inline static bool PlIsPointIntersectingLine( const QmMathVector2f *position, const QmMathVector2f *lineStart, const QmMathVector2f *lineEnd, const QmMathVector2f *lineNormal, float *intersection ) {
	*intersection = 0.0f;

#if 0
	QmMathVector2f xCoord = ( lineEnd->x > lineStart->x ) ? QmMathVector2f( lineEnd->x, lineStart->x ) : QmMathVector2f( lineStart->x, lineEnd->x );
	QmMathVector2f yCoord = ( lineEnd->y > lineStart->y ) ? QmMathVector2f( lineEnd->y, lineStart->y ) : QmMathVector2f( lineStart->y, lineEnd->y );
	if( !( position->x > xCoord.x && position->x < xCoord.y ) && !( position->y > yCoord.x && position->y < yCoord.y ) ) {
		return false;
	}
#endif

	//if( ( position->x > xCoord.y || position->x < xCoord.x ) && ( position->y > yCoord.y || position->y < yCoord.x ) ) {
	//	return false;
	//}

	/* figure out which side of the line we're on */
	float lineSide = PlTestPointLinePosition( position, lineStart, lineEnd );
	if ( lineSide <= -1000.0f ) {
		return false;
	}

	*intersection = lineSide;

	return ( *intersection <= 1000.0f );
}

inline static bool PlTestLineIntersection( float dst1, float dst2, const QmMathVector3f *lineStart, const QmMathVector3f *lineEnd, QmMathVector3f *hit ) {
	if ( ( dst1 * dst2 ) >= 0.0f || dst1 == dst2 ) {
		return false;
	}

	for ( unsigned int i = 0; i < 3; ++i ) {
		PL_VECTOR3_I( hit, i ) = PL_VECTOR3_I( lineStart, i ) + ( PL_VECTOR3_I( lineEnd, i ) - PL_VECTOR3_I( lineStart, i ) ) * ( -dst1 / ( dst2 - dst1 ) );
	}

	return true;
}

/**
 * Checks whether or not AABB is intersecting with the given line.
 * Currently only works in 2D space (X & Z).
 */
inline static bool PlIsAabbIntersectingLine( const PLCollisionAABB *bounds, const QmMathVector2f *lineStart, const QmMathVector2f *lineEnd, const QmMathVector2f *lineNormal ) {
#if 0
	QmMathVector2f origin = QmMathVector2f( bounds->origin.x, bounds->origin.z );

	QmMathVector2f a = plAddVector2( QmMathVector2f( bounds->mins.x, bounds->mins.z ), origin );
	QmMathVector2f b = plAddVector2( QmMathVector2f( bounds->maxs.x, bounds->mins.z ), origin );
	//QmMathVector2f c = plAddVector2( QmMathVector2f( bounds->mins.x, bounds->maxs.z ), origin );
	//QmMathVector2f d = plAddVector2( QmMathVector2f( bounds->maxs.x, bounds->maxs.z ), origin );

	float aR, bR, cR, dR;
	plIsPointIntersectingLine( &a, lineStart, lineEnd, lineNormal, &aR );
	plIsPointIntersectingLine( &b, lineStart, lineEnd, lineNormal, &bR );
	//plIsPointIntersectingLine( &c, lineStart, lineEnd, lineNormal, &cR );
	//plIsPointIntersectingLine( &d, lineStart, lineEnd, lineNormal, &dR );

#	if 0
	if( x == 0.0f || y == 0.0f ) {
		return QmMathVector2f( x, y );
	}
#	endif
#endif

	return false;
}

bool PlIsSphereIntersecting( const PLCollisionSphere *aSphere, const PLCollisionSphere *bSphere );
PLCollision PlIsSphereIntersectingPlane( const PLCollisionSphere *sphere, const PLCollisionPlane *plane );

/**
 * Tests if an AABB is intersecting with a plane, and returns the point of intersection.
 *
 * @param aabb		Input AABB to test against.
 * @param plane		Input plane to test again.
 * @param result	Output result.
 * @return			True on intersection, otherwise false.
 */
bool PlIsAabbIntersectingPlane( const PLCollisionAABB *aabb, const PLCollisionPlane *plane, PLCollision *result );

bool PlIsRayIntersectingAabb( const PLCollisionAABB *bounds, const PLCollisionRay *ray, QmMathVector3f *hitPoint );

PL_EXTERN_C_END

/************************************************************/
