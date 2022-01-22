#include "core.h"
#include "wbcomprosalinkedentities.h"
#include "idatastream.h"
#include "wbevent.h"

WBCompRosaLinkedEntities::WBCompRosaLinkedEntities()
:	m_LinkedEntities()
{
}

WBCompRosaLinkedEntities::~WBCompRosaLinkedEntities()
{
}

/*virtual*/ void WBCompRosaLinkedEntities::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( SetLinkedEntities );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sSetLinkedEntities )
	{
		STATIC_HASHED_STRING( LinkedEntities );
		void* pVoid = Event.GetPointer( sLinkedEntities );
		const Array<WBEntityRef>* const pLinkedEntities = static_cast<const Array<WBEntityRef>*>( pVoid );
		DEVASSERT( pLinkedEntities );

		m_LinkedEntities = *pLinkedEntities;
	}
}

#define VERSION_EMPTY			0
#define VERSION_LINKEDENTITIES	1
#define VERSION_CURRENT			1

uint WBCompRosaLinkedEntities::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version

	Size += 4;	// m_LinkedEntities.Size()
	Size += sizeof( WBEntityRef ) * m_LinkedEntities.Size();	// m_LinkedEntities

	return Size;
}

void WBCompRosaLinkedEntities::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteArray( m_LinkedEntities );
}

void WBCompRosaLinkedEntities::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_LINKEDENTITIES )
	{
		Stream.ReadArray( m_LinkedEntities );
	}
}
