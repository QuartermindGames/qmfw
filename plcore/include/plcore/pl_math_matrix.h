/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#pragma once

#include "qmmath/public/qm_math_quaternion.h"

PL_EXTERN_C

/******************************************************************/
/* Matrices */

#define PL_M3_POS( ROW, COL ) ( ( ROW ) + 3 * ( COL ) )
#define PL_M4_POS( ROW, COL ) ( ( ROW ) + 4 * ( COL ) )

/* I know, this is disgusting... */
#define pl_m4pos( row, col ) m[ PL_M4_POS( row, col ) ]

typedef struct PLMatrix3 {
	union {
		float m[ 9 ];
		float mm[ 3 ][ 3 ];// row, col
	};
} PLMatrix3;

typedef struct PLMatrix4 {
	union {
		float m[ 16 ];
		float mm[ 4 ][ 4 ];// row, col
	};
} PLMatrix4;

static inline PLMatrix4 PlAddMatrix4( PLMatrix4 m, PLMatrix4 m2 );
static inline PLMatrix4 PlSubtractMatrix4( PLMatrix4 m, PLMatrix4 m2 );
static inline PLMatrix4 PlScaleMatrix4( PLMatrix4 m, QmMathVector3f scale );
static inline PLMatrix4 PlMultiplyMatrix4( const PLMatrix4 *m, const PLMatrix4 *m2 );
PLMatrix4               PlRotateMatrix4ByQuaternion( const QmMathQuaternion *rotation );
PLMatrix4               PlRotateMatrix4( float angle, const QmMathVector3f *axis );
static inline PLMatrix4 PlTranslateMatrix4( QmMathVector3f v );
static inline PLMatrix4 PlInverseMatrix4( const PLMatrix4 *m );

QmMathVector3f PlGetMatrix4Translation( const PLMatrix4 *m );
QmMathVector3f PlGetMatrix4Angle( const PLMatrix4 *m );
QmMathVector3f qm_math_matrix4_get_scale( const PLMatrix4 *self );

QmMathVector2f PlConvertWorldToScreen( const QmMathVector3f *position, const PLMatrix4 *viewProjMatrix, const int *viewport, float *w, bool flip );
QmMathVector3f PlConvertScreenToWorld( QmMathVector2f windowCoordinate, const PLMatrix4 *viewMatrix, const PLMatrix4 *projMatrix, const int *viewport );

void PlExtractMatrix4Directions( const PLMatrix4 *matrix, QmMathVector3f *left, QmMathVector3f *up, QmMathVector3f *forward );

/* ClearMatrix */

static inline void PlClearMatrix3( PLMatrix3 *m ) {
	QM_OS_ZERO( m, sizeof( PLMatrix3 ) );
}

static inline void PlClearMatrix4( PLMatrix4 *m ) {
	QM_OS_ZERO( m, sizeof( PLMatrix4 ) );
}

/* Identity */

static inline PLMatrix3 PlMatrix3Identity( void ) {
	PLMatrix3 m = {};
	m.mm[ 0 ][ 0 ] = 1.0f;
	m.mm[ 1 ][ 1 ] = 1.0f;
	m.mm[ 2 ][ 2 ] = 1.0f;
	return m;
}

static inline PLMatrix4 PlMatrix4Identity( void ) {
	PLMatrix4 m = {};
	m.mm[ 0 ][ 0 ] = 1.0f;
	m.mm[ 1 ][ 1 ] = 1.0f;
	m.mm[ 2 ][ 2 ] = 1.0f;
	m.mm[ 3 ][ 3 ] = 1.0f;
	return m;
}

/* Transpose */

static inline PLMatrix3 PlTransposeMatrix3( PLMatrix3 m, PLMatrix3 m2 ) {
	for ( unsigned int j = 0; j < 3; ++j ) {
		for ( unsigned int i = 0; i < 3; ++i ) {
			m.m[ PL_M3_POS( i, j ) ] = m2.m[ PL_M3_POS( j, i ) ];
		}
	}
	return m;
}

