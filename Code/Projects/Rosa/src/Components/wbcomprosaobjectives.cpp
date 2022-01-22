#include "core.h"
#include "wbcomprosaobjectives.h"
#include "configmanager.h"
#include "idatastream.h"
#include "wbeventmanager.h"
#include "rosahudlog.h"
#include "rosagame.h"
#include "simplestring.h"

WBCompRosaObjectives::WBCompRosaObjectives()
:	m_Objectives()
,	m_Persist( false )
{
}

WBCompRosaObjectives::~WBCompRosaObjectives()
{
}

/*virtual*/ void WBCompRosaObjectives::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Persist );
	m_Persist = ConfigManager::GetInheritedBool( sPersist, false, sDefinitionName );
}

/*virtual*/ void WBCompRosaObjectives::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnInitialized );
	STATIC_HASHED_STRING( ClearObjectives );
	STATIC_HASHED_STRING( AddObjective );
	STATIC_HASHED_STRING( CompleteObjective );
	STATIC_HASHED_STRING( RepublishObjectives );
	STATIC_HASHED_STRING( PushPersistence );
	STATIC_HASHED_STRING( PullPersistence );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnInitialized )
	{
		PublishToHUD();
	}
	else if( EventName == sClearObjectives )
	{
		ClearObjectives();
	}
	else if( EventName == sAddObjective )
	{
		STATIC_HASHED_STRING( ObjectiveTag );
		const HashedString ObjectiveTag = Event.GetHash( sObjectiveTag );

		AddObjective( ObjectiveTag );
	}
	else if( EventName == sCompleteObjective )
	{
		STATIC_HASHED_STRING( ObjectiveTag );
		const HashedString ObjectiveTag = Event.GetHash( sObjectiveTag );

		STATIC_HASHED_STRING( Fail );
		const bool Fail = Event.GetBool( sFail );

		STATIC_HASHED_STRING( ForceAdd );
		const bool ForceAdd = Event.GetBool( sForceAdd );

		CompleteObjective( ObjectiveTag, Fail, ForceAdd );
	}
	else if( EventName == sRepublishObjectives )
	{
		PublishToHUD();
	}
	else if( EventName == sPushPersistence )
	{
		PushPersistence();
	}
	else if( EventName == sPullPersistence )
	{
		PullPersistence();
	}
}

void WBCompRosaObjectives::ClearObjectives()
{
	m_Objectives.Clear();

	PublishToHUD();
}

uint WBCompRosaObjectives::AddObjective( const HashedString& ObjectiveTag )
{
	uint ObjectiveIndex = m_Objectives.Size();
	if( ObjectiveExists( ObjectiveTag, ObjectiveIndex ) )
	{
		return ObjectiveIndex;
	}

	SObjective& NewObjective		= m_Objectives.PushBack();
	NewObjective.m_ObjectiveTag		= ObjectiveTag;
	NewObjective.m_ObjectiveStatus	= EOS_Incomplete;

	PublishToHUD();

	STATICHASH( NewObjective );
	GetGame()->GetHUDLog()->AddEntry( ConfigManager::GetLocalizedString( sNewObjective, "" ) );

	return ObjectiveIndex;
}

void WBCompRosaObjectives::CompleteObjective( const HashedString& ObjectiveTag, const bool Fail, const bool ForceAdd )
{
	uint ObjectiveIndex;
	if( ForceAdd )
	{
		// Try to add the objective first, which also gets the index if it already existed
		ObjectiveIndex = AddObjective( ObjectiveTag );
	}
	else if( !ObjectiveExists( ObjectiveTag, ObjectiveIndex ) )
	{
		// The objective doesn't exist and we're not force-adding it
		return;
	}

	SObjective&	Objective = m_Objectives[ ObjectiveIndex ];
	if( Objective.m_ObjectiveStatus != EOS_Incomplete )
	{
		// We've already completed this objective
		return;
	}

	Objective.m_ObjectiveStatus	= Fail ? EOS_Failed : EOS_Succeeded;

	if( Fail )
	{
		STATICHASH( ObjectiveFailed );
		GetGame()->GetHUDLog()->AddEntry( ConfigManager::GetLocalizedString( sObjectiveFailed, "" ) );
	}
	else
	{
		// Play Sound_ObjectiveComplete
		STATIC_HASHED_STRING( Sound_ObjectiveComplete );
		WB_MAKE_EVENT( PlaySoundDef, GetEntity() );
		WB_SET_AUTO( PlaySoundDef, Hash, Sound, sSound_ObjectiveComplete );
		WB_DISPATCH_EVENT( GetEventManager(), PlaySoundDef, GetEntity() );

		STATICHASH( ObjectiveSucceeded );
		GetGame()->GetHUDLog()->AddEntry( ConfigManager::GetLocalizedString( sObjectiveSucceeded, "" ) );
	}

	PublishToHUD();

	// Send events, mainly for achievement implementation
	WB_MAKE_EVENT( OnObjectiveComplete, GetEntity() );
	WB_SET_AUTO( OnObjectiveComplete, Hash, ObjectiveTag, ObjectiveTag );
	WB_SET_AUTO( OnObjectiveComplete, Bool, Failed, Fail );
	WB_DISPATCH_EVENT( GetEventManager(), OnObjectiveComplete, GetEntity() );
}

bool WBCompRosaObjectives::ObjectiveExists( const HashedString& ObjectiveTag, uint& OutIndex ) const
{
	FOR_EACH_ARRAY( ObjectiveIter, m_Objectives, SObjective )
	{
		const SObjective& Objective = ObjectiveIter.GetValue();
		if( Objective.m_ObjectiveTag == ObjectiveTag )
		{
			OutIndex = ObjectiveIter.GetIndex();
			return true;
		}
	}

	return false;
}

