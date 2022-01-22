#include "core.h"
#include "wbperosagetfaction.h"
#include "Components/wbcomprosafaction.h"
#include "configmanager.h"
#include "wbparamevaluatorfactory.h"
#include "reversehash.h"

WBPERosaGetFaction::WBPERosaGetFaction()
:	m_EntityPE( NULL )
{
}

WBPERosaGetFaction::~WBPERosaGetFaction()
{
	SafeDelete( m_EntityPE );
}

/*virtual*/ void WBPERosaGetFaction::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( EntityPE );
	m_EntityPE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sEntityPE, "", sDefinitionName ) );
}

/*virtual*/ void WBPERosaGetFaction::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam EntityValue;
	m_EntityPE->Evaluate( Context, EntityValue );

	WBEntity* const pEntity = EntityValue.GetEntity();
	if( !pEntity )
	{
		return;
	}

	WBCompRosaFaction* const pFaction = WB_GETCOMP( pEntity, RosaFaction );
	if( !pFaction )
	{
		return;
	}

	const SimpleString Faction = ReverseHash::ReversedHash( pFaction->GetFaction() );

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_String;
	EvaluatedParam.m_String	= Faction;
}
