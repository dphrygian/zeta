#include "core.h"
#include "wbcomprosaitem.h"
#include "configmanager.h"
#include "idatastream.h"

WBCompRosaItem::WBCompRosaItem()
:	m_Slot()
,	m_Persistent( false )
,	m_KeepHands( false )
{
}

WBCompRosaItem::~WBCompRosaItem()
{
}

/*virtual*/ void WBCompRosaItem::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Slot );
	m_Slot = ConfigManager::GetInheritedHash( sSlot, HashedString::NullString, sDefinitionName );

	STATICHASH( Persistent );
	m_Persistent = ConfigManager::GetInheritedBool( sPersistent, true, sDefinitionName );

	STATICHASH( KeepHands );
	m_KeepHands = ConfigManager::GetInheritedBool( sKeepHands, false, sDefinitionName );
}

#define VERSION_EMPTY	0
#define VERSION_SLOT	1
#define VERSION_CURRENT	1

uint WBCompRosaItem::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;						// Version
	Size += sizeof( HashedString );	// m_Slot

	return Size;
}

void WBCompRosaItem::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );
	Stream.WriteHashedString( m_Slot );
}

void WBCompRosaItem::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_SLOT )
	{
		m_Slot = Stream.ReadHashedString();
	}
}
