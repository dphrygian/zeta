#include "core.h"
#include "wbcompstatmod.h"
#include "configmanager.h"
#include "wbevent.h"
#include "reversehash.h"
#include "idatastream.h"

WBCompStatMod::SStatModifier::SStatModifier()
:	m_Active( false )
,	m_Event()
,	m_Stat()
,	m_Function( EMF_None )
,	m_Value( 0.0f )
{
}

WBCompStatMod::WBCompStatMod()
:	m_Serialize( false )
,	m_NonSerializedEvents()
,	m_StatModMap()
,	m_ActiveEvents()
,	m_ActiveEventRefCounts()
{
}

WBCompStatMod::~WBCompStatMod()
{
}

void WBCompStatMod::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( StatMod );

	STATICHASH( Serialize );
	const bool DefaultSerialize = ConfigManager::GetBool( sSerialize, false, sStatMod );
	m_Serialize = ConfigManager::GetInheritedBool( sSerialize, DefaultSerialize, sDefinitionName );

	// Add local stat mods first (order doesn't actually matter; stat mods are put in a multimap
	// so duplicate mods stack instead of overriding each other)
	AddStatMods( DefinitionName );

	// Optionally add statmods from other sets (or other StatMod components, ignoring their other properties)
	STATICHASH( NumStatModSets );
	const uint NumStatModSets = ConfigManager::GetInheritedInt( sNumStatModSets, 0, sDefinitionName );
	FOR_EACH_INDEX( StatModSetIndex, NumStatModSets )
	{
		const SimpleString StatModSet = ConfigManager::GetInheritedSequenceString( "StatModSet%d", StatModSetIndex, "", sDefinitionName );
		AddStatMods( StatModSet );
	}
}

void WBCompStatMod::AddStatMods( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( NumModifiers );
	const uint NumModifiers = ConfigManager::GetInheritedInt( sNumModifiers, 0, sDefinitionName );
	FOR_EACH_INDEX( ModifierIndex, NumModifiers )
	{
		const HashedString Modifier = ConfigManager::GetInheritedSequenceHash( "Modifier%d", ModifierIndex, HashedString::NullString, sDefinitionName );

		SStatModifier NewStatMod;

		STATICHASH( Event );
		NewStatMod.m_Event = ConfigManager::GetHash( sEvent, HashedString::NullString, Modifier );

		STATICHASH( Stat );
		NewStatMod.m_Stat = ConfigManager::GetHash( sStat, HashedString::NullString, Modifier );

		STATICHASH( Function );
		NewStatMod.m_Function = GetModifierFunctionFromString( ConfigManager::GetHash( sFunction, HashedString::NullString, Modifier ) );

		STATICHASH( Value );
		NewStatMod.m_Value = ConfigManager::GetFloat( sValue, 0.0f, Modifier );

		m_StatModMap.Insert( NewStatMod.m_Stat, NewStatMod );
	}

	STATICHASH( NumNonSerializedEvents );
	const uint NumNonSerializedEvents = ConfigManager::GetInheritedInt( sNumNonSerializedEvents, 0, sDefinitionName );
	FOR_EACH_INDEX( NonSerializedEventIndex, NumNonSerializedEvents )
	{
		const HashedString	NonSerializedEvent	= ConfigManager::GetInheritedSequenceHash( "NonSerializedEvent%d", NonSerializedEventIndex, HashedString::NullString, sDefinitionName );
		m_NonSerializedEvents.Insert( NonSerializedEvent );
	}
}

/*static*/ WBCompStatMod::EModifierFunction WBCompStatMod::GetModifierFunctionFromString( const HashedString& Function )
{
	STATIC_HASHED_STRING( Add );
	STATIC_HASHED_STRING( Multiply );
	STATIC_HASHED_STRING( Mul );
	STATIC_HASHED_STRING( Set );

	if( Function == sAdd )							{ return EMF_Add; }
	if( Function == sMultiply || Function == sMul )	{ return EMF_Multiply; }
	if( Function == sSet )							{ return EMF_Set; }

	DEVPRINTF( "Unknown stat mod function %s\n", ReverseHash::ReversedHash( Function ).CStr() );
	DEVWARNDESC( "Unknown stat mod function" );

	return EMF_None;
}

