// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Shaders
// Author:  Mark E. Sowden

#include <plgraphics/qm_gfx_driver.h>

#include "plg_private.h"

/////////////////////////////////////////////////////////////////////////////////////
// Shader Stage
/////////////////////////////////////////////////////////////////////////////////////

static void shader_stage_destroy( void *ptr )
{
	CallGfxFunction( DestroyShaderStage, ptr );
}

QmGfxShaderStage *qm_gfx_shader_stage_create( QmGfxShaderStageType type )
{
	QmGfxShaderStage *stage = QM_OS_MEMORY_NEW_D( QmGfxShaderStage, shader_stage_destroy );
	stage->type             = type;

	CallGfxFunction( CreateShaderStage, stage );

	return stage;
}

bool qm_gfx_shader_stage_compile( QmGfxShaderStage *self, const char *buf, size_t length, const char *localDirectory )
{
	CallReturningGfxFunction( CompileShaderStage, false, self, buf, length, localDirectory );
}

void qm_gfx_shader_stage_set_definitions( QmGfxShaderStage *self, const char definitions[][ PLG_MAX_DEFINITION_LENGTH ], unsigned int numDefinitions )
{
	if ( numDefinitions > PLG_MAX_DEFINITIONS )
	{
		numDefinitions = PLG_MAX_DEFINITIONS;
	}

	self->numDefinitions = numDefinitions;
	memcpy( self->definitions, definitions, PLG_MAX_DEFINITION_LENGTH * numDefinitions );
}

/////////////////////////////////////////////////////////////////////////////////////
// Shader Program
/////////////////////////////////////////////////////////////////////////////////////

static void shader_program_destroy( void *ptr )
{
	QmGfxShaderProgram *self = ptr;

	for ( unsigned int i = 0; i < QM_GFX_MAX_SHADER_STAGE_TYPES; ++i )
	{
		if ( self->stages[ i ] != NULL )
		{
			CallGfxFunction( DetachShaderStage, self, self->stages[ i ] );
			qm_os_memory_free( self->stages[ i ] );
		}
	}

	CallGfxFunction( DestroyShaderProgram, self );

	qm_os_memory_free( self->uniforms );
}

QmGfxShaderProgram *qm_gfx_shader_program_create()
{
	QmGfxShaderProgram *program = QM_OS_MEMORY_NEW_D( QmGfxShaderProgram, shader_program_destroy );
	CallGfxFunction( CreateShaderProgram, program );

	return program;
}

void qm_gfx_shader_program_attach_stage( QmGfxShaderProgram *self, QmGfxShaderStage *stage )
{
	self->stages[ self->num_stages++ ] = stage;
	CallGfxFunction( AttachShaderStage, self, stage );
}

QmGfxShaderProgram *PlgGetCurrentShaderProgram( void )
{
	return gfx_state.current_program;
}

bool PlgIsShaderProgramEnabled( QmGfxShaderProgram *program )
{
	return gfx_state.current_program == program;
}

bool qm_gfx_shader_program_link( QmGfxShaderProgram *self )
{
	CallGfxFunction( LinkShaderProgram, self );

	return self->is_linked;
}

void PlgSetShaderProgram( QmGfxShaderProgram *program )
{
	if ( program == gfx_state.current_program )
	{
		return;
	}

	CallGfxFunction( SetShaderProgram, program );

	gfx_state.current_program = program;
}

static QmGfxShaderProgram *get_shader_program( QmGfxShaderProgram *program )
{
	if ( program != NULL )
	{
		return program;
	}

	if ( gfx_state.current_program != NULL )
	{
		return gfx_state.current_program;
	}

	GfxLog( "NULL shader specified for uniform write, and no active shader program bound!\n" );
	return nullptr;
}

/*****************************************************/
/** shader uniform **/

int qm_gfx_shader_program_get_uniform_slot( QmGfxShaderProgram *self, const char *name )
{
	QmGfxShaderProgram *prg = get_shader_program( self );
	if ( prg == NULL )
	{
		return -1;
	}

	for ( unsigned int i = 0; i < prg->numUniforms; ++i )
	{
		if ( *prg->uniforms[ i ].name == '\0' )
		{
			continue;
		}

		if ( pl_strcasecmp( prg->uniforms[ i ].name, name ) == 0 )
		{
			return i;
		}
	}

	return -1;
}

QmGfxShaderUniformType qm_gfx_shader_program_get_uniform_type( const QmGfxShaderProgram *self, int slot )
{
	if ( slot < 0 || ( unsigned int ) slot >= self->numUniforms )
	{
		return QM_GFX_SHADER_UNIFORM_TYPE_INVALID;
	}

	return self->uniforms[ slot ].type;
}

