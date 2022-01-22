#include "core.h"
#include "wbperosaconvoselector.h"
#include "configmanager.h"
#include "wbparamevaluatorfactory.h"
#include "Components/wbcompvariablemap.h"

WBPERosaConvoSelector::WBPERosaConvoSelector()
:	m_Selections()
{
}

WBPERosaConvoSelector::~WBPERosaConvoSelector()
{
}

/*virtual*/ void WBPERosaConvoSelector::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( NumSelections );
	const uint NumSelections = ConfigManager::GetInt( sNumSelections, 0, sDefinitionName );
	for( uint SelectionIndex = 0; SelectionIndex < NumSelections; ++SelectionIndex )
	{
		const HashedString State = ConfigManager::GetSequenceHash( "Selection%dState", SelectionIndex, HashedString::NullString, sDefinitionName );
		const SimpleString Convo = ConfigManager::GetSequenceString( "Selection%dConvo", SelectionIndex, "", sDefinitionName );

		m_Selections.Insert( State, Convo );
	}
}

/*virtual*/ void WBPERosaConvoSelector::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	if( !Context.m_Entity )
	{
		return;
	}

	WBCompVariableMap* const pVariableMap = WB_GETCOMP( Context.m_Entity, VariableMap );
	if( !pVariableMap )
	{
		return;
	}

	STATIC_HASHED_STRING( ConvoState );
	WBEvent& Variables = pVariableMap->GetVariables();
	const HashedString ConvoState = Variables.GetHash( sConvoState );

	const Map<HashedString, SimpleString>::Iterator ConvoIter = m_Selections.Search( ConvoState );
	if( ConvoIter.IsNull() )
	{
		return;
	}

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_String;
	EvaluatedParam.m_String	= ConvoIter.GetValue();
}
