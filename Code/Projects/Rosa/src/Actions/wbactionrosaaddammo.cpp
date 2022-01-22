#include "core.h"
#include "wbactionrosaaddammo.h"
#include "configmanager.h"
#include "rosagame.h"
#include "wbeventmanager.h"

WBActionRosaAddAmmo::WBActionRosaAddAmmo()
:	m_Type()
,	m_TypePE()
,	m_Count( 0 )
,	m_CountPE()
{
}

WBActionRosaAddAmmo::~WBActionRosaAddAmmo()
{
}

/*virtual*/ void WBActionRosaAddAmmo::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

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

	STATICHASH( SuppressLog );
	m_SuppressLog = ConfigManager::GetBool( sSuppressLog, false, sDefinitionName );
}

/*virtual*/ void WBActionRosaAddAmmo::Execute()
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

	// HACKHACK for Zeta
	{
		WB_MAKE_EVENT( FlushMagazines, pPlayer );
		WB_DISPATCH_EVENT( GetEventManager(), FlushMagazines, pPlayer );
	}

	WB_MAKE_EVENT( AddAmmo, pPlayer );
	WB_SET_AUTO( AddAmmo, Hash, AmmoType, Type );
	WB_SET_AUTO( AddAmmo, Int, AmmoCount, Count );
	WB_SET_AUTO( AddAmmo, Bool, ShowLogMessage, !m_SuppressLog );
	WB_DISPATCH_EVENT( GetEventManager(), AddAmmo, pPlayer );
}
