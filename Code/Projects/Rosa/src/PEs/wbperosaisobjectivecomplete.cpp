#include "core.h"
#include "wbperosaisobjectivecomplete.h"
#include "rosagame.h"
#include "Components/wbcomprosaobjectives.h"
#include "configmanager.h"

WBPERosaIsObjectiveComplete::WBPERosaIsObjectiveComplete()
:	m_ObjectiveTag()
,	m_RejectFail( false )
{
}

WBPERosaIsObjectiveComplete::~WBPERosaIsObjectiveComplete()
{
}

/*virtual*/ void WBPERosaIsObjectiveComplete::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( Objective );
	m_ObjectiveTag = ConfigManager::GetHash( sObjective, HashedString::NullString, sDefinitionName );

	STATICHASH( RejectFail );
	m_RejectFail = ConfigManager::GetBool( sRejectFail, false, sDefinitionName );
}

/*virtual*/ void WBPERosaIsObjectiveComplete::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	Unused( Context );

	WBEntity* const				pPlayer		= RosaGame::GetPlayer();
	if( !pPlayer )
	{
		return;
	}

	WBCompRosaObjectives* const	pObjectives	= WB_GETCOMP( pPlayer, RosaObjectives );
	if( !pObjectives )
	{
		return;
	}

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Bool;
	EvaluatedParam.m_Bool	= pObjectives->IsObjectiveComplete( m_ObjectiveTag, m_RejectFail );
}
