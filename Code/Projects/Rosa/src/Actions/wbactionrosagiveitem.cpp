#include "core.h"
#include "wbactionrosagiveitem.h"
#include "configmanager.h"
#include "wbactionstack.h"
#include "wbevent.h"
#include "Components/wbcomprosainventory.h"

WBActionRosaGiveItem::WBActionRosaGiveItem()
:	m_ItemDef()
,	m_ItemDefPE()
,	m_GiveToPE()
{
}

WBActionRosaGiveItem::~WBActionRosaGiveItem()
{
}

/*virtual*/ void WBActionRosaGiveItem::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Item );
	m_ItemDef = ConfigManager::GetString( sItem, "", sDefinitionName );

	STATICHASH( ItemPE );
	const SimpleString ItemPE = ConfigManager::GetString( sItemPE, "", sDefinitionName );
	m_ItemDefPE.InitializeFromDefinition( ItemPE );

	STATICHASH( GiveTo );
	const SimpleString GiveToDef = ConfigManager::GetString( sGiveTo, "", sDefinitionName );
	m_GiveToPE.InitializeFromDefinition( GiveToDef );
}

/*virtual*/ void WBActionRosaGiveItem::Execute()
{
	WBAction::Execute();

	WBEntity* const pEntity = GetEntity();

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = pEntity;

	m_ItemDefPE.Evaluate( PEContext );
	const SimpleString ItemDef = ( m_ItemDefPE.GetType() == WBParamEvaluator::EPT_String ) ? m_ItemDefPE.GetString() : m_ItemDef;

	m_GiveToPE.Evaluate( PEContext );
	WBEntity* const pGiveToEntity = m_GiveToPE.GetEntity();
	if( pGiveToEntity )
	{
		GiveItemTo( ItemDef, pGiveToEntity );
	}
}

void WBActionRosaGiveItem::GiveItemTo( const SimpleString& ItemDef, WBEntity* const pEntity ) const
{
	DEVASSERT( pEntity );

	WBCompRosaInventory* const	pInventory		= WB_GETCOMP( pEntity, RosaInventory );
	ASSERT( pInventory );

	WBEntity* const				pGivenEntity	= WBWorld::GetInstance()->CreateEntity( ItemDef );
	ASSERT( pGivenEntity );

	const bool ShowLogMessage = true;
	pInventory->AddItem( pGivenEntity, ShowLogMessage );
}
