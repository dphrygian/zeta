#include "core.h"
#include "wbcomprosaswitch.h"
#include "wbcomprosaswitchable.h"
#include "wbcomprosatransform.h"
#include "idatastream.h"
#include "wbcomponentarrays.h"
#include "wbeventmanager.h"

WBCompRosaSwitch::WBCompRosaSwitch()
:	m_LinkedSwitchables()
{
}

WBCompRosaSwitch::~WBCompRosaSwitch()
{
}

/*virtual*/ void WBCompRosaSwitch::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( SetLinkedEntities );
	STATIC_HASHED_STRING( Switch );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sSetLinkedEntities )
	{
		STATIC_HASHED_STRING( LinkedEntities );
		void* pVoid = Event.GetPointer( sLinkedEntities );
		const Array<WBEntityRef>* const pLinkedEntities = static_cast<const Array<WBEntityRef>*>( pVoid );
		DEVASSERT( pLinkedEntities );

		SetLinkedSwitchables( *pLinkedEntities );
	}
	else if( EventName == sSwitch )
	{
		STATIC_HASHED_STRING( Frobber );
		WBEntity* const pFrobber = Event.GetEntity( sFrobber );

		Switch( pFrobber );
	}
}

void WBCompRosaSwitch::SetLinkedSwitchables( const Array<WBEntityRef>& LinkedEntities )
{
	DEVASSERT( m_LinkedSwitchables.Empty() );
	m_LinkedSwitchables.Reserve( LinkedEntities.Size() );

	FOR_EACH_ARRAY( LinkedEntityIter, LinkedEntities, WBEntityRef )
	{
		const WBEntityRef&	LinkedEntityRef	= LinkedEntityIter.GetValue();
		WBEntity* const		pLinkedEntity	= LinkedEntityRef.Get();
		DEVASSERT( pLinkedEntity );

		WBCompRosaSwitchable* const	pSwitchable = WB_GETCOMP( pLinkedEntity, RosaSwitchable );
		if( !pSwitchable )
		{
			continue;
		}

		m_LinkedSwitchables.PushBack( pLinkedEntity );
	}
}

void WBCompRosaSwitch::Switch( WBEntity* const pFrobber ) const
{
	WBEventManager* const pEventManager = GetEventManager();

	FOR_EACH_ARRAY( LinkedSwitchableIter, m_LinkedSwitchables, WBEntityRef )
	{
		const WBEntityRef&	LinkedSwitchableRef	= LinkedSwitchableIter.GetValue();
		WBEntity* const		pLinkedSwitchable	= LinkedSwitchableRef.Get();

		if( !pLinkedSwitchable )
		{
			continue;
		}

		WB_MAKE_EVENT( OnSwitched, pLinkedSwitchable );
		WB_SET_AUTO( OnSwitched, Entity, Frobber, pFrobber );
		WB_DISPATCH_EVENT( pEventManager, OnSwitched, pLinkedSwitchable );
	}
}

#define VERSION_EMPTY				0
#define VERSION_LINKEDSWITCHABLES	1
#define VERSION_CURRENT				1

uint WBCompRosaSwitch::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;													// Version

	Size += 4;													// m_LinkedSwitchables.Size()
	Size += sizeof( WBEntityRef ) * m_LinkedSwitchables.Size();	// m_LinkedSwitchables

	return Size;
}

void WBCompRosaSwitch::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteArray( m_LinkedSwitchables );
}

void WBCompRosaSwitch::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_LINKEDSWITCHABLES )
	{
		DEVASSERT( m_LinkedSwitchables.Empty() );
		Stream.ReadArray( m_LinkedSwitchables );
	}
}
