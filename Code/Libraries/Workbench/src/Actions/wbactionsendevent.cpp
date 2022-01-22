#include "core.h"
#include "wbactionsendevent.h"
#include "configmanager.h"
#include "wbeventmanager.h"
#include "Components/wbcompvariablemap.h"
#include "Components/wbcomplabel.h"

WBActionSendEvent::WBActionSendEvent()
:	m_EventName()
,	m_QueueEvent( false )
,	m_LogEvent( false )
,	m_DispatchDelay( 0.0f )
,	m_DispatchDelayPE()
,	m_DispatchTicks( 0 )
,	m_EventOwnerPE()
,	m_RecipientPE()
,	m_RecipientLabel()
,	m_Parameters()
,	m_VariableMapTag()
{
}

WBActionSendEvent::~WBActionSendEvent()
{
}

/*virtual*/ void WBActionSendEvent::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( EventName );
	m_EventName = ConfigManager::GetHash( sEventName, HashedString::NullString, sDefinitionName );
	ASSERT( m_EventName != HashedString::NullString );

	STATICHASH( LogEvent );
	m_LogEvent = ConfigManager::GetBool( sLogEvent, false, sDefinitionName );

	STATICHASH( DispatchDelay );
	m_DispatchDelay = ConfigManager::GetFloat( sDispatchDelay, 0.0f, sDefinitionName );

	STATICHASH( DispatchDelayPE );
	const SimpleString DispatchDelayPEDef = ConfigManager::GetString( sDispatchDelayPE, "", sDefinitionName );
	m_DispatchDelayPE.InitializeFromDefinition( DispatchDelayPEDef );

	STATICHASH( DispatchTicks );
	m_DispatchTicks = ConfigManager::GetInt( sDispatchTicks, 0, sDefinitionName );

	const bool DefaultQueue = ( m_DispatchDelay > 0.0f ) || m_DispatchDelayPE.IsInitialized() || ( m_DispatchTicks > 0 );
	STATICHASH( QueueEvent );
	m_QueueEvent = ConfigManager::GetBool( sQueueEvent, DefaultQueue, sDefinitionName );

	STATICHASH( EventOwner );
	const SimpleString EventOwnerDef = ConfigManager::GetString( sEventOwner, "", sDefinitionName );
	m_EventOwnerPE.InitializeFromDefinition( EventOwnerDef );

	STATICHASH( Recipient );
	const SimpleString RecipientDef = ConfigManager::GetString( sRecipient, "", sDefinitionName );
	m_RecipientPE.InitializeFromDefinition( RecipientDef );

	STATICHASH( RecipientLabel );
	m_RecipientLabel = ConfigManager::GetHash( sRecipientLabel, HashedString::NullString, sDefinitionName );

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

	STATICHASH( VariableMapTag );
	m_VariableMapTag = ConfigManager::GetHash( sVariableMapTag, HashedString::NullString, sDefinitionName );
}

void WBActionSendEvent::SendEvent( const WBEvent& Event, WBEntity* const pRecipient, WBEntity* const pEventOwner, const float DispatchDelay, const uint DispatchTicks ) const
{
	if( m_QueueEvent )
	{
		const TEventUID EventUID = WBWorld::GetInstance()->GetEventManager()->QueueEvent( Event, pRecipient, DispatchDelay, DispatchTicks );

		if( pEventOwner && m_VariableMapTag != HashedString::NullString )
		{
			WBCompVariableMap* const pVarMap = WB_GETCOMP( pEventOwner, VariableMap );
			DEVASSERT( pVarMap );

			WBEvent& VariableMap = pVarMap->GetVariables();
			VariableMap.SetInt( m_VariableMapTag, EventUID );
		}
	}
	else
	{
		WBWorld::GetInstance()->GetEventManager()->DispatchEvent( Event, pRecipient );
	}
}

/*virtual*/ void WBActionSendEvent::Execute()
{
	WBAction::Execute();

	WBEntity* const pContextEntity = GetEntity();

	WBEvent Event;

	Event.SetEventName( m_EventName );

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = pContextEntity;

	m_EventOwnerPE.Evaluate( PEContext );

	WBEntity* const pEvaluatedEventOwnerEntity	= m_EventOwnerPE.GetEntity();
	WBEntity* const pEventOwnerEntity			= pEvaluatedEventOwnerEntity ? pEvaluatedEventOwnerEntity : pContextEntity;

	if( pEventOwnerEntity )
	{
		pEventOwnerEntity->AddContextToEvent( Event );
	}

	if( m_LogEvent )
	{
		STATIC_HASHED_STRING( WBEvent_LogEvent );
		Event.SetBool( sWBEvent_LogEvent, true );
	}

	const uint NumParameters = m_Parameters.Size();
	for( uint ParameterIndex = 0; ParameterIndex < NumParameters; ++ParameterIndex )
	{
		SNamedParameter& Parameter = m_Parameters[ ParameterIndex ];
		Parameter.m_ValuePE.Evaluate( PEContext );
		Event.SetFromPE( Parameter.m_Name, Parameter.m_ValuePE );
	}

	float DispatchDelay = 0.0f;
	if( m_QueueEvent )
	{
		m_DispatchDelayPE.Evaluate( PEContext );
		DispatchDelay = ( m_DispatchDelayPE.GetType() == WBParamEvaluator::EPT_Float ) ? m_DispatchDelayPE.GetFloat() : m_DispatchDelay;
	}

	if( m_RecipientLabel )
	{
		Array<WBEntity*> RecipientEntities;
		WBCompLabel::GetEntitiesByLabel( m_RecipientLabel, RecipientEntities );
		FOR_EACH_ARRAY( RecipientIter, RecipientEntities, WBEntity* )
		{
			WBEntity* const pRecipientEntity = RecipientIter.GetValue();
			SendEvent( Event, pRecipientEntity, pEventOwnerEntity, DispatchDelay, m_DispatchTicks );
		}
	}
	else
	{
		m_RecipientPE.Evaluate( PEContext );
		WBEntity* const pRecipientEntity = m_RecipientPE.GetEntity();
		SendEvent( Event, pRecipientEntity, pEventOwnerEntity, DispatchDelay, m_DispatchTicks );
	}
}
