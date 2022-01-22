#include "core.h"
#include "wbcomprosaparticles.h"
#include "rosaparticles.h"
#include "configmanager.h"
#include "wbcomprosatransform.h"
#include "wbcomprosamesh.h"
#include "wbcomprosacamera.h"
#include "wbeventmanager.h"
#include "idatastream.h"
#include "mathcore.h"
#include "rosagame.h"
#include "rosaworld.h"
#include "rosamesh.h"

WBCompRosaParticles::WBCompRosaParticles()
:	m_ParticleSystems()
,	m_Hidden( false )
,	m_CullDistanceSq( 0.0f )
{
	STATIC_HASHED_STRING( OnWorldLoaded );
	GetEventManager()->AddObserver( sOnWorldLoaded, this );
}

WBCompRosaParticles::~WBCompRosaParticles()
{
	FOR_EACH_ARRAY( PSIter, m_ParticleSystems, SParticleSystem )
	{
		SParticleSystem& PS = PSIter.GetValue();
		SafeDelete( PS.m_System );
	}

	WBEventManager* const pEventManager = GetEventManager();
	if( pEventManager )
	{
		STATIC_HASHED_STRING( OnWorldLoaded );
		pEventManager->RemoveObserver( sOnWorldLoaded, this );
	}
}

/*virtual*/ void WBCompRosaParticles::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( RosaParticles );

	const bool			Serialize			= false;
	const bool			Attached			= true;
	const int			AttachBoneIndex		= INVALID_INDEX;
	const Vector* const	pLocation			= NULL;
	const Vector* const	pBeamEndLocation	= NULL;
	const Angles* const	pOrientation		= NULL;

	STATICHASH( Particles );
	const SimpleString ParticlesDef = ConfigManager::GetInheritedString( sParticles, "", sDefinitionName );
	if( ParticlesDef != "" )
	{
		AddParticleSystem( ParticlesDef, Serialize, Attached, AttachBoneIndex, pLocation, pBeamEndLocation, pOrientation );
	}

	STATICHASH( NumParticleSystems );
	const uint NumParticleSystems = ConfigManager::GetInheritedInt( sNumParticleSystems, 0, sDefinitionName );
	for( uint ParticleSystemIndex = 0; ParticleSystemIndex < NumParticleSystems; ++ParticleSystemIndex )
	{
		const SimpleString ParticleSystemDef = ConfigManager::GetInheritedSequenceString( "ParticleSystem%d", ParticleSystemIndex, "", sDefinitionName );
		AddParticleSystem( ParticleSystemDef, Serialize, Attached, AttachBoneIndex, pLocation, pBeamEndLocation, pOrientation );
	}

	STATICHASH( CullDistance );
	const float DefaultCullDistance = ConfigManager::GetFloat( sCullDistance, 0.0f, sRosaParticles );
	m_CullDistanceSq = Square( ConfigManager::GetInheritedFloat( sCullDistance, DefaultCullDistance, sDefinitionName ) );
}

bool WBCompRosaParticles::HasAttachBone() const
{
	FOR_EACH_ARRAY( PSIter, m_ParticleSystems, SParticleSystem )
	{
		const SParticleSystem& PS = PSIter.GetValue();
		if( PS.m_AttachBoneIndex != INVALID_INDEX )
		{
			return true;
		}
	}

	return false;
}

void WBCompRosaParticles::AddParticleSystem( const SimpleString& DefinitionName, const bool Serialize, const bool Attached, const int AttachBoneIndex, const Vector* const pLocation, const Vector* const pBeamEndLocation, const Angles* const pOrientation )
{
	WBCompRosaTransform* const pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	const Vector	Location		= pLocation			? ( *pLocation )		: pTransform->GetLocation();
	const bool		IsBeam			= pBeamEndLocation != NULL;
	const Vector	BeamEndLocation	= pBeamEndLocation	? ( *pBeamEndLocation )	: Location;
	const Angles	Orientation		= pOrientation		? ( *pOrientation )		: pTransform->GetOrientation();

	AddParticleSystem( DefinitionName, Serialize, Attached, AttachBoneIndex, Location, IsBeam, BeamEndLocation, Orientation );
}

