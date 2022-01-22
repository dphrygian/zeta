#include "core.h"
#include "wbactionrosaplaybark.h"
#include "configmanager.h"
#include "wbeventmanager.h"

WBActionRosaPlayBark::WBActionRosaPlayBark()
:	m_SoundDef()
,	m_SoundDefPE()
,	m_Category()
{
}

WBActionRosaPlayBark::~WBActionRosaPlayBark()
{
}

/*virtual*/ void WBActionRosaPlayBark::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Sound );
	m_SoundDef = ConfigManager::GetString( sSound, "", sDefinitionName );

	STATICHASH( SoundPE );
	const SimpleString SoundPE = ConfigManager::GetString( sSoundPE, "", sDefinitionName );
	m_SoundDefPE.InitializeFromDefinition( SoundPE );

	STATICHASH( Category );
	m_Category = ConfigManager::GetHash( sCategory, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBActionRosaPlayBark::Execute()
{
	WBAction::Execute();

	WBEntity* const pEntity = GetEntity();

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = pEntity;
	m_SoundDefPE.Evaluate( PEContext );
	const SimpleString SoundDef = ( m_SoundDefPE.GetType() == WBParamEvaluator::EPT_String ) ? m_SoundDefPE.GetString() : m_SoundDef;

	WB_MAKE_EVENT( PlayBark, pEntity );
	WB_SET_AUTO( PlayBark, Hash, Sound, SoundDef );
	WB_SET_AUTO( PlayBark, Hash, Category, m_Category );
	WB_DISPATCH_EVENT( GetEventManager(), PlayBark, pEntity );
}
