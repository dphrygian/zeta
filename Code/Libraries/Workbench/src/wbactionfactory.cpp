#include "core.h"
#include "wbactionfactory.h"
#include "configmanager.h"
#include "wbaction.h"
#include "wbactions.h"
#include "wbactionstack.h"

static Map<HashedString, WBActionFactoryFunc>	sFactoryFuncMap;

void WBActionFactory::RegisterFactory( const HashedString& TypeName, WBActionFactoryFunc Factory )
{
	ASSERT( sFactoryFuncMap.Search( TypeName ).IsNull() );
	ASSERT( Factory );
	sFactoryFuncMap[ TypeName ] = Factory;
}

void WBActionFactory::InitializeBaseFactories()
{
#define ADDWBACTIONFACTORY( type ) WBActionFactory::RegisterFactory( #type, WBAction##type::Factory )
#include "wbactions.h"
#undef ADDWBACTIONFACTORY
}

void WBActionFactory::ShutDown()
{
	sFactoryFuncMap.Clear();
}

WBAction* WBActionFactory::Create( const SimpleString& DefinitionName )
{
	if( DefinitionName == "" )
	{
		return NULL;
	}

	STATICHASH( ActionType );
	MAKEHASH( DefinitionName );
	HashedString ActionType = ConfigManager::GetHash( sActionType, "", sDefinitionName );

	Map<HashedString, WBActionFactoryFunc>::Iterator FactoryIter = sFactoryFuncMap.Search( ActionType );
	if( FactoryIter.IsNull() )
	{
		PRINTF( "Invalid type requested for WBAction %s.\n", DefinitionName.CStr() );
		WARNDESC( "Invalid WBAction type requested." );
		return NULL;
	}

	WBActionFactoryFunc pFactory = ( *FactoryIter );
	ASSERT( pFactory );

	WBAction* pAction = pFactory();
	ASSERT( pAction );

	pAction->InitializeFromDefinition( DefinitionName );

	return pAction;
}

void WBActionFactory::InitializeActionArray( const HashedString& DefinitionName, Array<WBAction*>& OutActionArray )
{
	// NOTE: This will fail to compile if I use PARANOID_HASH_CHECK. If I ever
	// need to test that, I can pass a string instead of a hash for the name.

	STATICHASH( NumActions );
	const uint NumActions = ConfigManager::GetInheritedInt( sNumActions, 0, DefinitionName );
	for( uint ActionIndex = 0; ActionIndex < NumActions; ++ActionIndex )
	{
		const SimpleString Action = ConfigManager::GetInheritedSequenceString( "Action%d", ActionIndex, "", DefinitionName );
		WBAction* const pAction = WBActionFactory::Create( Action );
		if( pAction )
		{
			OutActionArray.PushBack( pAction );
		}
	}
}

void WBActionFactory::InitializeActionArray( const HashedString& DefinitionName, const SimpleString& ActionPrefix, Array<WBAction*>& OutActionArray )
{
	const SimpleString NumPrefixActionsString = SimpleString::PrintF( "Num%sActions", ActionPrefix.CStr() );
	const SimpleString PrefixActionString = SimpleString::PrintF( "%sAction%%d", ActionPrefix.CStr() );

	MAKEHASHFROM( NumPrefixActions, NumPrefixActionsString );
	const uint NumActions = ConfigManager::GetInheritedInt( sNumPrefixActions, 0, DefinitionName );

	for( uint ActionIndex = 0; ActionIndex < NumActions; ++ActionIndex )
	{
		const SimpleString Action = ConfigManager::GetInheritedSequenceString( PrefixActionString, ActionIndex, "", DefinitionName );
		WBAction* const pAction = WBActionFactory::Create( Action );
		if( pAction )
		{
			OutActionArray.PushBack( pAction );
		}
	}
}

void WBActionFactory::ClearActionArray( Array<WBAction*>& OutActionArray )
{
	FOR_EACH_ARRAY( ActionIter, OutActionArray, WBAction* )
	{
		WBAction* pAction = ActionIter.GetValue();
		SafeDelete( pAction );
	}

	OutActionArray.Clear();
}

void WBActionFactory::ExecuteActionArray( const Array<WBAction*>& ActionArray, const WBEvent& Event, WBEntity* const pActingEntity )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	FOR_EACH_ARRAY( ActionIter, ActionArray, WBAction* )
	{
		WBAction* const pAction = ActionIter.GetValue();
		DEVASSERT( pAction );

		WBActionStack::Push( Event, pActingEntity );
		pAction->Execute();
		WBActionStack::Pop();
	}
}
