#include "core.h"
#include "wbeventmanager.h"
#include "iwbeventobserver.h"
#include "clock.h"
#include "idatastream.h"

#if LOG_EVENTS
#include "reversehash.h"
#endif

#if BUILD_DEBUG

#define BEGIN_ITERATING_QUEUE do{ ++m_IteratingQueueRefCount; } while(0)
#define END_ITERATING_QUEUE do{ --m_IteratingQueueRefCount; } while(0)
#define CHECK_ITERATING_QUEUE do{ DEVASSERT( 0 == m_IteratingQueueRefCount ); } while(0)

#define BEGIN_ITERATING_OBSERVERS do{ ++m_IteratingObserversRefCount; } while(0)
#define END_ITERATING_OBSERVERS do{ --m_IteratingObserversRefCount; } while(0)
#define CHECK_ITERATING_OBSERVERS do{ DEVASSERT( 0 == m_IteratingObserversRefCount ); } while(0)

#else

#define BEGIN_ITERATING_QUEUE DoNothing
#define END_ITERATING_QUEUE DoNothing
#define CHECK_ITERATING_QUEUE DoNothing

// DLP 16 Oct 2021: This is no longer just a debug tool!
#define BEGIN_ITERATING_OBSERVERS do{ ++m_IteratingObserversRefCount; } while(0)
#define END_ITERATING_OBSERVERS do{ --m_IteratingObserversRefCount; } while(0)
#define CHECK_ITERATING_OBSERVERS DoNothing

#endif

WBEventManager::WBEventManager()
:	m_EventQueue()
,	m_EventQueueAdd()
,	m_EventObservers()
,	m_PendingEventObservers()
,	m_LastQueueUID( 0 )
,	m_InsideDispatchRefCount( 0 )
,	m_DestroyAfterDispatch( false )
#if BUILD_DEBUG
,	m_IteratingQueueRefCount( 0 )
#endif
// DLP 16 Oct 2021: This is no longer just a debug tool!
,	m_IteratingObserversRefCount( 0 )
{
	m_EventQueue.SetDeflate( false );
	m_EventQueueAdd.SetDeflate( false );
}

WBEventManager::~WBEventManager()
{
}

void WBEventManager::Destroy()
{
	if( m_InsideDispatchRefCount > 0 )
	{
		m_DestroyAfterDispatch = true;
	}
	else
	{
		SafeDeleteNoNull( this );
	}
}

void WBEventManager::DispatchEvent( const WBEvent& Event, IWBEventObserver* const Recipient )
{
	InternalDispatchEvent( Event, Recipient );
}

TEventUID WBEventManager::QueueEvent( const WBEvent& Event, IWBEventObserver* const Recipient, const float DispatchDelay /*= 0.0f*/, const uint DispatchTicks /*= 0*/ )
{
	SQueuedEvent NewEvent;
	NewEvent.m_Event			= Event;
	NewEvent.m_UID				= ++m_LastQueueUID;
	NewEvent.m_Recipient		= Recipient;
	NewEvent.m_DispatchTime		= DispatchDelay + WBWorld::GetInstance()->GetTime();
	NewEvent.m_DispatchTicks	= ( DispatchTicks > 0 ) ? ( DispatchTicks + 1 ) : 0;	// Add 1 because it's meaningless to delay until the very next tick (same as a 0s delay)

	m_EventQueueAdd.PushBack( NewEvent );

	return m_LastQueueUID;
}

void WBEventManager::UnqueueEvent( const TEventUID& EventUID )
{
	for( uint QueuedEventIndex = 0; QueuedEventIndex < m_EventQueue.Size(); ++QueuedEventIndex )
	{
		SQueuedEvent& QueuedEvent = m_EventQueue[ QueuedEventIndex ];
		if( !QueuedEvent.m_Unqueue && QueuedEvent.m_UID == EventUID )
		{
			QueuedEvent.m_Unqueue = true;
		}
	}

	for( uint QueuedEventIndex = 0; QueuedEventIndex < m_EventQueueAdd.Size(); ++QueuedEventIndex )
	{
		SQueuedEvent& QueuedEvent = m_EventQueueAdd[ QueuedEventIndex ];
		if( !QueuedEvent.m_Unqueue && QueuedEvent.m_UID == EventUID )
		{
			QueuedEvent.m_Unqueue = true;
		}
	}
}

