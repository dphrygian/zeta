#include "core.h"
#include "wbperosagetitem.h"
#include "Components/wbcomprosainventory.h"
#include "configmanager.h"
#include "wbparamevaluatorfactory.h"

WBPERosaGetItem::WBPERosaGetItem()
:	m_EntityPE( NULL )
,	m_SlotPE( NULL )
{
}

WBPERosaGetItem::~WBPERosaGetItem()
{
	SafeDelete( m_EntityPE );
	SafeDelete( m_SlotPE );
}

/*virtual*/ void WBPERosaGetItem::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( EntityPE );
	m_EntityPE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sEntityPE, "", sDefinitionName ) );

	STATICHASH( SlotPE );
	m_SlotPE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sSlotPE, "", sDefinitionName ) );
}

/*virtual*/ void WBPERosaGetItem::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam EntityValue;
	m_EntityPE->Evaluate( Context, EntityValue );

	WBEntity* const pEntity = EntityValue.GetEntity();
	if( !pEntity )
	{
		return;
	}

	WBCompRosaInventory* const pInventory = WB_GETCOMP( pEntity, RosaInventory );
	if( !pInventory )
	{
		return;
	}

	WBParamEvaluator::SEvaluatedParam SlotValue;
	m_SlotPE->Evaluate( Context, SlotValue );

	ASSERT( SlotValue.m_Type == WBParamEvaluator::EPT_String );

	const HashedString	SlotHash	= SlotValue.m_String;
	WBEntity* const		pItem		= pInventory->GetItem( SlotHash );

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Entity;
	EvaluatedParam.m_Entity	= pItem;
}
