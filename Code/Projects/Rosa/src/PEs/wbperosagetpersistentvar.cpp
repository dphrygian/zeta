#include "core.h"
#include "wbperosagetpersistentvar.h"
#include "configmanager.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "rosapersistence.h"

WBPERosaGetPersistentVar::WBPERosaGetPersistentVar()
:	m_Key()
{
}

WBPERosaGetPersistentVar::~WBPERosaGetPersistentVar()
{
}

/*virtual*/ void WBPERosaGetPersistentVar::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( Key );
	m_Key = ConfigManager::GetHash( sKey, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBPERosaGetPersistentVar::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	Unused( Context );

	RosaFramework* const pFramework = RosaFramework::GetInstance();
	ASSERT( pFramework );

	RosaGame* const pGame = pFramework->GetGame();
	ASSERT( pGame );

	RosaPersistence* const pPersistence = pGame->GetPersistence();
	ASSERT( pPersistence );

	const WBEvent& PersistentVars = pPersistence->GetVariableMap();
	const WBEvent::SParameter* pParam = PersistentVars.GetParameter( m_Key );

	EvaluatedParam.Set( pParam );
}