void WBEventManager::UnqueueEvents( IWBEventObserver* const Recipient )
{
	for( uint QueuedEventIndex = 0; QueuedEventIndex < m_EventQueue.Size(); ++QueuedEventIndex )
	{
		SQueuedEvent& QueuedEvent = m_EventQueue[ QueuedEventIndex ];
		if( !QueuedEvent.m_Unqueue && QueuedEvent.m_Recipient == Recipient )
		{
			QueuedEvent.m_Unqueue = true;
		}
	}

	for( uint QueuedEventIndex = 0; QueuedEventIndex < m_EventQueueAdd.Size(); ++QueuedEventIndex )
	{
		SQueuedEvent& QueuedEvent = m_EventQueueAdd[ QueuedEventIndex ];
		if( !QueuedEvent.m_Unqueue && QueuedEvent.m_Recipient == Recipient )
		{
			QueuedEvent.m_Unqueue = true;
		}
	}
}

float WBEventManager::GetRemainingTime( const TEventUID& EventUID )
{
	const float CurrentTime = WBWorld::GetInstance()->GetTime();

	for( uint QueuedEventIndex = 0; QueuedEventIndex < m_EventQueue.Size(); ++QueuedEventIndex )
	{
		SQueuedEvent& QueuedEvent = m_EventQueue[ QueuedEventIndex ];
		if( !QueuedEvent.m_Unqueue && QueuedEvent.m_UID == EventUID )
		{
			return QueuedEvent.m_DispatchTime - CurrentTime;
		}
	}

	for( uint QueuedEventIndex = 0; QueuedEventIndex < m_EventQueueAdd.Size(); ++QueuedEventIndex )
	{
		SQueuedEvent& QueuedEvent = m_EventQueueAdd[ QueuedEventIndex ];
		if( !QueuedEvent.m_Unqueue && QueuedEvent.m_UID == EventUID )
		{
			return QueuedEvent.m_DispatchTime - CurrentTime;
		}
	}

	// Event is scheduled for unqueue, or doesn't exist
	return 0.0f;
}

void WBEventManager::Flush()
{
	CHECK_ITERATING_QUEUE;
	m_EventQueue.Clear();
	m_EventQueueAdd.Clear();
}

void WBEventManager::AddObserver( const HashedString& EventName, IWBEventObserver* const Observer, IWBEventObserver* const Recipient /*= NULL*/ )
{
	if( 0 == m_IteratingObserversRefCount )
	{
		CHECK_ITERATING_OBSERVERS;

		DEVASSERT( EventName );
		DEVASSERT( Observer );

		SObserver ObserverEntry;
		ObserverEntry.m_Observer = Observer;
		ObserverEntry.m_Recipient = Recipient;

		// Make sure this observer doesn't already exist
		DEBUGASSERT( m_EventObservers.Search( EventName, ObserverEntry ).IsNull() );

		m_EventObservers.Insert( EventName, ObserverEntry );
	}
	else
	{
		// CHECK_ITERATING_OBSERVERS would fail, so wait to add this observer until we're done iterating.
		// See after END_ITERATING_OBSERVERS in WBEventManager::InternalDispatchEvent.
		// This case occurs when spawning items while handling an observed event like PostLevelTransition, for example.
		// It used to be masked by not refcounting the iterating observers variable; fixing that exposed the real problem.
		// Is it a real problem? Does modifying m_EventObservers while iterating actually might? I don't know, but it MIGHT.

		SPendingObserver& PendingObserver = m_PendingEventObservers.PushBack();
		PendingObserver.m_EventName	= EventName;
		PendingObserver.m_Observer	= Observer;
		PendingObserver.m_Recipient	= Recipient;
		PendingObserver.m_Remove	= false;
	}
}