/*virtual*/ void WBCompStatMod::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( TriggerStatMod );
	STATIC_HASHED_STRING( UnTriggerStatMod );
	STATIC_HASHED_STRING( AddRefStatMod );
	STATIC_HASHED_STRING( ReleaseStatMod );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sTriggerStatMod )
	{
		STATIC_HASHED_STRING( StatModEvent );
		const HashedString StatModEvent = Event.GetHash( sStatModEvent );

		TriggerEvent( StatModEvent );
	}
	else if( EventName == sUnTriggerStatMod )
	{
		STATIC_HASHED_STRING( StatModEvent );
		const HashedString StatModEvent = Event.GetHash( sStatModEvent );

		UnTriggerEvent( StatModEvent );
	}
	else if( EventName == sAddRefStatMod )
	{
		STATIC_HASHED_STRING( StatModEvent );
		const HashedString StatModEvent = Event.GetHash( sStatModEvent );

		AddRefEvent( StatModEvent );
	}
	else if( EventName == sReleaseStatMod )
	{
		STATIC_HASHED_STRING( StatModEvent );
		const HashedString StatModEvent = Event.GetHash( sStatModEvent );

		ReleaseEvent( StatModEvent );
	}
}

void WBCompStatMod::TriggerEvent( const HashedString& Event )
{
	// Ignore refcount and force active
	DEVASSERTDESC( m_ActiveEventRefCounts.Search( Event ).IsNull(), "Using TriggerEvent with a refcounted statmod event!" );
	SetEventActive( Event, true );
}

void WBCompStatMod::UnTriggerEvent( const HashedString& Event )
{
	// Ignore refcount and force inactive
	DEVASSERTDESC( m_ActiveEventRefCounts.Search( Event ).IsNull(), "Using UnTriggerEvent with a refcounted statmod event!" );
	SetEventActive( Event, false );
}

void WBCompStatMod::AddRefEvent( const HashedString& Event )
{
	uint& RefCount = m_ActiveEventRefCounts[ Event ];
	if( 0 == RefCount )
	{
		SetEventActive( Event, true );
	}
	++RefCount;
}

void WBCompStatMod::ReleaseEvent( const HashedString& Event )
{
	uint& RefCount = m_ActiveEventRefCounts[ Event ];
	DEVASSERT( RefCount > 0 );
	--RefCount;
	if( 0 == RefCount )
	{
		m_ActiveEventRefCounts.Remove( Event );
		SetEventActive( Event, false );
	}
}

void WBCompStatMod::SetEventActive( const HashedString& Event, bool Active )
{
	PROFILE_FUNCTION;

	// Activate all the modifiers that match this event
	FOR_EACH_MULTIMAP( StatModIter, m_StatModMap, HashedString, SStatModifier )
	{
		SStatModifier& StatModifier = *StatModIter;
		if( StatModifier.m_Event == Event )
		{
			StatModifier.m_Active = Active;
		}
	}

	if( Active )
	{
		m_ActiveEvents.Insert( Event );
	}
	else
	{
		m_ActiveEvents.Remove( Event );
	}
}

float WBCompStatMod::ModifyFloat( const float Value, const HashedString& StatName )
{
	PROFILE_FUNCTION;

	float BaseValue	= Value;
	float AddValue	= 0.0f;
	float MulValue	= 1.0f;

	// Apply all active modifiers that match this stat name
	FOR_EACH_MULTIMAP_SEARCH( StatModIter, m_StatModMap, HashedString, SStatModifier, StatName )
	{
		SStatModifier& StatModifier = *StatModIter;
		if( StatModifier.m_Active )
		{
			if( StatModifier.m_Function == EMF_Add )
			{
				AddValue += StatModifier.m_Value;
			}
			else if( StatModifier.m_Function == EMF_Multiply )
			{
				MulValue *= StatModifier.m_Value;
			}
			else if( StatModifier.m_Function == EMF_Set )
			{
				BaseValue = StatModifier.m_Value;
			}
		}
	}

	// Always apply multipliers first, if any.
	// If needed, provide a sorting mechanism in data.
	return ( BaseValue * MulValue ) + AddValue;
}

