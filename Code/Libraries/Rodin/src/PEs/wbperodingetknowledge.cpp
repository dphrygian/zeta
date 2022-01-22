#include "core.h"
#include "wbperodingetknowledge.h"
#include "configmanager.h"
#include "wbparamevaluatorfactory.h"
#include "Components/wbcomprodinknowledge.h"

WBPERodinGetKnowledge::WBPERodinGetKnowledge()
:	m_EntityPE( NULL )
,	m_Key()
{
}

WBPERodinGetKnowledge::~WBPERodinGetKnowledge()
{
	SafeDelete( m_EntityPE );
}

/*virtual*/ void WBPERodinGetKnowledge::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( Entity );
	m_EntityPE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sEntity, "", sDefinitionName ) );

	STATICHASH( Key );
	m_Key = ConfigManager::GetHash( sKey, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBPERodinGetKnowledge::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	ASSERT( Context.m_Entity );

	WBParamEvaluator::SEvaluatedParam Value;
	m_EntityPE->Evaluate( Context, Value );

	WBEntity* const pKnowledgeEntity = Value.GetEntity();

	if( !pKnowledgeEntity )
	{
		return;
	}

	WBCompRodinKnowledge* const pKnowledge = WB_GETCOMP( Context.m_Entity, RodinKnowledge );
	if( !pKnowledge )
	{
		return;
	}

	const WBCompRodinKnowledge::TKnowledge* const pKnowledgeEntry = pKnowledge->GetKnowledge( pKnowledgeEntity );
	if( !pKnowledgeEntry )
	{
		return;
	}

	EvaluatedParam.Set( pKnowledgeEntry->GetParameter( m_Key ) );
}