void WBEventManager::RemoveObserver( const HashedString& EventName, IWBEventObserver* const Observer, IWBEventObserver* const Recipient /*= NULL*/ )
{
	if( 0 == m_IteratingObserversRefCount )
	{
		CHECK_ITERATING_OBSERVERS;

		DEVASSERT( EventName );
		DEVASSERT( Observer );

		SObserver ObserverEntry;
		ObserverEntry.m_Observer = Observer;
		ObserverEntry.m_Recipient = Recipient;

		// Make sure this observer exists
		DEBUGASSERT( m_EventObservers.Search( EventName, ObserverEntry ).IsValid() );

		m_EventObservers.Remove( EventName, ObserverEntry );
	}
	else
	{
		// This case occurs when destroying AIs while handling an observed event like ForceAIRespawn, for example.

		SPendingObserver& PendingObserver = m_PendingEventObservers.PushBack();
		PendingObserver.m_EventName	= EventName;
		PendingObserver.m_Observer	= Observer;
		PendingObserver.m_Recipient	= Recipient;
		PendingObserver.m_Remove	= true;
	}
}

// For debug builds, this removes the observer without the validation check that it actually *was* an observer
void WBEventManager::UncheckedRemoveObserver( const HashedString& EventName, IWBEventObserver* const Observer, IWBEventObserver* const Recipient /*= NULL*/ )
{
	CHECK_ITERATING_OBSERVERS;

	DEVASSERT( EventName );
	DEVASSERT( Observer );

	SObserver ObserverEntry;
	ObserverEntry.m_Observer = Observer;
	ObserverEntry.m_Recipient = Recipient;

	m_EventObservers.Remove( EventName, ObserverEntry );
}

void WBEventManager::PushAddQueueEvents()
{
	XTRACE_FUNCTION;

	// Array::Append doesn't work because it does a shallow copy.
	FOR_EACH_ARRAY( AddEventIter, m_EventQueueAdd, SQueuedEvent )
	{
		const SQueuedEvent& Event = AddEventIter.GetValue();
		if( Event.m_Unqueue )
		{
			// Ignore
		}
		else
		{
			CHECK_ITERATING_QUEUE;
			m_EventQueue.PushBack( Event );
		}
	}

	m_EventQueueAdd.Clear();
}

void WBEventManager::Tick()
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	const float CurrentTime = WBWorld::GetInstance()->GetTime();

	// Make sure we have all new queued events before ticking.
	PushAddQueueEvents();

	// Dispatch events whose delay has elapsed.
	XTRACE_BEGIN( DispatchEvents );
		BEGIN_ITERATING_QUEUE;
		for( uint QueuedEventIndex = 0; QueuedEventIndex < m_EventQueue.Size(); ++QueuedEventIndex )
		{
			SQueuedEvent& QueuedEvent = m_EventQueue[ QueuedEventIndex ];
			if( QueuedEvent.m_Unqueue )
			{
				continue;
			}

			if( ( QueuedEvent.m_DispatchTicks > 0	&& --QueuedEvent.m_DispatchTicks == 0 ) ||
				( QueuedEvent.m_DispatchTicks == 0	&& QueuedEvent.m_DispatchTime <= CurrentTime ) )
			{
				QueuedEvent.m_Unqueue = true;
				InternalDispatchEvent( QueuedEvent.m_Event, QueuedEvent.m_Recipient );
			}
		}
		END_ITERATING_QUEUE;
	XTRACE_END;

	// Remove dispatched or otherwise unqueued events.
	XTRACE_BEGIN( RemoveEvents );
		for( int QueuedEventIndex = m_EventQueue.Size() - 1; QueuedEventIndex >= 0; --QueuedEventIndex )
		{
			const SQueuedEvent& QueuedEvent = m_EventQueue[ QueuedEventIndex ];
			if( QueuedEvent.m_Unqueue )
			{
				// Slow remove to maintain order.
				m_EventQueue.Remove( QueuedEventIndex );
			}
		}
	XTRACE_END;

	// And make sure we have all new queued events after ticking.
	PushAddQueueEvents();
}