void WBCompRosaParticles::AddParticleSystem( const SimpleString& DefinitionName, const bool Serialize, const bool Attached, const int AttachBoneIndex, const Vector& Location, const bool IsBeam, const Vector& BeamEndLocation, const Angles& Orientation )
{
	if( DefinitionName == "" )
	{
		DEVWARNDESC( "WBCompRosaParticles::AddParticleSystem: Empty DefinitionName for particle system!" );
		return;
	}

	WBCompRosaMesh* const	pMesh			= WB_GETCOMP( GetEntity(), RosaMesh );
	const Vector			ActualLocation	= ( AttachBoneIndex == INVALID_INDEX ) ? Location : pMesh->GetBoneLocation( AttachBoneIndex );

	SParticleSystem& NewPS		= m_ParticleSystems.PushBack();

	NewPS.m_Attached			= Attached || ( AttachBoneIndex != INVALID_INDEX );
	NewPS.m_AttachBoneIndex		= AttachBoneIndex;
	NewPS.m_Serialize			= Serialize;
	NewPS.m_DefinitionName		= DefinitionName;
	NewPS.m_Location			= ActualLocation;
	NewPS.m_IsBeam				= IsBeam;
	NewPS.m_BeamEndLocation		= BeamEndLocation;
	NewPS.m_Orientation			= Orientation;
	NewPS.m_DefinitionNameHash	= DefinitionName;

	NewPS.m_System				= new RosaParticles;
	NewPS.m_System->InitializeFromDefinition(	DefinitionName );
	NewPS.m_System->SetIsBeam(					IsBeam );
	NewPS.m_System->SetLocation(				ActualLocation );
	if( IsBeam )
	{
		NewPS.m_System->SetBeamEndLocation(		BeamEndLocation );
	}
	NewPS.m_System->SetOrientation(				Orientation );
}

void WBCompRosaParticles::StopParticleSystem( const HashedString& DefinitionNameHash )
{
	FOR_EACH_ARRAY_REVERSE( PSIter, m_ParticleSystems, SParticleSystem )
	{
		SParticleSystem& PS = PSIter.GetValue();
		if( PS.m_DefinitionNameHash == DefinitionNameHash )
		{
			SafeDelete( PS.m_System );
			m_ParticleSystems.FastRemove( PSIter );
		}
	}
}

void WBCompRosaParticles::StopParticleSystems()
{
	FOR_EACH_ARRAY_REVERSE( PSIter, m_ParticleSystems, SParticleSystem )
	{
		SParticleSystem& PS = PSIter.GetValue();
		SafeDelete( PS.m_System );
	}
	m_ParticleSystems.Clear();
}

void WBCompRosaParticles::ExpireParticleSystem( const HashedString& DefinitionNameHash )
{
	FOR_EACH_ARRAY_REVERSE( PSIter, m_ParticleSystems, SParticleSystem )
	{
		SParticleSystem& PS = PSIter.GetValue();
		if( PS.m_DefinitionNameHash == DefinitionNameHash )
		{
			if( PS.m_System->IsExpired() )
			{
				continue;
			}

			PS.m_System->Expire();
		}
	}
}

void WBCompRosaParticles::ExpireParticleSystems()
{
	FOR_EACH_ARRAY( PSIter, m_ParticleSystems, SParticleSystem )
	{
		SParticleSystem& PS = PSIter.GetValue();
		DEVASSERT( PS.m_System );

		if( PS.m_System->IsExpired() )
		{
			continue;
		}

		PS.m_System->Expire();
	}
}

