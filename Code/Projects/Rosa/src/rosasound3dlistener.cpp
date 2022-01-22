#include "core.h"
#include "rosasound3dlistener.h"
#include "rosaworld.h"
#include "collisioninfo.h"
#include "mathcore.h"
#include "isoundinstance.h"
#include "configmanager.h"

RosaSound3DListener::RosaSound3DListener()
:	m_World( NULL )
,	m_VerticalScalar( 0.0f )
{
}

RosaSound3DListener::~RosaSound3DListener()
{
}

void RosaSound3DListener::Initialize()
{
	STATICHASH( AudioSystem );

	STATICHASH( VerticalScalar );
	m_VerticalScalar = ConfigManager::GetFloat( sVerticalScalar, 1.0f, sAudioSystem );
}

/*virtual*/ void RosaSound3DListener::ModifyAttenuation( ISoundInstance* const pSoundInstance, float& Attenuation ) const
{
	PROFILE_FUNCTION;

	if( !m_World )
	{
		return;
	}

	if( !pSoundInstance->ShouldCalcOcclusion() )
	{
		return;
	}

	CollisionInfo FromListenerInfo;
	FromListenerInfo.m_In_CollideWorld		= true;
	FromListenerInfo.m_In_CollideEntities	= true;
	FromListenerInfo.m_In_UserFlags			= EECF_Audio;

	CollisionInfo FromSoundInfo;
	FromSoundInfo.m_In_CollideWorld			= true;
	FromSoundInfo.m_In_CollideEntities		= true;
	FromSoundInfo.m_In_UserFlags			= EECF_Audio;

	const Vector	SoundLocation	= pSoundInstance->GetLocation();
	const bool		Occluded		= m_World->LineCheck( m_Location, SoundLocation, FromListenerInfo ) &&
									  m_World->LineCheck( SoundLocation, m_Location, FromSoundInfo );

	if( Occluded )
	{
		// Use the ratio between the distances to sound source and to occlusion as a factor in attenuation.
		Vector				ToOcclusionNear				= FromListenerInfo.m_Out_Intersection - m_Location;
		ToOcclusionNear.z								*= m_VerticalScalar;

		Vector				ToOcclusionFar				= FromSoundInfo.m_Out_Intersection - SoundLocation;
		ToOcclusionFar.z								*= m_VerticalScalar;

		Vector				ToSound						= SoundLocation - m_Location;
		ToSound.z										*= m_VerticalScalar;

		const float			OcclusionDepthScalar		= pSoundInstance->GetOcclusionDepthScalar();
		const float			OccludedFalloffRadius		= pSoundInstance->GetOccludedFalloffRadius();

		const float			DistanceToOcclusionNear		= ToOcclusionNear.Length();
		const float			DistanceToOcclusionFar		= ToOcclusionFar.Length();
		const float			DistanceToOcclusionTotal	= DistanceToOcclusionNear + DistanceToOcclusionFar;
		const float			DistanceToSoundActual		= ToSound.Length();
		const float			OcclusionDepthActual		= DistanceToSoundActual - DistanceToOcclusionTotal;
		const float			OcclusionDepthScaled		= OcclusionDepthScalar * OcclusionDepthActual;
		const float			DistanceToSoundScaled		= DistanceToOcclusionTotal + OcclusionDepthScaled;
		const float			DistanceRatio				= DistanceToOcclusionTotal / DistanceToSoundScaled;
		DEBUGASSERT( DistanceRatio <= 1.0f );

		// And attenuate occluded sounds more if the *near distance* is larger;
		// i.e., if the occluding surface is far from the player. If the occluder
		// is near the player and the sound is distant, this sounds wrong.
		const float			OcclusionAttenuation		= Attenuate( DistanceToOcclusionNear, OccludedFalloffRadius );

		Attenuation *= DistanceRatio * OcclusionAttenuation;
	}
}
