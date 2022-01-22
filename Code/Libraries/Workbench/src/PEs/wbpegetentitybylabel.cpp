#include "core.h"
#include "wbpegetentitybylabel.h"
#include "configmanager.h"
#include "Components/wbcomplabel.h"

WBPEGetEntityByLabel::WBPEGetEntityByLabel()
:	m_Label()
{
}

WBPEGetEntityByLabel::~WBPEGetEntityByLabel()
{
}

/*virtual*/ void WBPEGetEntityByLabel::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( Label );
	m_Label = ConfigManager::GetHash( sLabel, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBPEGetEntityByLabel::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	Unused( Context );

	WBEntity* const pLabelEntity = WBCompLabel::GetEntityByLabel( m_Label );

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Entity;
	EvaluatedParam.m_Entity	= pLabelEntity;
}
