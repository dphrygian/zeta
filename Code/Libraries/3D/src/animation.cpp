#include "core.h"
#include "animation.h"
#include "animevent.h"
#include "configmanager.h"
#include "animeventfactory.h"
#include "mathcore.h"

Animation::Animation()
:	m_AnimData()
,	m_AnimEvents()
,	m_PlayRate()
,	m_Velocity()
,	m_RotationalVelocity()
{
}

Animation::~Animation()
{
	for( uint i = 0; i < m_AnimEvents.Size(); ++i )
	{
		SafeDelete( m_AnimEvents[i] );
	}
	m_AnimEvents.Clear();
}

// Initialize anim events and other non-baked properties from config.
// (I don't want to make a habit of this, but since AnimEvents need
// to be created at runtime, might as well do the rest here and keep
// it consolidated.)
void Animation::InitializeFromDefinition( const SimpleString& QualifiedAnimationName )
{
	MAKEHASH( QualifiedAnimationName );

	m_AnimEvents.Clear();

	STATICHASH( NumAnimEvents );
	int NumAnimEvents = ConfigManager::GetInt( sNumAnimEvents, 0, sQualifiedAnimationName );
	for( int AnimEventIndex = 0; AnimEventIndex < NumAnimEvents; ++AnimEventIndex )
	{
		AnimEvent* pAnimEvent = NULL;

		const SimpleString	AnimEventDef	= ConfigManager::GetSequenceString(	"AnimEvent%dDef",	AnimEventIndex,	"",	sQualifiedAnimationName );
		const int			AnimEventFrame	= ConfigManager::GetSequenceInt(	"AnimEvent%dFrame",	AnimEventIndex,	0,	sQualifiedAnimationName );
		pAnimEvent							= AnimEventFactory::GetInstance()->Create( AnimEventDef );
		pAnimEvent->m_Time					= static_cast<float>( AnimEventFrame ) / ANIM_FRAMERATE;

		if( pAnimEvent->m_Time < 0.0f )
		{
			pAnimEvent->m_Time				= GetNonLoopingLengthSeconds() + pAnimEvent->m_Time;
		}

		m_AnimEvents.PushBack( pAnimEvent );
	}

	STATICHASH( PlayRate );
	m_PlayRate = ConfigManager::GetFloat( sPlayRate, 1.0f, sQualifiedAnimationName );

	STATICHASH( VelocityX );
	m_Velocity.x = ConfigManager::GetFloat( sVelocityX, 0.0f, sQualifiedAnimationName );

	STATICHASH( VelocityY );
	m_Velocity.y = ConfigManager::GetFloat( sVelocityY, 0.0f, sQualifiedAnimationName );

	STATICHASH( VelocityYaw );
	m_RotationalVelocity.Yaw = DEGREES_TO_RADIANS( ConfigManager::GetFloat( sVelocityYaw, 0.0f, sQualifiedAnimationName ) );
}

void Animation::GetVelocity( Vector& OutVelocity, Angles& OutRotationalVelocity ) const
{
	OutVelocity				= m_Velocity;
	OutRotationalVelocity	= m_RotationalVelocity;
}

// Calculate displacement at time Time given 2D velocity and yaw
// ROSANOTE: This was used in Couriers for pre-dodge tests to ensure
// clearance. I'm not currently using it, but leaving it around
// because that could be a good thing to bring back.
Vector Animation::GetDisplacement( float Time ) const
{
	if( Abs( m_RotationalVelocity.Yaw ) > EPSILON )
	{
		// Solve the radius of the curved motion from the known
		// arc length and angular delta (separately per axis).
		const Vector	SeparableArcLength	= m_Velocity * Time;
		const float		Theta				= m_RotationalVelocity.Yaw * Time;
		const float		SinTheta			= Sin( Theta );
		const float		CosTheta			= Cos( Theta );
		const Vector	SeparableRadius		= SeparableArcLength / Theta;

		Vector RetVal;
		RetVal.x += SeparableRadius.x * SinTheta;
		RetVal.y += SeparableRadius.x * ( 1.0f - CosTheta );
		RetVal.x += SeparableRadius.y * ( CosTheta - 1.0f );
		RetVal.y += SeparableRadius.y * SinTheta;
		return RetVal;
	}
	else
	{
		return m_Velocity * Time;
	}
}