static inline PLMatrix4 PlTransposeMatrix4( const PLMatrix4 *m ) {
	PLMatrix4 out;
	for ( unsigned int j = 0; j < 4; ++j ) {
		for ( unsigned int i = 0; i < 4; ++i ) {
			out.m[ PL_M4_POS( i, j ) ] = m->m[ PL_M4_POS( j, i ) ];
		}
	}

	return out;
}

/* Add */

static inline PLMatrix3 PlAddMatrix3( PLMatrix3 m, PLMatrix3 m2 ) {
	for ( unsigned int i = 0; i < 3; ++i ) {
		for ( unsigned int j = 0; j < 3; ++j ) {
			m.m[ PL_M3_POS( i, j ) ] += m2.m[ PL_M3_POS( i, j ) ];
		}
	}
	return m;
}

static inline PLMatrix4 PlAddMatrix4( PLMatrix4 m, PLMatrix4 m2 ) {
	for ( unsigned int i = 0; i < 4; ++i ) {
		for ( unsigned int j = 0; j < 4; ++j ) {
			m.m[ PL_M4_POS( i, j ) ] += m2.m[ PL_M4_POS( i, j ) ];
		}
	}
	return m;
}

/* Subtract */

static inline PLMatrix3 PlSubtractMatrix3( PLMatrix3 m, PLMatrix3 m2 ) {
	for ( unsigned int i = 0; i < 3; ++i ) {
		for ( unsigned int j = 0; j < 3; ++j ) {
			m.m[ PL_M3_POS( i, j ) ] -= m2.m[ PL_M3_POS( i, j ) ];
		}
	}
	return m;
}

static inline PLMatrix4 PlSubtractMatrix4( PLMatrix4 m, PLMatrix4 m2 ) {
	for ( unsigned int i = 0; i < 4; ++i ) {
		for ( unsigned int j = 0; j < 4; ++j ) {
			m.m[ PL_M4_POS( i, j ) ] -= m2.m[ PL_M4_POS( i, j ) ];
		}
	}
	return m;
}

/* Multiply */

static inline PLMatrix4 PlMultiplyMatrix4( const PLMatrix4 *m, const PLMatrix4 *m2 ) {
	PLMatrix4 out;
	for ( unsigned int col = 0; col < 4; ++col ) {
		for ( unsigned int row = 0; row < 4; ++row ) {
			float sum = 0.0f;
			for ( unsigned int i = 0; i < 4; ++i ) {
				sum += m->mm[ i ][ row ] * m2->mm[ col ][ i ];
			}
			out.mm[ col ][ row ] = sum;
		}
	}

	return out;
}

/******************************************************************/
/* Utility Functions */

static inline bool PlCompareMatrix( const PLMatrix4 *m, const PLMatrix4 *m2 ) {
	for ( unsigned int i = 0; i < 4; ++i ) {
		for ( unsigned int j = 0; j < 4; ++j ) {
			if ( m->m[ PL_M4_POS( i, j ) ] != m2->m[ PL_M4_POS( i, j ) ] ) {
				return false;
			}
		}
	}
	return true;
}

static inline PLMatrix4 PlScaleMatrix4( PLMatrix4 m, QmMathVector3f scale ) {
	m.mm[ 0 ][ 0 ] *= scale.x;
	m.mm[ 0 ][ 1 ] *= scale.x;
	m.mm[ 0 ][ 2 ] *= scale.x;

	m.mm[ 1 ][ 0 ] *= scale.y;
	m.mm[ 1 ][ 1 ] *= scale.y;
	m.mm[ 1 ][ 2 ] *= scale.y;

	m.mm[ 2 ][ 0 ] *= scale.z;
	m.mm[ 2 ][ 1 ] *= scale.z;
	m.mm[ 2 ][ 2 ] *= scale.z;

	return m;
}

static inline PLMatrix4 PlTranslateMatrix4( QmMathVector3f v ) {
	PLMatrix4 m = PlMatrix4Identity();
	m.mm[ 3 ][ 0 ] = v.x;
	m.mm[ 3 ][ 1 ] = v.y;
	m.mm[ 3 ][ 2 ] = v.z;
	return m;
}

