#include "core.h"
#include "animeventrosaexecuteaction.h"
#include "wbactionfactory.h"
#include "configmanager.h"
#include "wbactionstack.h"
#include "wbevent.h"
#include "rosamesh.h"
#include "Components/wbcompowner.h"

AnimEventRosaExecuteAction::AnimEventRosaExecuteAction()
:	m_Actions()
{
}

AnimEventRosaExecuteAction::~AnimEventRosaExecuteAction()
{
	WBActionFactory::ClearActionArray( m_Actions );
}

void AnimEventRosaExecuteAction::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBActionFactory::InitializeActionArray( DefinitionName, m_Actions );
}

void AnimEventRosaExecuteAction::Call( Mesh* pMesh, Animation* pAnimation )
{
	Unused( pMesh );
	Unused( pAnimation );

	RosaMesh* const	pRosaMesh	= static_cast<RosaMesh*>( pMesh );
	WBEntity* const		pEntity			= WBCompOwner::GetTopmostOwner( pRosaMesh->GetEntity() );
	DEVASSERT( pEntity );

	WBEvent OnAnimEventEvent;
	STATIC_HASHED_STRING( OnAnimEvent );
	OnAnimEventEvent.SetEventName( sOnAnimEvent );
	pEntity->AddContextToEvent( OnAnimEventEvent );

	WBActionFactory::ExecuteActionArray( m_Actions, OnAnimEventEvent, pEntity );
}
