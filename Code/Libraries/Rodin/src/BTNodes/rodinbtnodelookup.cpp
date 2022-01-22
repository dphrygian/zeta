#include "core.h"
#include "rodinbtnodelookup.h"
#include "rodinbtnodefactory.h"
#include "Components/wbcomprodinbehaviortree.h"
#include "configmanager.h"

RodinBTNodeLookup::RodinBTNodeLookup()
{
}

RodinBTNodeLookup::~RodinBTNodeLookup()
{
}

void RodinBTNodeLookup::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	ASSERT( m_BehaviorTree );

	MAKEHASH( DefinitionName );

	STATICHASH( Key );
	const HashedString Key = ConfigManager::GetHash( sKey, "", sDefinitionName );

	STATICHASH( Default );
	const SimpleString Default = ConfigManager::GetString( sDefault, "", sDefinitionName );

	const SimpleString NodeDef = m_BehaviorTree->GetLookupNode( Key );
	const SimpleString UsingDef = ( NodeDef == "" ) ? Default : NodeDef;
	DEVASSERT( UsingDef != "" );

	m_Child = RodinBTNodeFactory::Create( UsingDef, m_BehaviorTree );
	DEVASSERT( m_Child );
}
