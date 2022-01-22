#include "core.h"
#include "wbactionrosaconditionalshowbook.h"
#include "configmanager.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "rosapersistence.h"

WBActionRosaConditionalShowBook::WBActionRosaConditionalShowBook()
:	m_PersistenceKey()
{
}

WBActionRosaConditionalShowBook::~WBActionRosaConditionalShowBook()
{
}

/*virtual*/ void WBActionRosaConditionalShowBook::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBActionRosaShowBook::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( PersistenceKey );
	m_PersistenceKey = ConfigManager::GetHash( sPersistenceKey, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBActionRosaConditionalShowBook::Execute()
{
	STATICHASH( ShowTutorials );
	const bool				ShowTutorials	= ConfigManager::GetBool( sShowTutorials );
	if( !ShowTutorials )
	{
		// The user has disabled tutorialization; but don't set the
		// persistence key, so they may see this book later if enabled.
		return;
	}

	RosaPersistence* const	pPersistence	= RosaFramework::GetInstance()->GetGame()->GetPersistence();
	DEVASSERT( pPersistence );

	WBEvent&				PersistentVars	= pPersistence->GetVariableMap();
	if( PersistentVars.GetBool( m_PersistenceKey ) )
	{
		// We've already shown this book once, ignore it now.
		// ROSATODO: Could change this to be a "show n times" thing instead of once only.
		return;
	}

	// Set the persistence key to mark that we've seen this book before.
	PersistentVars.SetBool( m_PersistenceKey, true );

	// Actually show the book.
	WBActionRosaShowBook::Execute();
}
