#include "core.h"
#include "wbactionrosaplayanim.h"
#include "configmanager.h"
#include "wbactionstack.h"
#include "wbeventmanager.h"

WBActionRosaPlayAnim::WBActionRosaPlayAnim()
:	m_AnimationName( "" )
,	m_Loop( false )
,	m_PlayRate( 0.0f )
,	m_PlayRatePE()
,	m_BlendTime( 0.0f )
,	m_Layered( false )
,	m_Additive( false )
{
}

WBActionRosaPlayAnim::~WBActionRosaPlayAnim()
{
}

/*virtual*/ void WBActionRosaPlayAnim::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Animation );
	m_AnimationName = ConfigManager::GetHash( sAnimation, HashedString::NullString, sDefinitionName );

	STATICHASH( Loop );
	m_Loop = ConfigManager::GetBool( sLoop, false, sDefinitionName );

	STATICHASH( PlayRate );
	m_PlayRate = ConfigManager::GetFloat( sPlayRate, 0.0f, sDefinitionName );

	STATICHASH( PlayRatePE );
	SimpleString PlayRateDef = ConfigManager::GetString( sPlayRatePE, "", sDefinitionName );
	m_PlayRatePE.InitializeFromDefinition( PlayRateDef );

	STATICHASH( BlendTime );
	m_BlendTime = ConfigManager::GetFloat( sBlendTime, 0.0f, sDefinitionName );

	STATICHASH( Layered );
	m_Layered = ConfigManager::GetBool( sLayered, false, sDefinitionName );

	STATICHASH( Additive );
	m_Additive = ConfigManager::GetBool( sAdditive, false, sDefinitionName );
}

/*virtual*/ void WBActionRosaPlayAnim::Execute()
{
	WBAction::Execute();

	WBEntity* const pEntity = GetEntity();

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = pEntity;

	m_PlayRatePE.Evaluate( PEContext );
	const float PlayRate = ( m_PlayRatePE.GetType() == WBParamEvaluator::EPT_Float ) ? m_PlayRatePE.GetFloat() : m_PlayRate;

	if( pEntity )
	{
		WB_MAKE_EVENT( PlayAnim, pEntity );
		WB_SET_AUTO( PlayAnim, Hash, AnimationName, m_AnimationName );
		WB_SET_AUTO( PlayAnim, Bool, Loop, m_Loop );
		WB_SET_AUTO( PlayAnim, Float, PlayRate, PlayRate );
		WB_SET_AUTO( PlayAnim, Float, BlendTime, m_BlendTime );
		WB_SET_AUTO( PlayAnim, Bool, Layered, m_Layered );
		WB_SET_AUTO( PlayAnim, Bool, Additive, m_Additive );
		WB_DISPATCH_EVENT( GetEventManager(), PlayAnim, pEntity );
	}
}
