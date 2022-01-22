#include "core.h"
#include "wbperosacampaignmodify.h"
#include "configmanager.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "rosacampaign.h"

WBPERosaCampaignModify::WBPERosaCampaignModify()
:	m_Name()
,	m_Override( false )
,	m_Force( false )
{
}

WBPERosaCampaignModify::~WBPERosaCampaignModify()
{
}

/*virtual*/ void WBPERosaCampaignModify::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBPEUnaryOp::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Name );
	m_Name = ConfigManager::GetHash( sName, HashedString::NullString, sDefinitionName );

	STATICHASH( Override );
	m_Override = ConfigManager::GetBool( sOverride, false, sDefinitionName );

	STATICHASH( Force );
	m_Force = ConfigManager::GetBool( sForce, false, sDefinitionName );
}

/*virtual*/ void WBPERosaCampaignModify::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	RosaFramework* const pFramework = RosaFramework::GetInstance();
	ASSERT( pFramework );

	RosaGame* const pGame = pFramework->GetGame();
	ASSERT( pGame );

	RosaCampaign* const pCampaign = pGame->GetCampaign();
	ASSERT( pCampaign );

	WBParamEvaluator::SEvaluatedParam Value;
	m_Input->Evaluate( Context, Value );

	if( Value.m_Type == WBParamEvaluator::EPT_Bool )
	{
		EvaluatedParam.m_Type = WBParamEvaluator::EPT_Bool;
		EvaluatedParam.m_Bool =
			pCampaign->OverrideBool(	m_Name,	Value.GetBool(),	m_Force );
	}
	else if( Value.m_Type == WBParamEvaluator::EPT_Int )
	{
		EvaluatedParam.m_Type = WBParamEvaluator::EPT_Int;
		EvaluatedParam.m_Int = m_Override ?
			pCampaign->OverrideInt(		m_Name,	Value.GetInt(),		m_Force ) :
			pCampaign->ModifyInt(		m_Name,	Value.GetInt(),		m_Force );
	}
	else if( Value.m_Type == WBParamEvaluator::EPT_Float )
	{
		EvaluatedParam.m_Type = WBParamEvaluator::EPT_Float;
		EvaluatedParam.m_Float = m_Override ?
			pCampaign->OverrideFloat(	m_Name,	Value.GetFloat(),	m_Force ) :
			pCampaign->ModifyFloat(		m_Name,	Value.GetFloat(),	m_Force );
	}
	else if( Value.m_Type == WBParamEvaluator::EPT_String )
	{
		EvaluatedParam.m_Type = WBParamEvaluator::EPT_String;
		EvaluatedParam.m_String = m_Override ?
			pCampaign->OverrideString(	m_Name,	Value.GetString(),	m_Force ) :
			pCampaign->AppendString(	m_Name,	Value.GetString(),	m_Force );
	}
}
