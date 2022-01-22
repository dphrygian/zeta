#include "core.h"
#include "wbpegetname.h"
#include "configmanager.h"

WBPEGetName::WBPEGetName()
:	m_UniqueName( false )
{
}

WBPEGetName::~WBPEGetName()
{
}

/*virtual*/ void WBPEGetName::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBPEUnaryOp::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( UniqueName );
	m_UniqueName = ConfigManager::GetBool( sUniqueName, false, sDefinitionName );
}

void WBPEGetName::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam Value;
	m_Input->Evaluate( Context, Value );

	ASSERT( Value.m_Type == WBParamEvaluator::EPT_Entity );

	if( Value.m_Type != WBParamEvaluator::EPT_Entity )
	{
		return;
	}

	WBEntity* const pEntity = Value.m_Entity.Get();
	if( !pEntity )
	{
		return;
	}

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_String;
	EvaluatedParam.m_String	= m_UniqueName ? pEntity->GetUniqueName() : pEntity->GetName();
}
