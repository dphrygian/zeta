#include "core.h"
#include "wbcomprosacampaignartifact.h"
#include "configmanager.h"
#include "wbeventmanager.h"
#include "mathcore.h"
#include "rosacampaign.h"

WBCompRosaCampaignArtifact::WBCompRosaCampaignArtifact()
:	m_Tag()
,	m_Capacity( 0 )
{
}

WBCompRosaCampaignArtifact::~WBCompRosaCampaignArtifact()
{
}

/*virtual*/ void WBCompRosaCampaignArtifact::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Tag );
	m_Tag							= ConfigManager::GetInheritedHash( sTag, HashedString::NullString, sDefinitionName );

	STATICHASH( Capacity );
	const float	DefaultCapacity		= ConfigManager::GetInheritedFloat( sCapacity, 1.0f, sDefinitionName );

	STATICHASH( CapacityLowThreat );
	const float	CapacityLowThreat	= ConfigManager::GetInheritedFloat( sCapacityLowThreat, DefaultCapacity, sDefinitionName );

	STATICHASH( CapacityHighThreat );
	const float	CapacityHighThreat	= ConfigManager::GetInheritedFloat( sCapacityHighThreat, DefaultCapacity, sDefinitionName );

	// OLDVAMP
	const float	ThreatAlpha			= 0.0f; //RosaCampaign::GetCampaign()->GetThreatAlpha();
	m_Capacity						= static_cast<uint>( Lerp( CapacityLowThreat, CapacityHighThreat, ThreatAlpha ) );
}

/*virtual*/ void WBCompRosaCampaignArtifact::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( ResolveArtifact );
	STATIC_HASHED_STRING( ReduceArtifact );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sResolveArtifact )
	{
		// Notify the campaign
		WB_MAKE_EVENT( OnArtifactResolved, NULL );
		WB_SET_AUTO( OnArtifactResolved, Hash, Tag, m_Tag );
		WB_DISPATCH_EVENT( GetEventManager(), OnArtifactResolved, NULL );
	}
	else if( EventName == sReduceArtifact )
	{
		// Notify the campaign
		WB_MAKE_EVENT( OnArtifactReduced, NULL );
		WB_SET_AUTO( OnArtifactReduced, Hash, Tag, m_Tag );
		WB_DISPATCH_EVENT( GetEventManager(), OnArtifactReduced, NULL );
	}
}

/*virtual*/ void WBCompRosaCampaignArtifact::AddContextToEvent( WBEvent& Event ) const
{
	Super::AddContextToEvent( Event );

	WB_SET_CONTEXT( Event, Int, ArtifactCapacity, m_Capacity );
}
