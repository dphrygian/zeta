#include "core.h"
#include "wbperosagetcharactervo.h"
//#include "Components/wbcomprosacharacter.h"
#include "configmanager.h"
#include "wbparamevaluatorfactory.h"

WBPERosaGetCharacterVO::WBPERosaGetCharacterVO()
:	m_EntityPE( NULL )
,	m_VO()
{
}

WBPERosaGetCharacterVO::~WBPERosaGetCharacterVO()
{
	SafeDelete( m_EntityPE );
}

/*virtual*/ void WBPERosaGetCharacterVO::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( EntityPE );
	m_EntityPE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sEntityPE, "", sDefinitionName ) );

	STATICHASH( VO );
	m_VO = ConfigManager::GetString( sVO, "", sDefinitionName );
}

/*virtual*/ void WBPERosaGetCharacterVO::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam EntityValue;
	m_EntityPE->Evaluate( Context, EntityValue );

	WBEntity* const pEntity = EntityValue.GetEntity();
	if( !pEntity )
	{
		return;
	}

	//WBCompRosaCharacter* const pCharacter = WB_GETCOMP( pEntity, RosaCharacter );
	//if( !pCharacter )
	//{
	//	return;
	//}

	// ZETATODO: Reimplement this as needed. It was only ever used on Eldritch;
	// AIs in Vamp used PEs from wardrobes, whole different system.
	const SimpleString VoiceSet = "";	//pCharacter->GetCurrentVoiceSet();
	MAKEHASH( VoiceSet );
	MAKEHASH( m_VO );

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_String;
	EvaluatedParam.m_String	= ConfigManager::GetString( sm_VO, "", sVoiceSet );
}
