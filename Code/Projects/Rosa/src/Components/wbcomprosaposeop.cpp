#include "core.h"
#include "wbcomprosaposeop.h"
#include "configmanager.h"

WBCompRosaPoseOp::WBCompRosaPoseOp()
:	m_AssignedEntity()
,	m_IdleAnimation()
,	m_RefHeight( 0.0f )
,	m_PairTag()
,	m_Enabled( false )
,	m_RequiredActorProps()
,	m_ForbiddenActorProps()
,	m_RequiredActorRels()
,	m_ForbiddenActorRels()
{
}

WBCompRosaPoseOp::~WBCompRosaPoseOp()
{
}

void WBCompRosaPoseOp::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( IdleAnimation );
	m_IdleAnimation = ConfigManager::GetInheritedHash( sIdleAnimation, HashedString::NullString, sDefinitionName );

	STATICHASH( RefHeight );
	m_RefHeight = ConfigManager::GetInheritedFloat( sRefHeight, 0.0f, sDefinitionName );

	STATICHASH( PairTag );
	m_PairTag = ConfigManager::GetInheritedHash( sPairTag, HashedString::NullString, sDefinitionName );

	STATICHASH( Enabled );
	m_Enabled = ConfigManager::GetInheritedBool( sEnabled, true, sDefinitionName );

	STATICHASH( NumRequiredActorProps );
	const uint NumRequiredActorProps = ConfigManager::GetInheritedInt( sNumRequiredActorProps, 0, sDefinitionName );
	FOR_EACH_INDEX( RequiredActorPropIndex, NumRequiredActorProps )
	{
		const HashedString RequiredActorProp = ConfigManager::GetInheritedSequenceHash( "RequiredActorProp%d", RequiredActorPropIndex, HashedString::NullString, sDefinitionName );
		m_RequiredActorProps.PushBack( RequiredActorProp );
	}

	STATICHASH( NumForbiddenActorProps );
	const uint NumForbiddenActorProps = ConfigManager::GetInheritedInt( sNumForbiddenActorProps, 0, sDefinitionName );
	FOR_EACH_INDEX( ForbiddenActorPropIndex, NumForbiddenActorProps )
	{
		const HashedString ForbiddenActorProp = ConfigManager::GetInheritedSequenceHash( "ForbiddenActorProp%d", ForbiddenActorPropIndex, HashedString::NullString, sDefinitionName );
		m_ForbiddenActorProps.PushBack( ForbiddenActorProp );
	}

	STATICHASH( NumRequiredActorRels );
	const uint NumRequiredActorRels = ConfigManager::GetInheritedInt( sNumRequiredActorRels, 0, sDefinitionName );
	FOR_EACH_INDEX( RequiredActorRelsIndex, NumRequiredActorRels )
	{
		const HashedString RequiredActorRels = ConfigManager::GetInheritedSequenceHash( "RequiredActorRels%d", RequiredActorRelsIndex, HashedString::NullString, sDefinitionName );
		m_RequiredActorRels.PushBack( RequiredActorRels );
	}

	STATICHASH( NumForbiddenActorRels );
	const uint NumForbiddenActorRels = ConfigManager::GetInheritedInt( sNumForbiddenActorRels, 0, sDefinitionName );
	FOR_EACH_INDEX( ForbiddenActorRelsIndex, NumForbiddenActorRels )
	{
		const HashedString ForbiddenActorRels = ConfigManager::GetInheritedSequenceHash( "ForbiddenActorRels%d", ForbiddenActorRelsIndex, HashedString::NullString, sDefinitionName );
		m_ForbiddenActorRels.PushBack( ForbiddenActorRels );
	}
}