void WBEventManager::InternalDispatchEvent( const WBEvent& Event, IWBEventObserver* const Recipient )
{
	XTRACE_FUNCTION;

	++m_InsideDispatchRefCount;

#if LOG_EVENTS
	STATIC_HASHED_STRING( WBEvent_LogEvent );
	const bool LogEvent = Event.GetBool( sWBEvent_LogEvent );
	if( LogEvent )
	{
		const SimpleString		EventName		= ReverseHash::ReversedHash( Event.GetEventName() );

		STATIC_HASHED_STRING( EventOwner );
		const WBEntity* const	pEventOwner		= Event.GetEntity( sEventOwner );
		const SimpleString		EventOwnerName	= pEventOwner ? pEventOwner->GetUniqueName() : "None";

		const WBEntityRef		RecipientEntity	= Recipient ? Recipient->GetEntityUID() : 0;
		const WBEntity* const	pRecipient		= RecipientEntity.Get();
		const SimpleString		RecipientName	= pRecipient ? pRecipient->GetUniqueName() : "None";

		const float				CurrentTime		= WBWorld::GetInstance()->GetTime();

		if( EventOwnerName == RecipientName )
		{
			PRINTF( "WBE %.2f: %s Own: %s\n", CurrentTime, EventName.CStr(), EventOwnerName.CStr() );
		}
		else
		{
			PRINTF( "WBE %.2f: %s Own: %s Rec: %s\n", CurrentTime, EventName.CStr(), EventOwnerName.CStr(), RecipientName.CStr() );
		}
	}
#endif

	if( Recipient )
	{
		Recipient->HandleEvent( Event );
	}

	BEGIN_ITERATING_OBSERVERS;
	FOR_EACH_MULTIMAP_SEARCH( ObserverIter, m_EventObservers, HashedString, SObserver, Event.GetEventName() )
	{
		const SObserver& ObserverEntry = ObserverIter.GetValue();

		IWBEventObserver* const Observer = ObserverEntry.m_Observer;
		ASSERT( Observer );
		if( Observer == Recipient )
		{
			// Don't let recipient handle it twice if it's registered as an observer.
			continue;
		}

		IWBEventObserver* const ObserverRecipient = ObserverEntry.m_Recipient;
		if( ObserverRecipient && ObserverRecipient != Recipient )
		{
			// Ignore if observer isn't listening for this recipient (and isn't a global observer).
			continue;
		}

		Observer->HandleEvent( Event );
	}
	END_ITERATING_OBSERVERS;

	// Latently add pending observers if we're done iterating.
	if( 0 == m_IteratingObserversRefCount )
	{
		FOR_EACH_ARRAY( PendingObserverIter, m_PendingEventObservers, SPendingObserver )
		{
			const SPendingObserver& PendingObserver = PendingObserverIter.GetValue();
			if( PendingObserver.m_Remove )
			{
				RemoveObserver( PendingObserver.m_EventName, PendingObserver.m_Observer, PendingObserver.m_Recipient );
			}
			else
			{
				AddObserver( PendingObserver.m_EventName, PendingObserver.m_Observer, PendingObserver.m_Recipient );
			}
		}
		m_PendingEventObservers.Clear();
	}

	--m_InsideDispatchRefCount;
	if( m_DestroyAfterDispatch && 0 == m_InsideDispatchRefCount  )
	{
		SafeDeleteNoNull( this );
	}
}

bool WBEventManager::HasQueuedEvents( IWBEventObserver* const Recipient ) const
{
	FOR_EACH_ARRAY( QueuedEventIter, m_EventQueue, SQueuedEvent )
	{
		const SQueuedEvent& QueuedEvent = QueuedEventIter.GetValue();
		if( !QueuedEvent.m_Unqueue && QueuedEvent.m_Recipient == Recipient )
		{
			return true;
		}
	}

	FOR_EACH_ARRAY( QueuedEventIter, m_EventQueueAdd, SQueuedEvent )
	{
		const SQueuedEvent& QueuedEvent = QueuedEventIter.GetValue();
		if( !QueuedEvent.m_Unqueue && QueuedEvent.m_Recipient == Recipient )
		{
			return true;
		}
	}

	return false;
}

// Observers are not serialized. Registered observers will remain registered, and new observers should reregister.

#define VERSION_EMPTY		0
#define VERSION_UIDS		1
#define VERSION_EVENTUIDS	2
#define VERSION_TICKS		3
#define VERSION_CURRENT		3

