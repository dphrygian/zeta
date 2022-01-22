#include "core.h"
#include "wbactionrosaremoveammo.h"
#include "configmanager.h"
#include "rosagame.h"
#include "wbeventmanager.h"

WBActionRosaRemoveAmmo::WBActionRosaRemoveAmmo()
:	m_Spend( false )
,	m_Type()
,	m_TypePE()
,	m_Count( 0 )
,	m_CountPE()
{
}

WBActionRosaRemoveAmmo::~WBActionRosaRemoveAmmo()
{
}

/*virtual*/ void WBActionRosaRemoveAmmo::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Spend );
	m_Spend = ConfigManager::GetBool( sSpend, false, sDefinitionName );

	STATICHASH( Type );
	m_Type = ConfigManager::GetHash( sType, HashedString::NullString, sDefinitionName );

	STATICHASH( TypePE );
	const SimpleString TypePEDef = ConfigManager::GetString( sTypePE, "", sDefinitionName );
	m_TypePE.InitializeFromDefinition( TypePEDef );

	STATICHASH( Count );
	m_Count = ConfigManager::GetInt( sCount, 1, sDefinitionName );

	STATICHASH( CountPE );
	const SimpleString CountPEDef = ConfigManager::GetString( sCountPE, "", sDefinitionName );
	m_CountPE.InitializeFromDefinition( CountPEDef );
}

/*virtual*/ void WBActionRosaRemoveAmmo::Execute()
{
	WBAction::Execute();

	WBEntity* const			pPlayer		= RosaGame::GetPlayer();
	DEVASSERT( pPlayer );

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity					= pPlayer;

	m_TypePE.Evaluate( PEContext );
	const HashedString		Type		= m_TypePE.HasRoot() ? m_TypePE.GetString() : m_Type;

	m_CountPE.Evaluate( PEContext );
	const uint				Count		= m_CountPE.HasRoot() ? m_CountPE.GetInt() : m_Count;

	if( m_Spend )
	{
		WB_MAKE_EVENT( SpendAmmo, pPlayer );
		WB_SET_AUTO( SpendAmmo, Int, AmmoCount, Count );
		WB_DISPATCH_EVENT( GetEventManager(), SpendAmmo, pPlayer );
	}
	else
	{
		WB_MAKE_EVENT( RemoveAmmo, pPlayer );
		WB_SET_AUTO( RemoveAmmo, Hash, AmmoType, Type );
		WB_SET_AUTO( RemoveAmmo, Int, AmmoCount, Count );
		WB_SET_AUTO( RemoveAmmo, Bool, ShowLogMessage, false );
		WB_DISPATCH_EVENT( GetEventManager(), RemoveAmmo, pPlayer );
	}
}
