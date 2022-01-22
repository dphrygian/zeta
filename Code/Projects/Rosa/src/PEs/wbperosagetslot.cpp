#include "core.h"
#include "wbperosagetslot.h"
#include "Components/wbcomprosaitem.h"
#include "configmanager.h"
#include "wbparamevaluatorfactory.h"
#include "reversehash.h"

WBPERosaGetSlot::WBPERosaGetSlot()
:	m_EntityPE( NULL )
{
}

WBPERosaGetSlot::~WBPERosaGetSlot()
{
	SafeDelete( m_EntityPE );
}

/*virtual*/ void WBPERosaGetSlot::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( EntityPE );
	m_EntityPE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sEntityPE, "", sDefinitionName ) );
}

/*virtual*/ void WBPERosaGetSlot::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam EntityValue;
	m_EntityPE->Evaluate( Context, EntityValue );

	WBEntity* const pEntity = EntityValue.GetEntity();
	if( !pEntity )
	{
		return;
	}

	WBCompRosaItem* const pItem = WB_GETCOMP( pEntity, RosaItem );
	if( !pItem )
	{
		return;
	}

	const HashedString	SlotHash	= pItem->GetSlot();

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_String;
	EvaluatedParam.m_String	= ReverseHash::ReversedHash( SlotHash );
}