/*virtual*/ void WBCompRosaParticles::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	FOR_EACH_ARRAY_REVERSE( PSIter, m_ParticleSystems, SParticleSystem )
	{
		SParticleSystem& PS = PSIter.GetValue();
		DEVASSERT( PS.m_System );

		if( PS.m_System->IsFinished() )
		{
			SafeDelete( PS.m_System );
			m_ParticleSystems.FastRemove( PSIter );
		}
		else
		{
			PS.m_System->Tick( DeltaTime );
		}
	}
}

bool WBCompRosaParticles::IsWithinCullDistance() const
{
	if( m_CullDistanceSq == 0.0f )
	{
		return true;
	}

	const Vector	ViewLocation	= RosaGame::GetPlayerViewLocation();

	// If any of the systems are within the cull distance, do draw
	FOR_EACH_ARRAY( PSIter, m_ParticleSystems, SParticleSystem )
	{
		const SParticleSystem& PS = PSIter.GetValue();
		DEBUGASSERT( PS.m_System );

		const Vector	ViewOffset	= PS.m_System->GetLocation() - ViewLocation;
		const float		DistanceSq	= ViewOffset.LengthSquared();

		if( DistanceSq < m_CullDistanceSq )
		{
			return true;
		}
	}

	return false;
}

bool WBCompRosaParticles::IntersectsAnyVisibleSector() const
{
	PROFILE_FUNCTION;

	RosaWorld* const	pWorld				= GetWorld();
	const uint			NumVisibleSectors	= pWorld->GetNumVisibleSectors();

	// If any of the systems are within a visible sector, do draw
	for( uint Index = 0; Index < NumVisibleSectors; ++Index )
	{
		const AABB& SectorRenderBound = pWorld->GetVisibleSectorRenderBound( Index );
		FOR_EACH_ARRAY( PSIter, m_ParticleSystems, SParticleSystem )
		{
			const SParticleSystem& PS = PSIter.GetValue();
			DEBUGASSERT( PS.m_System );
			const RosaMesh* const pMesh = PS.m_System->GetDynamicMesh();
			if( pMesh && pMesh->m_AABB.Intersects( SectorRenderBound ) )
			{
				return true;
			}
		}
	}

	return false;
}

/*virtual*/ void WBCompRosaParticles::Render()
{
	XTRACE_FUNCTION;

	if( m_ParticleSystems.Empty() )
	{
		return;
	}

	if( m_Hidden )
	{
		return;
	}

	if( !IsWithinCullDistance() )
	{
		return;
	}

	if( !IntersectsAnyVisibleSector() )
	{
		return;
	}

	FOR_EACH_ARRAY( PSIter, m_ParticleSystems, SParticleSystem )
	{
		const SParticleSystem& PS = PSIter.GetValue();
		DEVASSERT( PS.m_System );
		PS.m_System->Render();
	}
}

