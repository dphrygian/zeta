#ifndef THREED_H
#define THREED_H

#define USE_LONG_INDICES 0

#define MAX_TEXTURE_STAGES	8	// 2013-09-30: Reduced from 16 to 8 because when do I ever need that many textures?
#define MAX_BONES			4	// Max weighted bones per vertex
#define MAX_BONE_MATRICES	32	// Max bones in skeletal mesh

#define CLEAR_NONE		0x0
#define CLEAR_COLOR		0x1
#define CLEAR_DEPTH		0x2
#define CLEAR_STENCIL	0x4

#if USE_LONG_INDICES
typedef unsigned int index_t;
#else
typedef unsigned short index_t;
#endif

// For weights or indices
struct SBoneData
{
	c_uint8 m_Data[ MAX_BONES ];
};

struct SRect
{
	SRect()
	:	m_Left( 0.0f )
	,	m_Top( 0.0f )
	,	m_Right( 0.0f )
	,	m_Bottom( 0.0f )
	{
	}

	SRect( const float Left, const float Top, const float Right, const float Bottom )
	:	m_Left( Left )
	,	m_Top( Top )
	,	m_Right( Right )
	,	m_Bottom( Bottom )
	{
	}

	bool operator==( const SRect& OtherRect ) const
	{
		return
			m_Left		== OtherRect.m_Left		&&
			m_Top		== OtherRect.m_Top		&&
			m_Right		== OtherRect.m_Right	&&
			m_Bottom	== OtherRect.m_Bottom;
	}

	bool operator!=( const SRect& OtherRect ) const
	{
		return
			m_Left		!= OtherRect.m_Left		||
			m_Top		!= OtherRect.m_Top		||
			m_Right		!= OtherRect.m_Right	||
			m_Bottom	!= OtherRect.m_Bottom;
	}

	float m_Left;
	float m_Top;
	float m_Right;
	float m_Bottom;
};

// Use this instead of creating a renderer directly
// to avoid linking a main project into D3D

#if BUILD_WINDOWS_NO_SDL
#include <Windows.h>
class IRenderer* CreateD3D9Renderer( HWND hWnd, class Display* const pDisplay );
#endif

class IRenderer* CreateGL2Renderer( class Window* const pWindow, class Display* const pDisplay );

#endif // THREED_H