void WBEventManager::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const float CurrentTime = WBWorld::GetInstance()->GetTime();

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_UIDS )
	{
		m_LastQueueUID = Stream.ReadUInt32();
	}

	const uint NumQueuedEvents = Stream.ReadUInt32();
	for( uint QueuedEventIndex = 0; QueuedEventIndex < NumQueuedEvents; ++QueuedEventIndex )
	{
		const bool Unqueued = Stream.ReadBool();
		if( Unqueued )
		{
			continue;
		}

		const float	RemainingTime	= Stream.ReadFloat();
		const uint	DispatchTicks	= ( Version >= VERSION_TICKS ) ? Stream.ReadUInt32() : 0;

		const uint PackedEventSize = Stream.ReadUInt32();
		WBPackedEvent PackedEvent;
		PackedEvent.Reinit( NULL, PackedEventSize );
		Stream.Read( PackedEventSize, PackedEvent.GetData() );

		// NOTE: Entities are the only recipients that can be serialized currently.
		const uint EntityUID = Stream.ReadUInt32();
		IWBEventObserver* pRecipient = WBWorld::GetInstance()->GetEntity( EntityUID );

		const TEventUID EventUID = ( Version >= VERSION_EVENTUIDS ) ? Stream.ReadUInt32() : 0;

		SQueuedEvent NewEvent;
		NewEvent.m_Event.Unpack( PackedEvent );
		NewEvent.m_Recipient		= pRecipient;
		NewEvent.m_DispatchTime		= RemainingTime + CurrentTime;
		NewEvent.m_DispatchTicks	= DispatchTicks;
		NewEvent.m_UID				= EventUID;

		m_EventQueue.PushBack( NewEvent );
	}
}

void WBEventManager::Save( const IDataStream& Stream ) const
{
	XTRACE_FUNCTION;

	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteUInt32( m_LastQueueUID );

	const uint TotalQueuedEvents = m_EventQueue.Size() + m_EventQueueAdd.Size();
	Stream.WriteUInt32( TotalQueuedEvents );

	for( uint QueuedEventIndex = 0; QueuedEventIndex < m_EventQueue.Size(); ++QueuedEventIndex )
	{
		const SQueuedEvent& QueuedEvent = m_EventQueue[ QueuedEventIndex ];
		SaveQueuedEvent( QueuedEvent, Stream );
	}

	for( uint QueuedEventIndex = 0; QueuedEventIndex < m_EventQueueAdd.Size(); ++QueuedEventIndex )
	{
		const SQueuedEvent& QueuedEvent = m_EventQueueAdd[ QueuedEventIndex ];
		SaveQueuedEvent( QueuedEvent, Stream );
	}
}

void WBEventManager::SaveQueuedEvent( const SQueuedEvent& QueuedEvent, const IDataStream& Stream ) const
{
	Stream.WriteBool( QueuedEvent.m_Unqueue );
	if( QueuedEvent.m_Unqueue )
	{
		return;
	}

	const float	CurrentTime		= WBWorld::GetInstance()->GetTime();
	const float	RemainingTime	= QueuedEvent.m_DispatchTime - CurrentTime;
	Stream.WriteFloat( RemainingTime );
	Stream.WriteUInt32( QueuedEvent.m_DispatchTicks );

	WBPackedEvent PackedEvent;
	QueuedEvent.m_Event.Pack( PackedEvent );

	Stream.WriteUInt32( PackedEvent.GetSize() );
	Stream.Write( PackedEvent.GetSize(), PackedEvent.GetData() );

#if BUILD_DEV
	if( QueuedEvent.m_Recipient && QueuedEvent.m_Recipient->GetEntityUID() == 0 )
	{
		// Events serialized without a valid recipient will not be restored properly.
		// Route through an entity if possible (e.g., target RosaGame through WBCompRosaPlayer).
		// (This is usually a RodinBTNodeWait.)
		PRINTF( "Queued event %s being serialized with no UID for recipient.\n", QueuedEvent.m_Event.GetEventNameString().CStr() );
	}
#endif

	Stream.WriteUInt32( QueuedEvent.m_Recipient ? QueuedEvent.m_Recipient->GetEntityUID() : 0 );
	Stream.WriteUInt32( QueuedEvent.m_UID );
}