/*virtual*/ void WBCompRosaParticles::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnMoved );
	STATIC_HASHED_STRING( OnTeleported );
	STATIC_HASHED_STRING( OnTurned );
	STATIC_HASHED_STRING( OnWorldLoaded );
	STATIC_HASHED_STRING( OnAnimationTick );
	STATIC_HASHED_STRING( PlayParticleSystem );
	STATIC_HASHED_STRING( StopParticleSystem );
	STATIC_HASHED_STRING( StopParticleSystems );
	STATIC_HASHED_STRING( ExpireParticleSystem );
	STATIC_HASHED_STRING( ExpireParticleSystems );
	STATIC_HASHED_STRING( Hide );
	STATIC_HASHED_STRING( Show );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnMoved || EventName == sOnTeleported || EventName == sOnWorldLoaded || EventName == sOnTurned || EventName == sOnAnimationTick )
	{
		WBEntity* const				pEntity		= GetEntity();
		DEVASSERT( pEntity );

		WBCompRosaTransform* const	pTransform	= pEntity->GetTransformComponent<WBCompRosaTransform>();
		DEVASSERT( pTransform );

		WBCompRosaMesh* const		pMesh		= WB_GETCOMP( pEntity, RosaMesh );

		const Angles Orientation	= pTransform->GetOrientation();

		FOR_EACH_ARRAY( PSIter, m_ParticleSystems, SParticleSystem )
		{
			const SParticleSystem& PS = PSIter.GetValue();
			if( PS.m_Attached )
			{
				DEVASSERT( PS.m_System );

				const Vector Location		= ( PS.m_AttachBoneIndex == INVALID_INDEX ) ? pTransform->GetLocation() : pMesh->GetBoneLocation( PS.m_AttachBoneIndex );

				const WBCompRosaCamera* const pCamera = WB_GETCOMP( GetEntity(), RosaCamera );
				if( pCamera )
				{
					const Vector NewLocation	= pCamera->GetModifiedTranslation( WBCompRosaCamera::EVM_All, Location );
					const Angles NewOrientation	= pCamera->GetModifiedOrientation( WBCompRosaCamera::EVM_All, Orientation );

					PS.m_System->SetLocation(				NewLocation );
					if( PS.m_System->IsBeam() )
					{
						PS.m_System->SetBeamEndLocation(	PS.m_System->GetBeamEndLocation() );
					}
					PS.m_System->SetOrientation(			NewOrientation );
				}
				else
				{
					PS.m_System->SetLocation(				Location );
					if( PS.m_System->IsBeam() )
					{
						PS.m_System->SetBeamEndLocation(	PS.m_System->GetBeamEndLocation() );
					}
					PS.m_System->SetOrientation(			Orientation );
				}
			}
		}
	}
	else if( EventName == sPlayParticleSystem )
	{
		STATIC_HASHED_STRING( ParticleSystem );
		const SimpleString ParticleSystemDef = Event.GetString( sParticleSystem );

		STATIC_HASHED_STRING( Attached );
		const bool Attached = Event.GetBool( sAttached );

		STATIC_HASHED_STRING( AttachBone );
		const HashedString AttachBone = Event.GetHash( sAttachBone );
		const WBCompRosaMesh* const pMesh = WB_GETCOMP( GetEntity(), RosaMesh );
		const int AttachBoneIndex = ( pMesh && AttachBone ) ? pMesh->GetBoneIndex( AttachBone ) : INVALID_INDEX;

		STATIC_HASHED_STRING( Location );
		const Vector Location = Event.GetVector( sLocation );

		STATIC_HASHED_STRING( BeamEndLocation );
		const Vector BeamEndLocation = Event.GetVector( sBeamEndLocation );

		STATIC_HASHED_STRING( Orientation );
		const Angles Orientation = Event.GetAngles( sOrientation );

		const bool		Serialize			= true;
		const Vector*	pLocation			= Event.HasParameter( sLocation )			? ( &Location )			: NULL;
		const Vector*	pBeamEndLocation	= Event.HasParameter( sBeamEndLocation )	? ( &BeamEndLocation )	: NULL;
		const Angles*	pOrientation		= Event.HasParameter( sOrientation )		? ( &Orientation )		: NULL;

		AddParticleSystem( ParticleSystemDef, Serialize, Attached, AttachBoneIndex, pLocation, pBeamEndLocation, pOrientation );
	}
	else if( EventName == sStopParticleSystem )
	{
		STATIC_HASHED_STRING( ParticleSystem );
		const HashedString ParticleSystemDef = Event.GetHash( sParticleSystem );

		StopParticleSystem( ParticleSystemDef );
	}
	else if( EventName == sStopParticleSystems )
	{
		StopParticleSystems();
	}
	else if( EventName == sExpireParticleSystem )
	{
		STATIC_HASHED_STRING( ParticleSystem );
		const HashedString ParticleSystemDef = Event.GetHash( sParticleSystem );

		ExpireParticleSystem( ParticleSystemDef );
	}
	else if( EventName == sExpireParticleSystems )
	{
		ExpireParticleSystems();
	}
	else if( EventName == sHide )
	{
		m_Hidden = true;
	}
	else if( EventName == sShow )
	{
		m_Hidden = false;
	}
}

