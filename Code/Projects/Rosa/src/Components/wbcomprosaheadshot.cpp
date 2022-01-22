#include "core.h"
#include "wbcomprosaheadshot.h"
#include "configmanager.h"

WBCompRosaHeadshot::WBCompRosaHeadshot()
:	m_Headshots()
{
}

WBCompRosaHeadshot::~WBCompRosaHeadshot()
{
}

/*virtual*/ void WBCompRosaHeadshot::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( NumHeadshots );
	const uint NumHeadshots = ConfigManager::GetInheritedInt( sNumHeadshots, 0, sDefinitionName );
	for( uint HeadshotIndex = 0; HeadshotIndex < NumHeadshots; ++HeadshotIndex )
	{
		const HashedString	HeadshotBone	= ConfigManager::GetInheritedSequenceHash( "Headshot%dBone", HeadshotIndex, HashedString::NullString, sDefinitionName );
		const float			HeadshotMod		= ConfigManager::GetInheritedSequenceFloat( "Headshot%dMod", HeadshotIndex, 0.0f, sDefinitionName );
		m_Headshots[ HeadshotBone ] = HeadshotMod;
	}
}

float WBCompRosaHeadshot::GetHeadshotMod( const HashedString& BoneName ) const
{
	THeadshotMap::Iterator HeadshotIter = m_Headshots.Search( BoneName );
	return HeadshotIter.IsValid() ? HeadshotIter.GetValue() : 1.0f;
}
