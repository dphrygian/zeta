#ifndef ANIMATION_H
#define ANIMATION_H

#include "array.h"
#include "hashedstring.h"
#include "vector.h"
#include "angles.h"

#define ANIM_FRAMERATE		30.0f
#define ANIM_NAME_LENGTH	32

class AnimEvent;
class SimpleString;

// This is the part of Animation that can be saved in resource files
struct SAnimData
{
	SAnimData()
	:	m_HashedName()
	,	m_StartFrame( 0 )
	,	m_Length( 0 )
	{
		m_Name[0] = '\0';
	}

	char				m_Name[ ANIM_NAME_LENGTH ];	// Only needed for matching entries in animations.config anymore
	HashedString		m_HashedName;				// Used to get animation by name
	c_uint16			m_StartFrame;				// In bone array's frame list
	c_uint16			m_Length;					// In frames
};

class Animation
{
public:
	Animation();
	~Animation();

	void				InitializeFromDefinition( const SimpleString& QualifiedAnimationName );

	uint				GetLengthFrames() const				{ return m_AnimData.m_Length; }
	float				GetLengthSeconds() const			{ return m_AnimData.m_Length / ANIM_FRAMERATE; }
	float				GetNonLoopingLengthSeconds() const	{ return ( m_AnimData.m_Length - 1 ) / ANIM_FRAMERATE; }
	float				GetPlayRate() const					{ return m_PlayRate; }

	void				GetVelocity( Vector& OutVelocity, Angles& OutRotationalVelocity ) const;
	Vector				GetDisplacement( float Time ) const;

	SAnimData			m_AnimData;

	Array<AnimEvent*>	m_AnimEvents;

	// Fixed play rate, for time scaling without editing clips
	float				m_PlayRate;

	// Cheap substitute for root motion
	Vector				m_Velocity;
	Angles				m_RotationalVelocity;
};

#endif // ANIMATION_H