/*static*/ const char* WBCompStatMod::GetFunctionString( EModifierFunction Function )
{
	if( Function == EMF_Add )
	{
		return "+";
	}
	else if( Function == EMF_Multiply )
	{
		return "*";
	}
	else if( Function == EMF_Set )
	{
		return "=";
	}
	else
	{
		return "?";
	}
}

#if BUILD_DEV
void WBCompStatMod::Report() const
{
	Super::Report();

	PRINTF( WBPROPERTY_REPORT_PREFIX "Num stat mods: %d\n", m_StatModMap.Size() );
	FOR_EACH_MULTIMAP( StatModIter, m_StatModMap, HashedString, SStatModifier )
	{
		const SStatModifier& StatModifier = StatModIter.GetValue();
		PRINTF( WBPROPERTY_REPORT_PREFIX WB_REPORT_SPACER "%s : %s : %s (%s, %f)\n",
			ReverseHash::ReversedHash( StatModifier.m_Event ).CStr(),
			ReverseHash::ReversedHash( StatModifier.m_Stat ).CStr(),
			StatModifier.m_Active ? "Active" : "Inactive",
			GetFunctionString( StatModifier.m_Function ),
			StatModifier.m_Value );
	}
}
#endif

#define VERSION_EMPTY					0
#define VERSION_ACTIVEEVENTS			1
#define VERSION_ACTIVEEVENTREFCOUNTS	2
#define VERSION_CURRENT					2

uint WBCompStatMod::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;					// Version

	if( !m_Serialize )
	{
		return Size;
	}

	Size += 4;																// NumActiveEvents
	Size += sizeof( HashedString ) * m_ActiveEvents.Size();					// Active events

	Size += 4;																// Num ref counts
	Size += ( sizeof( HashedString ) + 4 ) * m_ActiveEventRefCounts.Size();	// Active event refcounts

	return Size;
}

void WBCompStatMod::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	if( !m_Serialize )
	{
		return;
	}

	// Save everything, even non-serialized events; this way, we can
	// restore them if we change the config NonSerializedEvents.
	Stream.WriteUInt32( m_ActiveEvents.Size() );
	FOR_EACH_SET( ActiveEventIter, m_ActiveEvents, HashedString )
	{
		const HashedString& ActiveEvent = ActiveEventIter.GetValue();
		Stream.WriteHashedString( ActiveEvent );
	}

	// Serializing these separately because historically, nothing has been refcounted,
	// and it's likely that very few stat mod events will need this.
	Stream.WriteUInt32( m_ActiveEventRefCounts.Size() );
	FOR_EACH_MAP( ActiveEventRefCountIter, m_ActiveEventRefCounts, HashedString, uint )
	{
		const HashedString& ActiveEvent = ActiveEventRefCountIter.GetKey();
		const uint			RefCount	= ActiveEventRefCountIter.GetValue();
		Stream.WriteHashedString( ActiveEvent );
		Stream.WriteUInt32( RefCount );
	}
}

void WBCompStatMod::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( !m_Serialize )
	{
		return;
	}

	if( Version >= VERSION_ACTIVEEVENTS )
	{
		const uint NumActiveEvents = Stream.ReadUInt32();
		FOR_EACH_INDEX( ActiveEventIndex, NumActiveEvents )
		{
			const HashedString ActiveEvent = Stream.ReadHashedString();
			if( m_NonSerializedEvents.Contains( ActiveEvent ) )
			{
				// Don't load this! It was only saved as a backup in case the config NonSerializedEvents changes.
			}
			else
			{
				SetEventActive( ActiveEvent, true );
			}
		}
	}

	if( Version >= VERSION_ACTIVEEVENTREFCOUNTS )
	{
		const uint NumActiveEventRefCounts = Stream.ReadUInt32();
		FOR_EACH_INDEX( ActiveEventRefCountIndex, NumActiveEventRefCounts )
		{
			const HashedString	ActiveEvent	= Stream.ReadHashedString();
			const uint			RefCount	= Stream.ReadUInt32();
			DEVASSERT( RefCount > 0 );
			m_ActiveEventRefCounts[ ActiveEvent ] = RefCount;
		}
	}
}