static inline PLMatrix4 PlInverseMatrix4( const PLMatrix4 *m ) {
	PLMatrix4 out;

	out.m[ 0 ] =
	        m->m[ 5 ] * m->m[ 10 ] * m->m[ 15 ] -
	        m->m[ 5 ] * m->m[ 11 ] * m->m[ 14 ] -
	        m->m[ 9 ] * m->m[ 6 ] * m->m[ 15 ] +
	        m->m[ 9 ] * m->m[ 7 ] * m->m[ 14 ] +
	        m->m[ 13 ] * m->m[ 6 ] * m->m[ 11 ] -
	        m->m[ 13 ] * m->m[ 7 ] * m->m[ 10 ];
	out.m[ 4 ] =
	        -m->m[ 4 ] * m->m[ 10 ] * m->m[ 15 ] +
	        m->m[ 4 ] * m->m[ 11 ] * m->m[ 14 ] +
	        m->m[ 8 ] * m->m[ 6 ] * m->m[ 15 ] -
	        m->m[ 8 ] * m->m[ 7 ] * m->m[ 14 ] -
	        m->m[ 12 ] * m->m[ 6 ] * m->m[ 11 ] +
	        m->m[ 12 ] * m->m[ 7 ] * m->m[ 10 ];
	out.m[ 8 ] =
	        m->m[ 4 ] * m->m[ 9 ] * m->m[ 15 ] -
	        m->m[ 4 ] * m->m[ 11 ] * m->m[ 13 ] -
	        m->m[ 8 ] * m->m[ 5 ] * m->m[ 15 ] +
	        m->m[ 8 ] * m->m[ 7 ] * m->m[ 13 ] +
	        m->m[ 12 ] * m->m[ 5 ] * m->m[ 11 ] -
	        m->m[ 12 ] * m->m[ 7 ] * m->m[ 9 ];
	out.m[ 12 ] =
	        -m->m[ 4 ] * m->m[ 9 ] * m->m[ 14 ] +
	        m->m[ 4 ] * m->m[ 10 ] * m->m[ 13 ] +
	        m->m[ 8 ] * m->m[ 5 ] * m->m[ 14 ] -
	        m->m[ 8 ] * m->m[ 6 ] * m->m[ 13 ] -
	        m->m[ 12 ] * m->m[ 5 ] * m->m[ 10 ] +
	        m->m[ 12 ] * m->m[ 6 ] * m->m[ 9 ];
	out.m[ 1 ] =
	        -m->m[ 1 ] * m->m[ 10 ] * m->m[ 15 ] +
	        m->m[ 1 ] * m->m[ 11 ] * m->m[ 14 ] +
	        m->m[ 9 ] * m->m[ 2 ] * m->m[ 15 ] -
	        m->m[ 9 ] * m->m[ 3 ] * m->m[ 14 ] -
	        m->m[ 13 ] * m->m[ 2 ] * m->m[ 11 ] +
	        m->m[ 13 ] * m->m[ 3 ] * m->m[ 10 ];
	out.m[ 5 ] =
	        m->m[ 0 ] * m->m[ 10 ] * m->m[ 15 ] -
	        m->m[ 0 ] * m->m[ 11 ] * m->m[ 14 ] -
	        m->m[ 8 ] * m->m[ 2 ] * m->m[ 15 ] +
	        m->m[ 8 ] * m->m[ 3 ] * m->m[ 14 ] +
	        m->m[ 12 ] * m->m[ 2 ] * m->m[ 11 ] -
	        m->m[ 12 ] * m->m[ 3 ] * m->m[ 10 ];
	out.m[ 9 ] =
	        -m->m[ 0 ] * m->m[ 9 ] * m->m[ 15 ] +
	        m->m[ 0 ] * m->m[ 11 ] * m->m[ 13 ] +
	        m->m[ 8 ] * m->m[ 1 ] * m->m[ 15 ] -
	        m->m[ 8 ] * m->m[ 3 ] * m->m[ 13 ] -
	        m->m[ 12 ] * m->m[ 1 ] * m->m[ 11 ] +
	        m->m[ 12 ] * m->m[ 3 ] * m->m[ 9 ];
	out.m[ 13 ] =
	        m->m[ 0 ] * m->m[ 9 ] * m->m[ 14 ] -
	        m->m[ 0 ] * m->m[ 10 ] * m->m[ 13 ] -
	        m->m[ 8 ] * m->m[ 1 ] * m->m[ 14 ] +
	        m->m[ 8 ] * m->m[ 2 ] * m->m[ 13 ] +
	        m->m[ 12 ] * m->m[ 1 ] * m->m[ 10 ] -
	        m->m[ 12 ] * m->m[ 2 ] * m->m[ 9 ];
	out.m[ 2 ] =
	        m->m[ 1 ] * m->m[ 6 ] * m->m[ 15 ] -
	        m->m[ 1 ] * m->m[ 7 ] * m->m[ 14 ] -
	        m->m[ 5 ] * m->m[ 2 ] * m->m[ 15 ] +
	        m->m[ 5 ] * m->m[ 3 ] * m->m[ 14 ] +
	        m->m[ 13 ] * m->m[ 2 ] * m->m[ 7 ] -
	        m->m[ 13 ] * m->m[ 3 ] * m->m[ 6 ];
	out.m[ 6 ] =
	        -m->m[ 0 ] * m->m[ 6 ] * m->m[ 15 ] +
	        m->m[ 0 ] * m->m[ 7 ] * m->m[ 14 ] +
	        m->m[ 4 ] * m->m[ 2 ] * m->m[ 15 ] -
	        m->m[ 4 ] * m->m[ 3 ] * m->m[ 14 ] -
	        m->m[ 12 ] * m->m[ 2 ] * m->m[ 7 ] +
	        m->m[ 12 ] * m->m[ 3 ] * m->m[ 6 ];
	out.m[ 10 ] =
	        m->m[ 0 ] * m->m[ 5 ] * m->m[ 15 ] -
	        m->m[ 0 ] * m->m[ 7 ] * m->m[ 13 ] -
	        m->m[ 4 ] * m->m[ 1 ] * m->m[ 15 ] +
	        m->m[ 4 ] * m->m[ 3 ] * m->m[ 13 ] +
	        m->m[ 12 ] * m->m[ 1 ] * m->m[ 7 ] -
	        m->m[ 12 ] * m->m[ 3 ] * m->m[ 5 ];
	out.m[ 14 ] =
	        -m->m[ 0 ] * m->m[ 5 ] * m->m[ 14 ] +
	        m->m[ 0 ] * m->m[ 6 ] * m->m[ 13 ] +
	        m->m[ 4 ] * m->m[ 1 ] * m->m[ 14 ] -
	        m->m[ 4 ] * m->m[ 2 ] * m->m[ 13 ] -
	        m->m[ 12 ] * m->m[ 1 ] * m->m[ 6 ] +
	        m->m[ 12 ] * m->m[ 2 ] * m->m[ 5 ];
	out.m[ 3 ] =
	        -m->m[ 1 ] * m->m[ 6 ] * m->m[ 11 ] +
	        m->m[ 1 ] * m->m[ 7 ] * m->m[ 10 ] +
	        m->m[ 5 ] * m->m[ 2 ] * m->m[ 11 ] -
	        m->m[ 5 ] * m->m[ 3 ] * m->m[ 10 ] -
	        m->m[ 9 ] * m->m[ 2 ] * m->m[ 7 ] +
	        m->m[ 9 ] * m->m[ 3 ] * m->m[ 6 ];
	out.m[ 7 ] =
	        m->m[ 0 ] * m->m[ 6 ] * m->m[ 11 ] -
	        m->m[ 0 ] * m->m[ 7 ] * m->m[ 10 ] -
	        m->m[ 4 ] * m->m[ 2 ] * m->m[ 11 ] +
	        m->m[ 4 ] * m->m[ 3 ] * m->m[ 10 ] +
	        m->m[ 8 ] * m->m[ 2 ] * m->m[ 7 ] -
	        m->m[ 8 ] * m->m[ 3 ] * m->m[ 6 ];
	out.m[ 11 ] =
	        -m->m[ 0 ] * m->m[ 5 ] * m->m[ 11 ] +
	        m->m[ 0 ] * m->m[ 7 ] * m->m[ 9 ] +
	        m->m[ 4 ] * m->m[ 1 ] * m->m[ 11 ] -
	        m->m[ 4 ] * m->m[ 3 ] * m->m[ 9 ] -
	        m->m[ 8 ] * m->m[ 1 ] * m->m[ 7 ] +
	        m->m[ 8 ] * m->m[ 3 ] * m->m[ 5 ];
	out.m[ 15 ] =
	        m->m[ 0 ] * m->m[ 5 ] * m->m[ 10 ] -
	        m->m[ 0 ] * m->m[ 6 ] * m->m[ 9 ] -
	        m->m[ 4 ] * m->m[ 1 ] * m->m[ 10 ] +
	        m->m[ 4 ] * m->m[ 2 ] * m->m[ 9 ] +
	        m->m[ 8 ] * m->m[ 1 ] * m->m[ 6 ] -
	        m->m[ 8 ] * m->m[ 2 ] * m->m[ 5 ];

	float d = m->m[ 0 ] * out.m[ 0 ] + m->m[ 1 ] * out.m[ 4 ] + m->m[ 2 ] * out.m[ 8 ] + m->m[ 3 ] * out.m[ 12 ];
	if ( d == 0 ) {
		return *m;
	}

	d = 1.0f / d;

	for ( unsigned int i = 0; i < 16; ++i ) {
		out.m[ i ] *= d;
	}

	return out;
}