#define VERSION_EMPTY			0
#define VERSION_HIDDEN			1
#define VERSION_SYSTEMS			2
#define VERSION_BEAMENDLOCATION	3
#define VERSION_ISBEAM			4
#define VERSION_ATTACHBONEINDEX	5
#define VERSION_CURRENT			5

uint WBCompRosaParticles::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version

	Size += 1;	// m_Hidden

	Size += 4;	// m_ParticleSystems.Size()
	FOR_EACH_ARRAY( ParticleSystemIter, m_ParticleSystems, SParticleSystem )
	{
		const SParticleSystem& ParticleSystem = ParticleSystemIter.GetValue();
		Size += 1;	// m_Serialize
		if( ParticleSystem.m_Serialize )
		{
			Size += 1;					// m_Attached
			Size += 4;					// m_AttachBoneIndex
			Size += sizeof( Vector );	// m_Location
			Size += 1;					// m_IsBeam
			Size += sizeof( Vector );	// m_BeamEndLocation
			Size += sizeof( Angles );	// m_Orientation
			Size += IDataStream::SizeForWriteString( ParticleSystem.m_DefinitionName );
		}
	}

	return Size;
}

void WBCompRosaParticles::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteBool( m_Hidden );

	Stream.WriteUInt32( m_ParticleSystems.Size() );
	FOR_EACH_ARRAY( ParticleSystemIter, m_ParticleSystems, SParticleSystem )
	{
		const SParticleSystem& ParticleSystem = ParticleSystemIter.GetValue();
		const bool Serialized  = ParticleSystem.m_Serialize && !ParticleSystem.m_System->IsExpired();
		Stream.WriteBool( Serialized );
		if( Serialized )
		{
			Stream.WriteBool(		ParticleSystem.m_Attached );
			Stream.WriteInt32(		ParticleSystem.m_AttachBoneIndex );
			Stream.Write<Vector>(	ParticleSystem.m_Location );
			Stream.WriteBool(		ParticleSystem.m_IsBeam );
			Stream.Write<Vector>(	ParticleSystem.m_BeamEndLocation );
			Stream.Write<Angles>(	ParticleSystem.m_Orientation );
			Stream.WriteString(		ParticleSystem.m_DefinitionName );
		}
	}
}

void WBCompRosaParticles::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_HIDDEN )
	{
		m_Hidden = Stream.ReadBool();
	}

	if( Version >= VERSION_SYSTEMS )
	{
		const uint NumParticleSystems = Stream.ReadUInt32();
		for( uint ParticleSystemIndex = 0; ParticleSystemIndex < NumParticleSystems; ++ParticleSystemIndex )
		{
			if( Stream.ReadBool() )	// Serialized
			{
				const bool			Attached		= Stream.ReadBool();
				const int			AttachBoneIndex	= ( Version >= VERSION_ATTACHBONEINDEX )	? Stream.ReadInt32()	: INVALID_INDEX;
				const Vector		Location		= Stream.Read<Vector>();
				const bool			IsBeam			= ( Version >= VERSION_ISBEAM )				? Stream.ReadBool()		: false;
				const Vector		BeamEndLocation	= ( Version >= VERSION_BEAMENDLOCATION )	? Stream.Read<Vector>()	: Vector();
				const Angles		Orientation		= Stream.Read<Angles>();
				const SimpleString	DefinitionName	= Stream.ReadString();
				const bool			Serialize		= true;

				AddParticleSystem( DefinitionName, Serialize, Attached, AttachBoneIndex, Location, IsBeam, BeamEndLocation, Orientation );
			}
		}
	}
}