bool WBCompRosaObjectives::IsObjectiveComplete( const HashedString& ObjectiveTag, const bool RejectFail ) const
{
	uint ObjectiveIndex;
	if( !ObjectiveExists( ObjectiveTag, ObjectiveIndex ) )
	{
		return false;
	}

	const SObjective&	Objective			= m_Objectives[ ObjectiveIndex ];
	const bool			ObjectiveSucceeded	= Objective.m_ObjectiveStatus == EOS_Succeeded;
	const bool			ObjectiveFailed		= Objective.m_ObjectiveStatus == EOS_Failed;
	const bool			ObjectiveCompleted	= ObjectiveSucceeded || ( ObjectiveFailed && !RejectFail );

	return ObjectiveCompleted;
}

void WBCompRosaObjectives::PublishToHUD() const
{
	SimpleString LeftAlignedString;
	SimpleString RightAlignedString;

	STATICHASH( ObjIncomplete );
	const SimpleString ObjIncompleteString = ConfigManager::GetLocalizedString( sObjIncomplete, "" );

	STATICHASH( ObjSucceeded );
	const SimpleString ObjSucceededString = ConfigManager::GetLocalizedString( sObjSucceeded, "" );

	STATICHASH( ObjFailed );
	const SimpleString ObjFailedString = ConfigManager::GetLocalizedString( sObjFailed, "" );

	FOR_EACH_ARRAY( ObjectiveIter, m_Objectives, SObjective )
	{
		const SObjective& Objective	= ObjectiveIter.GetValue();
		if( Objective.m_ObjectiveStatus == EOS_None )
		{
			WARN;
			continue;
		}

		const SimpleString ObjectiveString = ConfigManager::GetLocalizedString( Objective.m_ObjectiveTag, "" );

		// HACKHACK: Content-aware glyph codepoints
		SimpleString StatusString;
		if(		 Objective.m_ObjectiveStatus == EOS_Incomplete )	{ StatusString = ObjIncompleteString; }
		else if( Objective.m_ObjectiveStatus == EOS_Succeeded )		{ StatusString = ObjSucceededString; }
		else if( Objective.m_ObjectiveStatus == EOS_Failed )		{ StatusString = ObjFailedString; }

		LeftAlignedString	+= SimpleString::PrintF( "%s %s\n", StatusString.CStr(), ObjectiveString.CStr() );
		RightAlignedString	+= SimpleString::PrintF( "%s %s\n", ObjectiveString.CStr(), StatusString.CStr() );
	}

	STATICHASH( RosaObjectives );
	STATICHASH( ObjectivesLeft );
	ConfigManager::SetString( sObjectivesLeft, LeftAlignedString.CStr(), sRosaObjectives );

	STATICHASH( ObjectivesRight );
	ConfigManager::SetString( sObjectivesRight, RightAlignedString.CStr(), sRosaObjectives );
}

void WBCompRosaObjectives::PushPersistence() const
{
	if( !m_Persist )
	{
		return;
	}

	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	STATIC_HASHED_STRING( NumObjectives );
	Persistence.SetInt( sNumObjectives, m_Objectives.Size() );

	FOR_EACH_ARRAY( ObjectiveIter, m_Objectives, SObjective )
	{
		const SObjective&	Objective		= ObjectiveIter.GetValue();
		const uint			ObjectiveIndex	= ObjectiveIter.GetIndex();

		Persistence.SetHash( SimpleString::PrintF( "Objective%dTag", ObjectiveIndex ), Objective.m_ObjectiveTag );
		Persistence.SetInt( SimpleString::PrintF( "Objective%dStatus", ObjectiveIndex ), Objective.m_ObjectiveStatus );
	}
}

void WBCompRosaObjectives::PullPersistence()
{
	if( !m_Persist )
	{
		return;
	}

	// HACKHACK: If we have objectives already, don't pull persistence;
	// this means that a script probably pushed new objectives for a level
	// and we don't want to import the persistent objectives.
	if( m_Objectives.Size() > 0 )
	{
		return;
	}

	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	STATIC_HASHED_STRING( NumObjectives );
	const uint NumObjectives = Persistence.GetInt( sNumObjectives );

	m_Objectives.Resize( NumObjectives );
	FOR_EACH_ARRAY( ObjectiveIter, m_Objectives, SObjective )
	{
		SObjective&	Objective		= ObjectiveIter.GetValue();
		const uint	ObjectiveIndex	= ObjectiveIter.GetIndex();

		Objective.m_ObjectiveTag	= Persistence.GetHash( SimpleString::PrintF( "Objective%dTag", ObjectiveIndex ) );
		Objective.m_ObjectiveStatus	= static_cast<EObjectiveStatus>( Persistence.GetInt( SimpleString::PrintF( "Objective%dStatus", ObjectiveIndex ) ) );
	}

	PublishToHUD();
}

#define VERSION_EMPTY		0
#define VERSION_OBJECTIVES	1
#define VERSION_CURRENT		1

uint WBCompRosaObjectives::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version

	Size += 4;	// m_Objectives.Size()
	Size += sizeof( SObjective ) * m_Objectives.Size();

	return Size;
}

void WBCompRosaObjectives::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteUInt32( m_Objectives.Size() );
	Stream.Write( m_Objectives.MemorySize(), m_Objectives.GetData() );
}

void WBCompRosaObjectives::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_OBJECTIVES )
	{
		const uint NumObjectives = Stream.ReadUInt32();
		m_Objectives.Resize( NumObjectives );
		Stream.Read( m_Objectives.MemorySize(), m_Objectives.GetData() );
	}
}