PLMatrix4 PlLookAt( QmMathVector3f eye, QmMathVector3f center, QmMathVector3f up );

static inline PLMatrix4 PlFrustum( float left, float right, float bottom, float top, float nearf, float farf ) {
	float m0 = 2.0f * nearf;
	float m1 = right - left;
	float m2 = top - bottom;
	float m3 = farf - nearf;

	PLMatrix4 m;

	m.mm[ 0 ][ 0 ] = m0 / m1;
	m.mm[ 0 ][ 1 ] = 0.0f;
	m.mm[ 0 ][ 2 ] = ( right + left ) / m1;
	m.mm[ 0 ][ 3 ] = 0.0f;

	m.mm[ 1 ][ 0 ] = 0.0f;
	m.mm[ 1 ][ 1 ] = m0 / m2;
	m.mm[ 1 ][ 2 ] = ( top + bottom ) / m2;
	m.mm[ 1 ][ 3 ] = 0.0f;

	m.mm[ 2 ][ 0 ] = 0.0f;
	m.mm[ 2 ][ 1 ] = 0.0f;
	m.mm[ 2 ][ 2 ] = -( farf + nearf ) / m3;
	m.mm[ 2 ][ 3 ] = -1.0f;

	m.mm[ 3 ][ 0 ] = 0.0f;
	m.mm[ 3 ][ 1 ] = 0.0f;
	m.mm[ 3 ][ 2 ] = -1.0f;
	m.mm[ 3 ][ 3 ] = 0.0f;

	return m;
}

