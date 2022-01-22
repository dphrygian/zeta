#ifndef GL2_H
#define GL2_H

#include "GL/glew.h"
#ifdef WIN32
#include "GL/wglew.h"
#endif

#if BUILD_DEV
#define GLERRORCHECK do { const GLenum Error = glGetError(); if( Error == GL_NO_ERROR ) { break; } else { PRINTF( "GL error: 0x%04X\n", Error ); WARNDESC( "GL check" ); } } while(1)
#else
#define GLERRORCHECK DoNothing
#endif

#if BUILD_DEBUG
#define GLPARANOIDERRORCHECK GLERRORCHECK
#else
#define GLPARANOIDERRORCHECK DoNothing
#endif

// Guard structs that save old values and restore when they fall out of scope.
#define GLGUARD_BINDTEXTURE		SGLGuard_BindTexture	AutoGuard_BindTexture
#define GLGUARD_BINDCUBEMAP		SGLGuard_BindCubemap	AutoGuard_BindCubemap
#define GLGUARD_ACTIVETEXTURE	SGLGuard_ActiveTexture	AutoGuard_ActiveTexture

struct SGLGuard_BindTexture
{
	SGLGuard_BindTexture()
	:	m_FormerValue( 0 )
	{
		glGetIntegerv( GL_TEXTURE_BINDING_2D, &m_FormerValue );
	}

	~SGLGuard_BindTexture()
	{
		glBindTexture( GL_TEXTURE_2D, m_FormerValue );
	}

	GLint m_FormerValue;
};

struct SGLGuard_BindCubemap
{
	SGLGuard_BindCubemap()
	:	m_FormerValue( 0 )
	{
		glGetIntegerv( GL_TEXTURE_BINDING_CUBE_MAP, &m_FormerValue );
	}

	~SGLGuard_BindCubemap()
	{
		glBindTexture( GL_TEXTURE_CUBE_MAP, m_FormerValue );
	}

	GLint m_FormerValue;
};

struct SGLGuard_ActiveTexture
{
	SGLGuard_ActiveTexture()
	:	m_FormerValue( 0 )
	{
		glGetIntegerv( GL_ACTIVE_TEXTURE, &m_FormerValue );
	}

	~SGLGuard_ActiveTexture()
	{
		glActiveTexture( m_FormerValue );
	}

	GLint m_FormerValue;
};

#endif // GL2_H
