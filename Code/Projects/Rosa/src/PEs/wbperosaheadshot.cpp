#include "core.h"
#include "wbperosaheadshot.h"
#include "Components/wbcomprosaheadshot.h"
#include "Components/wbcomprosamesh.h"
#include "configmanager.h"
#include "wbparamevaluatorfactory.h"

WBPERosaHeadshot::WBPERosaHeadshot()
:	m_EntityPE( NULL )
,	m_BonePE( NULL )
{
}

WBPERosaHeadshot::~WBPERosaHeadshot()
{
	SafeDelete( m_EntityPE );
	SafeDelete( m_BonePE );
}

/*virtual*/ void WBPERosaHeadshot::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( EntityPE );
	m_EntityPE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sEntityPE, "", sDefinitionName ) );

	STATICHASH( BonePE );
	m_BonePE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sBonePE, "", sDefinitionName ) );
}

/*virtual*/ void WBPERosaHeadshot::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam EntityValue;
	m_EntityPE->Evaluate( Context, EntityValue );

	WBParamEvaluator::SEvaluatedParam BoneValue;
	m_BonePE->Evaluate( Context, BoneValue );

	WBEntity* const pEntity = EntityValue.GetEntity();
	if( !pEntity )
	{
		return;
	}

	WBCompRosaHeadshot* const	pHeadshot	= WB_GETCOMP( pEntity, RosaHeadshot );
	WBCompRosaMesh* const		pMesh		= WB_GETCOMP( pEntity, RosaMesh );
	const HashedString			BoneName	= pMesh		? pMesh->GetBoneName( BoneValue.GetInt() )	: HashedString::NullString;
	const float					HeadshotMod	= pHeadshot	? pHeadshot->GetHeadshotMod( BoneName )		: 1.0f;

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Float;
	EvaluatedParam.m_Float	= HeadshotMod;
}
