#include "core.h"
#include "animeventrosasendevent.h"
#include "configmanager.h"
#include "wbactionstack.h"
#include "wbeventmanager.h"
#include "rosamesh.h"

AnimEventRosaSendEvent::AnimEventRosaSendEvent()
:	m_EventName( HashedString::NullString )
,	m_QueueEvent( false )
,	m_DispatchDelay( 0.0f )
,	m_DispatchTicks( 0 )
,	m_Parameters()
{
}

AnimEventRosaSendEvent::~AnimEventRosaSendEvent()
{
}

void AnimEventRosaSendEvent::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( EventName );
	m_EventName = ConfigManager::GetHash( sEventName, "", sDefinitionName );

	STATICHASH( DispatchDelay );
	m_DispatchDelay = ConfigManager::GetFloat( sDispatchDelay, 0.0f, sDefinitionName );

	STATICHASH( DispatchTicks );
	m_DispatchTicks = ConfigManager::GetInt( sDispatchTicks, 0, sDefinitionName );

	const bool DefaultQueue = ( m_DispatchDelay > 0.0f ) || ( m_DispatchTicks > 0 );
	STATICHASH( QueueEvent );
	m_QueueEvent = ConfigManager::GetBool( sQueueEvent, DefaultQueue, sDefinitionName );

	STATICHASH( NumParameters );
	const uint NumParameters = ConfigManager::GetInt( sNumParameters, 0, sDefinitionName );
	for( uint ParameterIndex = 0; ParameterIndex < NumParameters; ++ParameterIndex )
	{
		const HashedString Name = ConfigManager::GetSequenceHash( "Parameter%dName", ParameterIndex, HashedString::NullString, sDefinitionName );
		const SimpleString ValuePEDef = ConfigManager::GetSequenceString( "Parameter%dValue", ParameterIndex, "", sDefinitionName );

		SNamedParameter& Parameter = m_Parameters.PushBack();

		Parameter.m_Name = Name;
		Parameter.m_ValuePE.InitializeFromDefinition( ValuePEDef );
	}
}

void AnimEventRosaSendEvent::Call( Mesh* pMesh, Animation* pAnimation )
{
	Unused( pAnimation );

	RosaMesh* const pRosaMesh = static_cast<RosaMesh*>( pMesh );
	WBEntity* const pEntity = pRosaMesh->GetEntity();

	WBEvent Event;

	Event.SetEventName( m_EventName );

	if( pEntity )
	{
		pEntity->AddContextToEvent( Event );
	}

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = pEntity;

	const uint NumParameters = m_Parameters.Size();
	for( uint ParameterIndex = 0; ParameterIndex < NumParameters; ++ParameterIndex )
	{
		SNamedParameter& Parameter = m_Parameters[ ParameterIndex ];
		Parameter.m_ValuePE.Evaluate( PEContext );
		Event.SetFromPE( Parameter.m_Name, Parameter.m_ValuePE );
	}

	if( m_QueueEvent )
	{
		WBWorld::GetInstance()->GetEventManager()->QueueEvent( Event, pEntity, m_DispatchDelay, m_DispatchTicks );
	}
	else
	{
		WBWorld::GetInstance()->GetEventManager()->DispatchEvent( Event, pEntity );
	}
}