unsigned int qm_gfx_shader_program_get_num_uniform_elements( const QmGfxShaderProgram *self, int slot )
{
	if ( slot < 0 || ( unsigned int ) slot >= self->numUniforms )
	{
		return 0;
	}

	return self->uniforms[ slot ].numElements;
}

static int shader_program_validate_uniform_slot( const QmGfxShaderProgram *program, const int slot )
{
	if ( slot == -1 )
	{
		GfxLog( "Invalid shader uniform slot!\n" );
		return -1;
	}

	if ( ( unsigned int ) slot >= program->numUniforms )
	{
		GfxLog( "Potential overflow for uniform slot! (%d / %d)\n", slot, program->numUniforms );
		return -1;
	}

	if ( program->uniforms[ slot ].type == QM_GFX_SHADER_UNIFORM_TYPE_INVALID )
	{
		GfxLog( "Unknown uniform type for slot! (%d)\n", slot );
		return -1;
	}

	return slot;
}

void qm_gfx_shader_program_set_uniform_default( QmGfxShaderProgram *self, const int slot, const void *defaultValue )
{
	switch ( self->uniforms[ slot ].type )
	{
		case QM_GFX_SHADER_UNIFORM_TYPE_FLOAT:
			self->uniforms[ slot ].defaultFloat = *( float * ) defaultValue;
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER2D:
		case QM_GFX_SHADER_UNIFORM_TYPE_INT:
			self->uniforms[ slot ].defaultInt = *( int * ) defaultValue;
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_UINT:
			self->uniforms[ slot ].defaultUInt = *( unsigned int * ) defaultValue;
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_BOOL:
			self->uniforms[ slot ].defaultBool = *( bool * ) defaultValue;
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_DOUBLE:
			self->uniforms[ slot ].defaultDouble = *( double * ) defaultValue;
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_VEC2:
			self->uniforms[ slot ].defaultVec2 = *( QmMathVector2f * ) defaultValue;
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_VEC3:
			self->uniforms[ slot ].defaultVec3 = *( QmMathVector3f * ) defaultValue;
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_VEC4:
			self->uniforms[ slot ].defaultVec4 = *( QmMathVector4f * ) defaultValue;
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_MAT3:
			self->uniforms[ slot ].defaultMat3 = *( PLMatrix3 * ) defaultValue;
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_MAT4:
			self->uniforms[ slot ].defaultMat4 = *( PLMatrix4 * ) defaultValue;
			break;
		default:
			break;
	}
}

void qm_gfx_shader_program_set_uniform_to_default( QmGfxShaderProgram *self, int slot )
{
	switch ( self->uniforms[ slot ].type )
	{
		case QM_GFX_SHADER_UNIFORM_TYPE_FLOAT:
			qm_gfx_shader_program_set_uniform( self, slot, &self->uniforms[ slot ].defaultFloat, false );
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER2D:
		case QM_GFX_SHADER_UNIFORM_TYPE_INT:
			qm_gfx_shader_program_set_uniform( self, slot, &self->uniforms[ slot ].defaultInt, false );
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_UINT:
			qm_gfx_shader_program_set_uniform( self, slot, &self->uniforms[ slot ].defaultUInt, false );
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_BOOL:
			qm_gfx_shader_program_set_uniform( self, slot, &self->uniforms[ slot ].defaultBool, false );
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_DOUBLE:
			qm_gfx_shader_program_set_uniform( self, slot, &self->uniforms[ slot ].defaultDouble, false );
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_VEC2:
			qm_gfx_shader_program_set_uniform( self, slot, &self->uniforms[ slot ].defaultVec2, false );
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_VEC3:
			qm_gfx_shader_program_set_uniform( self, slot, &self->uniforms[ slot ].defaultVec3, false );
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_VEC4:
			qm_gfx_shader_program_set_uniform( self, slot, &self->uniforms[ slot ].defaultVec4, false );
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_MAT3:
			qm_gfx_shader_program_set_uniform( self, slot, &self->uniforms[ slot ].defaultMat3, false );
			break;
		case QM_GFX_SHADER_UNIFORM_TYPE_MAT4:
			qm_gfx_shader_program_set_uniform( self, slot, &self->uniforms[ slot ].defaultMat4, false );
			break;
		default:
			break;
	}
}

void qm_gfx_shader_program_set_uniform( QmGfxShaderProgram *self, int slot, const void *value, bool transpose )
{
	if ( shader_program_validate_uniform_slot( self, slot ) == -1 )
	{
		return;
	}

	/* this should be done by the GL layer!! */
	QmGfxShaderProgram *oldProgram = PlgGetCurrentShaderProgram();
	PlgSetShaderProgram( self );

	CallGfxFunction( SetShaderUniformValue, self, slot, value, transpose );

	/* this should be done by the GL layer!! */
	PlgSetShaderProgram( oldProgram );
}