static inline PLMatrix4 PlOrtho( float left, float right, float bottom, float top, float nearf, float farf ) {
	float tx = -( right + left ) / ( right - left );
	float ty = -( top + bottom ) / ( top - bottom );
	float tz = -( farf + nearf ) / ( farf - nearf );

	PLMatrix4 frustumMatrix;

	frustumMatrix.m[ 0 ] = 2 / ( right - left );
	frustumMatrix.m[ 1 ] = 0.0f;
	frustumMatrix.m[ 2 ] = 0.0f;
	frustumMatrix.m[ 3 ] = 0.0f;

	frustumMatrix.m[ 4 ] = 0.0f;
	frustumMatrix.m[ 5 ] = 2 / ( top - bottom );
	frustumMatrix.m[ 6 ] = 0.0f;
	frustumMatrix.m[ 7 ] = 0.0f;

	frustumMatrix.m[ 8 ] = 0.0f;
	frustumMatrix.m[ 9 ] = 0.0f;
	frustumMatrix.m[ 10 ] = -2 / ( farf - nearf );
	frustumMatrix.m[ 11 ] = 0.0f;

	frustumMatrix.m[ 12 ] = tx;
	frustumMatrix.m[ 13 ] = ty;
	frustumMatrix.m[ 14 ] = tz;
	frustumMatrix.m[ 15 ] = 1.0f;

	return frustumMatrix;
}

static inline PLMatrix4 PlPerspective( float fov, float aspect, float nearf, float farf ) {
	float y_max = nearf * tanf( fov * QM_MATH_PI / 360.0f );
	float x_max = y_max * aspect;
	return PlFrustum( -x_max, x_max, -y_max, y_max, nearf, farf );
}

// http://www.terathon.com/code/oblique.php
/**
 * The following code modifies the projection matrix so that the near plane of the standard view
 * frustum is moved so that it coincides with a given arbitrary plane. The far plane is adjusted so
 * that the resulting view frustum has the best shape possible. This code assumes that the original
 * projection matrix is a perspective projection (standard or infinite). The clipPlane parameter
 * must be in camera-space coordinates, and its w-coordinate must be negative (corresponding to the
 * camera being on the negative side of the plane).
 */
static inline PLMatrix4 qm_math_matrix4_oblique( const PLMatrix4 *proj, const QmMathVector4f clip )
{
	//TODO: if this is useful anywhere else, move it
#define QM_MATH_SGN( X ) ( ( X ) > 0.0f ? 1.0f : ( ( ( X ) < 0.0f ) ? -1.0f : 0.0f ) )

	QmMathVector4f q;
	q.x = ( QM_MATH_SGN( clip.x ) + proj->m[ 8 ] ) / proj->m[ 0 ];
	q.y = ( QM_MATH_SGN( clip.y ) + proj->m[ 9 ] ) / proj->m[ 5 ];
	q.z = -1.0f;
	q.w = ( 1.0f + proj->m[ 10 ] ) / proj->m[ 14 ];

	QmMathVector4f c = qm_math_vector4f_scale_float( clip, 2.0f / qm_math_vector4f_dot_product( clip, q ) );

	PLMatrix4 out = *proj;
	out.m[ 2 ]    = c.x;
	out.m[ 6 ]    = c.y;
	out.m[ 10 ]   = c.z + 1.0f;
	out.m[ 14 ]   = c.w;

	return out;
}

/* Matrix Stack, sorta mirrors OpenGL behaviour
 * TODO: move this into plgraphics */

typedef enum PLMatrixMode {
	PL_MODELVIEW_MATRIX,
	PL_VIEW_MATRIX,
	PL_PROJECTION_MATRIX,
	PL_TEXTURE_MATRIX,

	PL_NUM_MATRIX_MODES
} PLMatrixMode;

void PlMatrixMode( PLMatrixMode mode );
PLMatrix4 *qm_math_matrix_stack_get_current();
PLMatrixMode PlGetMatrixMode( void );

PLMatrix4 *PlGetMatrix( PLMatrixMode mode );
void PlLoadMatrix( const PLMatrix4 *matrix );
void PlLoadIdentityMatrix( void );

void PlMultiMatrix( const PLMatrix4 *matrix );
void PlRotateMatrix( float angle, const QmMathVector3f *axis );
void PlRotateMatrix3f( float angle, float x, float y, float z );
void PlTranslateMatrix( QmMathVector3f vector );
void PlScaleMatrix( QmMathVector3f scale );
void PlInverseMatrix( void );

void PlPushMatrix( void );
void PlPopMatrix( void );

PL_EXTERN_C_END
