#include "core.h"
#include "rosasaveload.h"
#include "fileutil.h"
#include "zlib.h"
#include "filestream.h"
#include "dynamicmemorystream.h"
#include "rosapersistence.h"
#include "rosacampaign.h"
#include "rosaworld.h"
#include "rosaframework.h"
#include "datapipe.h"
#include "memorystream.h"
#include "wbeventmanager.h"
#include "configmanager.h"
#include "timedate.h"
#include "Achievements/iachievementmanager.h"

// ROSANOTE: Incrementing this will invalidate all prior saves.
// This version *must* match the version in the rosasaveindex.txt file,
// or all saved games will be ignored. Increment this when breaking
// changes have to be made, so old saves can't be loaded.
#define ROSA_SAVE_VERSION 2

RosaSaveLoad::RosaSaveLoad()
:	m_WorldFiles()
,	m_PendingSaveSlot()
#if ROSA_USE_ACTIVESAVESLOT
,	m_ActiveSaveSlot()
#endif
{
}

RosaSaveLoad::~RosaSaveLoad()
{
}

/*static*/ SimpleString RosaSaveLoad::GetRawSaveIndexFile()
{
	return SimpleString( "rosasaveindex.txt" );
}

/*static*/ SimpleString RosaSaveLoad::GetSaveIndexFile()
{
	return RosaFramework::GetInstance()->GetSaveLoadPath() + GetRawSaveIndexFile();
}

/*static*/ SimpleString RosaSaveLoad::GetMRUSaveSlot()
{
	STATICHASH( RosaSaveIndex );
	STATICHASH( MRUSaveSlot );
	return ConfigManager::GetString( sMRUSaveSlot, "", sRosaSaveIndex );
}

/*static*/ void RosaSaveLoad::SetMRUSaveSlot( const SimpleString& MRUSaveSlot )
{
	STATICHASH( RosaSaveIndex );
	STATICHASH( MRUSaveSlot );
	ConfigManager::SetString( sMRUSaveSlot, MRUSaveSlot.CStr(), sRosaSaveIndex );
}

/*static*/ void RosaSaveLoad::UpdateSaveSlot( const SimpleString& SlotName )
{
	MAKEHASH( SlotName );

	RosaFramework* const	pFramework	= RosaFramework::GetInstance();
	DEVASSERT( pFramework );

	RosaGame* const			pGame		= pFramework->GetGame();
	DEVASSERT( pGame );

	// OLDVAMP
	//RosaCampaign* const		pCampaign	= pGame->GetCampaign();
	//DEVASSERT( pCampaign );

	const timedate_tm_t CurrentLocalTime = TimeDate::GetLocalTime();

	STATICHASH( Version );
	ConfigManager::SetInt( sVersion, ROSA_SAVE_VERSION, sSlotName );

	STATICHASH( LevelName );
	ConfigManager::SetString( sLevelName, pGame->GetCurrentFriendlyLevelName().CStr(), sSlotName );

	// OLDVAMP
	//STATICHASH( Legacy );
	//ConfigManager::SetInt( sLegacy, pCampaign->GetLegacy(), sSlotName );

	// OLDVAMP
	//STATICHASH( Season );
	//ConfigManager::SetInt( sSeason, pCampaign->GetSeason(), sSlotName );

	// OLDVAMP
	//STATICHASH( Episode );
	//ConfigManager::SetInt( sEpisode, pCampaign->GetEpisode(), sSlotName );

	STATICHASH( DateY );
	ConfigManager::SetInt( sDateY, TimeDate::GetYear( CurrentLocalTime ), sSlotName );

	STATICHASH( DateM );
	ConfigManager::SetInt( sDateM, TimeDate::GetMonth( CurrentLocalTime ) + 1, sSlotName );

	STATICHASH( DateD );
	ConfigManager::SetInt( sDateD, TimeDate::GetDay( CurrentLocalTime ), sSlotName );

	STATICHASH( TimeH );
	ConfigManager::SetInt( sTimeH, TimeDate::GetHours24( CurrentLocalTime ), sSlotName );

	STATICHASH( TimeM );
	ConfigManager::SetInt( sTimeM, TimeDate::GetMinutes( CurrentLocalTime ), sSlotName );

	STATICHASH( TimeS );
	ConfigManager::SetInt( sTimeS, TimeDate::GetSeconds( CurrentLocalTime ), sSlotName );
}

/*static*/ void RosaSaveLoad::MoveSaveSlot( const SimpleString& OldSlotName, const SimpleString& NewSlotName )
{
	MAKEHASH( OldSlotName );
	MAKEHASH( NewSlotName );

	STATICHASH( Version );
	ConfigManager::SetInt( sVersion, ConfigManager::GetInt( sVersion, 0, sOldSlotName ), sNewSlotName );

	STATICHASH( LevelName );
	ConfigManager::SetString( sLevelName, ConfigManager::GetString( sLevelName, "", sOldSlotName ), sNewSlotName );

	STATICHASH( Legacy );
	ConfigManager::SetInt( sLegacy, ConfigManager::GetInt( sLegacy, 0, sOldSlotName ), sNewSlotName );

	STATICHASH( Season );
	ConfigManager::SetInt( sSeason, ConfigManager::GetInt( sSeason, 0, sOldSlotName ), sNewSlotName );

	STATICHASH( Episode );
	ConfigManager::SetInt( sEpisode, ConfigManager::GetInt( sEpisode, 0, sOldSlotName ), sNewSlotName );

	STATICHASH( DateY );
	ConfigManager::SetInt( sDateY, ConfigManager::GetInt( sDateY, 0, sOldSlotName ), sNewSlotName );

	STATICHASH( DateM );
	ConfigManager::SetInt( sDateM, ConfigManager::GetInt( sDateM, 0, sOldSlotName ), sNewSlotName );

	STATICHASH( DateD );
	ConfigManager::SetInt( sDateD, ConfigManager::GetInt( sDateD, 0, sOldSlotName ), sNewSlotName );

	STATICHASH( TimeH );
	ConfigManager::SetInt( sTimeH, ConfigManager::GetInt( sTimeH, 0, sOldSlotName ), sNewSlotName );

	STATICHASH( TimeM );
	ConfigManager::SetInt( sTimeM, ConfigManager::GetInt( sTimeM, 0, sOldSlotName ), sNewSlotName );

	STATICHASH( TimeS );
	ConfigManager::SetInt( sTimeS, ConfigManager::GetInt( sTimeS, 0, sOldSlotName ), sNewSlotName );
}

/*static*/ SimpleString RosaSaveLoad::GetRawSlotSaveFile( const SimpleString& SlotName )
{
	if( SlotName == "MRU" )
	{
		// HACKHACK: If SlotName is "MRU", return the most recently used slot
		return GetRawSlotSaveFile( GetMRUSaveSlot() );
	}
	else
	{
		return SlotName.ToLower() + ".rosasave";
	}
}

/*static*/ void RosaSaveLoad::GetSaveSlotInfos( Array<SSaveSlotInfo>& OutSaveSlotInfos, const bool FilterQuickAndAutosaves )
{
	STATICHASH( NumAutosaves );
	const uint NumAutosaves = ConfigManager::GetInt( sNumAutosaves );

	STATICHASH( NumSaves );
	const uint NumSaves = ConfigManager::GetInt( sNumSaves );

	if( !FilterQuickAndAutosaves )
	{
		{
			SSaveSlotInfo& QuicksaveSlotInfo	= OutSaveSlotInfos.PushBack();
			QuicksaveSlotInfo.m_Type			= ESST_Quicksave;
			GetSaveSlotInfo( "Quicksave", QuicksaveSlotInfo );
		}

		for( uint AutosaveIndex = 0; AutosaveIndex < NumAutosaves; ++AutosaveIndex )
		{
			SSaveSlotInfo& AutosaveSlotInfo	= OutSaveSlotInfos.PushBack();
			AutosaveSlotInfo.m_Type			= ESST_Autosave;
			AutosaveSlotInfo.m_TypeIndex	= AutosaveIndex;
			GetSaveSlotInfo( SimpleString::PrintF( "Autosave%d", AutosaveIndex ), AutosaveSlotInfo );
		}
	}

	for( uint SaveIndex = 0; SaveIndex < NumSaves; ++SaveIndex )
	{
		SSaveSlotInfo& SaveSlotInfo	= OutSaveSlotInfos.PushBack();
		SaveSlotInfo.m_Type			= ESST_Save;
		SaveSlotInfo.m_TypeIndex	= SaveIndex;
		GetSaveSlotInfo( SimpleString::PrintF( "Save%d", SaveIndex ), SaveSlotInfo );
	}
}

/*static*/ void RosaSaveLoad::GetSaveSlotInfo( const SimpleString& Slot, SSaveSlotInfo& OutSaveSlotInfo )
{
	if( Slot == "MRU" )
	{
		// HACKHACK: If SlotName is "MRU", redirect to the most recently used slot
		GetSaveSlotInfo( GetMRUSaveSlot(), OutSaveSlotInfo );
		return;
	}

	ASSERT( Slot != "" );

	MAKEHASH( Slot );

	OutSaveSlotInfo.m_SlotName = Slot;

	STATICHASH( Version );
	OutSaveSlotInfo.m_Version = ConfigManager::GetInt( sVersion, 0, sSlot );

	const SimpleString RawFilename	= GetRawSlotSaveFile( Slot );
	const SimpleString FullFilename	= RosaFramework::GetInstance()->GetSaveLoadPath() + RawFilename;
	OutSaveSlotInfo.m_Empty =
		OutSaveSlotInfo.m_Version != ROSA_SAVE_VERSION ||
		!FileUtil::Exists( FullFilename.CStr() );

	STATICHASH( LevelName );
	OutSaveSlotInfo.m_LevelName = ConfigManager::GetString( sLevelName, "", sSlot );

	STATICHASH( Legacy );
	OutSaveSlotInfo.m_Legacy = ConfigManager::GetInt( sLegacy, 0, sSlot );

	STATICHASH( Season );
	OutSaveSlotInfo.m_Season = ConfigManager::GetInt( sSeason, 0, sSlot );

	STATICHASH( Episode );
	OutSaveSlotInfo.m_Episode = ConfigManager::GetInt( sEpisode, 0, sSlot );

	STATICHASH( DateY );
	OutSaveSlotInfo.m_DateY = ConfigManager::GetInt( sDateY, 0, sSlot );

	STATICHASH( DateM );
	OutSaveSlotInfo.m_DateM = ConfigManager::GetInt( sDateM, 0, sSlot );

	STATICHASH( DateD );
	OutSaveSlotInfo.m_DateD = ConfigManager::GetInt( sDateD, 0, sSlot );

	STATICHASH( TimeH );
	OutSaveSlotInfo.m_TimeH = ConfigManager::GetInt( sTimeH, 0, sSlot );

	STATICHASH( TimeM );
	OutSaveSlotInfo.m_TimeM = ConfigManager::GetInt( sTimeM, 0, sSlot );

	STATICHASH( TimeS );
	OutSaveSlotInfo.m_TimeS = ConfigManager::GetInt( sTimeS, 0, sSlot );
}

void RosaSaveLoad::FlushWorldFiles()
{
	XTRACE_FUNCTION;

	PRINTF( "S/L: Flushing world files:\n" );

	const SimpleString		SaveLoadPath	= RosaFramework::GetInstance()->GetSaveLoadPath();

#if !ROSA_USE_PERSISTENT_WORLDS
	DEVASSERTDESC( m_WorldFiles.Empty(), "Zeta is not expected to save world files. Did something go wrong? (See also RosaGame::GoToLevel)" );
#endif

	FOR_EACH_ARRAY( WorldFileIter, m_WorldFiles, SimpleString )
	{
		const SimpleString&	WorldFile		= WorldFileIter.GetValue();
		const SimpleString	FullWorldFile	= SaveLoadPath + WorldFile;

		PRINTF( "     Removing %s\n", FullWorldFile.CStr() );
		FileUtil::RemoveFile( FullWorldFile.CStr() );
	}

	m_WorldFiles.Clear();
}

bool RosaSaveLoad::HasMRUSave()
{
	if( GetMRUSaveSlot() == "" )
	{
		return false;
	}

	SSaveSlotInfo MRUSaveSlot;
	GetSaveSlotInfo( GetMRUSaveSlot(), MRUSaveSlot );

	if( MRUSaveSlot.m_Empty )
	{
		// This covers the case of a version mismatch
		return false;
	}

	return true;
}

bool RosaSaveLoad::TryLoadSave( const uint Index )
{
	return TryLoadCommon( SimpleString::PrintF( "Save%d", Index ) );
}

void RosaSaveLoad::SaveSave( const uint Index )
{
	SaveCommon( SimpleString::PrintF( "Save%d", Index ), true /*TrySaveWorldState*/ );
}

bool RosaSaveLoad::TryLoadAutosave( const uint Index )
{
	return TryLoadCommon( SimpleString::PrintF( "Autosave%d", Index ) );
}

// NOTE: Always saves into first slot, and bounces all existing autosaves down one slot
void RosaSaveLoad::SaveAutosave()
{
	STATICHASH( NumAutosaves );
	const int NumAutosaves = ConfigManager::GetInt( sNumAutosaves );
	ASSERT( NumAutosaves > 0 );

	for( int AutosaveIndex = NumAutosaves - 2; AutosaveIndex >= 0; --AutosaveIndex )
	{
		const int			NextAutosaveIndex	= AutosaveIndex + 1;
		const SimpleString&	CurrentSlot			= SimpleString::PrintF( "Autosave%d", AutosaveIndex );
		const SimpleString&	NextSlot			= SimpleString::PrintF( "Autosave%d", NextAutosaveIndex );

		MoveCommon( CurrentSlot, NextSlot );
	}

	SaveCommon( "Autosave0", true /*TrySaveWorldState*/ );
}

bool RosaSaveLoad::TryLoadQuicksave()
{
	return TryLoadCommon( "Quicksave" );
}

void RosaSaveLoad::SaveQuicksave()
{
	SaveCommon( "Quicksave", true /*TrySaveWorldState*/ );
}

bool RosaSaveLoad::TryLoadSlot( const SimpleString& SlotName )
{
	return TryLoadCommon( SlotName );
}

void RosaSaveLoad::SaveSlot( const SimpleString& SlotName )
{
	SaveCommon( SlotName, true /*TrySaveWorldState*/ );
}

bool RosaSaveLoad::IsSlotFull( const SimpleString& SlotName )
{
	SSaveSlotInfo SlotInfo;
	GetSaveSlotInfo( SlotName, SlotInfo );

	return !SlotInfo.m_Empty;
}

void RosaSaveLoad::MoveCommon( const SimpleString& OldSlotName, const SimpleString& NewSlotName )
{
	if( OldSlotName == "" ||
		NewSlotName == "" ||
		OldSlotName == NewSlotName )
	{
		return;
	}

	const SimpleString OldRawFilename	= GetRawSlotSaveFile( OldSlotName );
	const SimpleString OldFullFilename	= RosaFramework::GetInstance()->GetSaveLoadPath() + OldRawFilename;

	const SimpleString NewRawFilename	= GetRawSlotSaveFile( NewSlotName );
	const SimpleString NewFullFilename	= RosaFramework::GetInstance()->GetSaveLoadPath() + NewRawFilename;

	// Move saved game
	if( FileUtil::Exists( OldFullFilename.CStr() ) )
	{
		// Move config for index file
		MoveSaveSlot( OldSlotName, NewSlotName );

		const bool Success = FileUtil::Move( OldFullFilename.CStr(), NewFullFilename.CStr() );
		ASSERT( Success );
		Unused( Success );

		// Publish index file from config
		WriteSaveIndexFile();
	}
}

void RosaSaveLoad::DeleteCommon( const SimpleString& SlotName )
{
	if( SlotName == "" )
	{
		return;
	}

	const SimpleString RawFilename	= GetRawSlotSaveFile( SlotName );
	const SimpleString FullFilename	= RosaFramework::GetInstance()->GetSaveLoadPath() + RawFilename;

	if( FileUtil::Exists( FullFilename.CStr() ) )
	{
		const bool Success = FileUtil::RemoveFile( FullFilename.CStr() );
		ASSERT( Success );
		Unused( Success );

		// NOTE: No need to publish index file (WriteSaveIndexFile) because
		// that isn't used to indicate empty slots. The file being gone is
		// sufficient.
	}
}

void RosaSaveLoad::SaveCommon( const SimpleString& SlotName, const bool TrySaveWorldState )
{
	if( SlotName == "" )
	{
		return;
	}

	const SimpleString RawFilename	= GetRawSlotSaveFile( SlotName );
	const SimpleString FullFilename	= RosaFramework::GetInstance()->GetSaveLoadPath() + RawFilename;

	// Update config for index file
	SetMRUSaveSlot( SlotName );
	UpdateSaveSlot( SlotName );

	// Save game
	SaveMaster( FullFilename, TrySaveWorldState );

	// Publish index file from config
	WriteSaveIndexFile();

	// Store stats after every save
	IAchievementManager* const pAchievementManager = RosaFramework::GetInstance()->GetAchievementManager();
	if( pAchievementManager )
	{
		pAchievementManager->Store();
	}
}

bool RosaSaveLoad::TryLoadCommon( const SimpleString& SlotName )
{
	if( SlotName == "" )
	{
		return false;
	}

	const SimpleString RawFilename	= GetRawSlotSaveFile( SlotName );
	const SimpleString FullFilename	= RosaFramework::GetInstance()->GetSaveLoadPath() + RawFilename;

	return TryLoadMaster( FullFilename );
}

/*static*/ void RosaSaveLoad::WriteSaveIndexFile()
{
	const SimpleString	SaveIndexFile		= GetSaveIndexFile();
	const FileStream	SaveIndexFileStream	= FileStream( SaveIndexFile.CStr(), FileStream::EFM_Write );

	const SimpleString	SaveIndexContext	= "RosaSaveIndex";

	ConfigManager::BeginWriting();

	ConfigManager::Write( SaveIndexFileStream, "MRUSaveSlot", SaveIndexContext );

	WriteSaveSlotInfos( SaveIndexFileStream );
}

/*static*/ void RosaSaveLoad::WriteSaveSlotInfos( const IDataStream& Stream )
{
	STATICHASH( NumAutosaves );
	const uint NumAutosaves = ConfigManager::GetInt( sNumAutosaves );

	STATICHASH( NumSaves );
	const uint NumSaves = ConfigManager::GetInt( sNumSaves );

	// ROSANOTE: This is fine to do even if quicksaves are dev-only;
	// see below, nothing will be written if a quicksave does not exist.
	{
		WriteSaveSlotInfo( "Quicksave", Stream );
	}

	for( uint AutosaveIndex = 0; AutosaveIndex < NumAutosaves; ++AutosaveIndex )
	{
		WriteSaveSlotInfo( SimpleString::PrintF( "Autosave%d", AutosaveIndex ), Stream );
	}

	for( uint SaveIndex = 0; SaveIndex < NumSaves; ++SaveIndex )
	{
		WriteSaveSlotInfo( SimpleString::PrintF( "Save%d", SaveIndex ), Stream );
	}
}

/*static*/ void RosaSaveLoad::WriteSaveSlotInfo( const SimpleString& Slot, const IDataStream& Stream )
{
	// These do nothing if the vars don't exist, so for example,
	// nothing will be written to Quicksave if user has not quicksaved.

	ConfigManager::Write( Stream, "Version",	Slot );

	// SAVELOADTODO: Presentation for save slots
	ConfigManager::Write( Stream, "LevelName",	Slot );
	ConfigManager::Write( Stream, "Legacy",		Slot );
	ConfigManager::Write( Stream, "Season",		Slot );
	ConfigManager::Write( Stream, "Episode",	Slot );
	ConfigManager::Write( Stream, "DateY",		Slot );
	ConfigManager::Write( Stream, "DateM",		Slot );
	ConfigManager::Write( Stream, "DateD",		Slot );
	ConfigManager::Write( Stream, "TimeH",		Slot );
	ConfigManager::Write( Stream, "TimeM",		Slot );
	ConfigManager::Write( Stream, "TimeS",		Slot );
}

bool RosaSaveLoad::TryLoadMaster( const SimpleString& MasterFile )
{
	XTRACE_FUNCTION;

	if( MasterFile == "" )
	{
		return false;
	}

	const bool MasterFileExists = FileUtil::Exists( MasterFile.CStr() );
	if( !MasterFileExists )
	{
		return false;
	}

	FlushWorldFiles();

	PRINTF( "S/L: Restoring master file %s\n", MasterFile.CStr() );
	FileStream MasterFileStream( MasterFile.CStr(), FileStream::EFM_Read );
	const uint UncompressedSize = MasterFileStream.ReadUInt32();
	const uint CompressedSize = MasterFileStream.ReadUInt32();

	Array<byte> CompressedBuffer;
	CompressedBuffer.Resize( CompressedSize );

	Array<byte> UncompressedBuffer;
	UncompressedBuffer.Resize( UncompressedSize );

	MasterFileStream.Read( CompressedSize, CompressedBuffer.GetData() );

	uLong DestinationSize = static_cast<uLong>( UncompressedSize );
	uncompress( UncompressedBuffer.GetData(), &DestinationSize, CompressedBuffer.GetData(), CompressedSize );

	MemoryStream MasterMemoryStream( UncompressedBuffer.GetData(), UncompressedSize );
	return LoadMaster( MasterMemoryStream );
}

void RosaSaveLoad::SaveMaster( const SimpleString& MasterFile, const bool TrySaveWorldState )
{
	XTRACE_FUNCTION;

	DynamicMemoryStream MasterMemoryStream;
	SaveMaster( MasterMemoryStream, TrySaveWorldState );

	const Array<byte>& UncompressedBuffer = MasterMemoryStream.GetArray();
	const uint UncompressedSize = UncompressedBuffer.MemorySize();

	uLong CompressedSize = compressBound( UncompressedSize );
	Array<byte> CompressedBuffer;
	CompressedBuffer.Resize( static_cast<c_uint32>( CompressedSize ) );

	compress( CompressedBuffer.GetData(), &CompressedSize, UncompressedBuffer.GetData(), UncompressedSize );
	CompressedBuffer.Resize( static_cast<c_uint32>( CompressedSize ) );

	PRINTF( "S/L: Creating master file %s\n", MasterFile.CStr() );
	FileStream MasterFileStream( MasterFile.CStr(), FileStream::EFM_Write );
	MasterFileStream.WriteUInt32( UncompressedSize );
	MasterFileStream.WriteUInt32( CompressedSize );
	MasterFileStream.Write( CompressedBuffer.MemorySize(), CompressedBuffer.GetData() );
}

bool RosaSaveLoad::TryLoadWorld( const SimpleString& WorldFile )
{
	XTRACE_FUNCTION;

#if !ROSA_USE_PERSISTENT_WORLDS
	DEVWARNDESC( "Zeta is not expected to load world files. Did something go wrong? (See also RosaGame::GoToLevel)" );
#endif

	const SimpleString SaveLoadPath		= RosaFramework::GetInstance()->GetSaveLoadPath();
	const SimpleString FullWorldFile	= SaveLoadPath + WorldFile;

	const bool WorldFileExists = FileUtil::Exists( FullWorldFile.CStr() );
	if( !WorldFileExists )
	{
		return false;
	}

	if( !m_WorldFiles.Find( WorldFile ) )
	{
		// If this fails, it means the world file exists but we don't know about it.
		// It's probably stale data from a crashed run. Don't load it.
		return false;
	}

	FileStream WorldFileStream( FullWorldFile.CStr(), FileStream::EFM_Read );
	LoadWorld( WorldFileStream );

	return true;
}

void RosaSaveLoad::SaveWorld( const SimpleString& WorldFile )
{
	XTRACE_FUNCTION;

#if !ROSA_USE_PERSISTENT_WORLDS
	DEVWARNDESC( "Zeta is not expected to save world files. Did something go wrong? (See also RosaGame::GoToLevel)" );
#endif

	const SimpleString SaveLoadPath		= RosaFramework::GetInstance()->GetSaveLoadPath();
	const SimpleString FullWorldFile	= SaveLoadPath + WorldFile;

	PRINTF( "S/L: Creating world file %s\n", FullWorldFile.CStr() );
	FileStream WorldFileStream( FullWorldFile.CStr(), FileStream::EFM_Write );
	SaveWorld( WorldFileStream );

	m_WorldFiles.PushBackUnique( WorldFile );
}

/*static*/ bool RosaSaveLoad::ShouldSaveCurrentWorld() const
{
	RosaFramework* const	pFramework	= RosaFramework::GetInstance();
	DEVASSERT( pFramework );

	RosaGame* const			pGame		= pFramework->GetGame();
	DEVASSERT( pGame );

	if( pGame->IsInTitleScreen() )
	{
		return false;
	}

	// Don't save if the player is dead
	if( !RosaGame::IsPlayerAlive() )
	{
		return false;
	}

	// Don't save game if the campaign has been lost
	if( RosaCampaign::GetCampaign()->IsLost() )
	{
		return false;
	}

	// Don't save if we have initiated the ending sequence (or are disabling pause for any other reason)
	if( RosaGame::IsPlayerDisablingPause() )
	{
		return false;
	}

	return true;
}

void RosaSaveLoad::Report() const
{
	PRINTF( "RosaSaveLoad:\n" );
	PRINTF( "  World files:\n" );
	FOR_EACH_ARRAY( WorldFileIter, m_WorldFiles, SimpleString )
	{
		PRINTF( "    %s\n", WorldFileIter.GetValue().CStr() );
	}
	PRINTF( "  Pending save slot: %s\n", m_PendingSaveSlot.CStr() );
#if ROSA_USE_ACTIVESAVESLOT
	PRINTF( "  Active save slot: %s\n", m_ActiveSaveSlot.CStr() );
#endif
}

#define MASTER_VERSION_EMPTY		0
#define MASTER_VERSION_PERSISTENCE	1
#define MASTER_VERSION_WORLD		2
#define MASTER_VERSION_WORLDMEMORY	3
#define MASTER_VERSION_CAMPAIGN		4
#define MASTER_VERSION_ACTIVESLOT	5
#define MASTER_VERSION_CURRENT		5

void RosaSaveLoad::SaveMaster( const IDataStream& Stream, const bool TrySaveWorldState )
{
	XTRACE_FUNCTION;

	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	ASSERT( pFramework );

	RosaWorld* const		pWorld			= pFramework->GetWorld();
	ASSERT( pWorld );

	RosaGame* const			pGame			= pFramework->GetGame();
	ASSERT( pGame );

	RosaPersistence* const	pPersistence	= pGame->GetPersistence();
	ASSERT( pPersistence );

	RosaCampaign* const		pCampaign		= pGame->GetCampaign();
	ASSERT( pCampaign );

	// Write version
	Stream.WriteUInt32( MASTER_VERSION_CURRENT );

#if ROSA_USE_ACTIVESAVESLOT
	DEVASSERT( m_ActiveSaveSlot != "" );
	Stream.WriteString( m_ActiveSaveSlot );
#endif

	// Write persistence
	pPersistence->Save( Stream );

	// Write campaign
	pCampaign->Save( Stream );

	// Write current world
	const bool WorldSaved = TrySaveWorldState && ShouldSaveCurrentWorld();
	Stream.WriteBool( WorldSaved );
	if( WorldSaved )
	{
		Stream.WriteString( pGame->GetCurrentLevelName() );
		pWorld->Save( Stream );
	}
	else
	{
		// If we're not saving this world, we shouldn't be saving any other worlds either.
		FlushWorldFiles();
	}

	// Write world memories, bundling loose files
	const SimpleString		SaveLoadPath	= RosaFramework::GetInstance()->GetSaveLoadPath();
	Stream.WriteUInt32( m_WorldFiles.Size() );
	FOR_EACH_ARRAY( WorldFileIter, m_WorldFiles, SimpleString )
	{
		const SimpleString& WorldFile		= WorldFileIter.GetValue();
		const SimpleString	FullWorldFile	= SaveLoadPath + WorldFile;

		const bool WorldFileExists = FileUtil::Exists( FullWorldFile.CStr() );
		Stream.WriteBool( WorldFileExists );

		if( WorldFileExists )
		{
			FileStream WorldFileStream( FullWorldFile.CStr(), FileStream::EFM_Read );
			const uint WorldFileSize = WorldFileStream.Size();

			Stream.WriteString( WorldFile );
			Stream.WriteUInt32( WorldFileSize );

			DataPipe WorldFilePipe( WorldFileStream, Stream );
			WorldFilePipe.Pipe( WorldFileSize );
		}
	}
}

bool RosaSaveLoad::LoadMaster( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	ASSERT( m_WorldFiles.Empty() );

	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	ASSERT( pFramework );

	RosaWorld* const		pWorld			= pFramework->GetWorld();
	ASSERT( pWorld );

	RosaGame* const			pGame			= pFramework->GetGame();
	ASSERT( pGame );

	RosaPersistence* const	pPersistence	= pGame->GetPersistence();
	ASSERT( pPersistence );

	RosaCampaign* const		pCampaign		= pGame->GetCampaign();
	ASSERT( pCampaign );

	bool WorldLoaded = false;

	// Read version
	const uint Version = Stream.ReadUInt32();

#if ROSA_USE_ACTIVESAVESLOT
	if( Version >= MASTER_VERSION_ACTIVESLOT )
	{
		m_ActiveSaveSlot = Stream.ReadString();
		DEVASSERT( m_ActiveSaveSlot != "" );
	}
#endif

	// Read persistence
	if( Version >= MASTER_VERSION_PERSISTENCE )
	{
		pPersistence->Load( Stream );
	}

	// Read campaign
	if( Version >= MASTER_VERSION_CAMPAIGN )
	{
		pCampaign->Load( Stream );
	}

	// Read current world
	if( Version >= MASTER_VERSION_WORLD )
	{
		WorldLoaded = Stream.ReadBool();
		if( WorldLoaded )
		{
			pGame->SetCurrentLevelName( Stream.ReadString() );
			pWorld->Load( Stream );
			pFramework->InitializeTools();
			pGame->RefreshUIRetreatEnabled();
		}
	}

	// Read world memories, restore loose files
	if( Version >= MASTER_VERSION_WORLDMEMORY )
	{
		PRINTF( "S/L: Restoring world files:\n" );
		const SimpleString		SaveLoadPath	= RosaFramework::GetInstance()->GetSaveLoadPath();
		const uint NumWorldFiles = Stream.ReadUInt32();
		for( uint WorldFileIndex = 0; WorldFileIndex < NumWorldFiles; ++WorldFileIndex )
		{
			const bool WorldFileExisted = Stream.ReadBool();
			if( !WorldFileExisted )
			{
				continue;
			}

			const SimpleString	WorldFile		= Stream.ReadString();
			const SimpleString	FullWorldFile	= SaveLoadPath + WorldFile;
			const uint WorldFileSize = Stream.ReadUInt32();

			PRINTF( "     Restoring %s\n", FullWorldFile.CStr() );

			FileStream WorldFileStream( FullWorldFile.CStr(), FileStream::EFM_Write );
			DataPipe WorldFilePipe( Stream, WorldFileStream );
			WorldFilePipe.Pipe( WorldFileSize );

			m_WorldFiles.PushBack( WorldFile );
		}
	}

	WB_MAKE_EVENT( OnMasterFileLoaded, NULL );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), OnMasterFileLoaded, NULL );

	return WorldLoaded;
}

#define WORLD_VERSION_EMPTY		0
#define WORLD_VERSION_WORLD		1
#define WORLD_VERSION_CURRENT	1

void RosaSaveLoad::SaveWorld( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	// We assume we should save this world if we've made it this far.
	DEVASSERT( ShouldSaveCurrentWorld() );

	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	ASSERT( pFramework );

	RosaWorld* const		pWorld			= pFramework->GetWorld();
	ASSERT( pWorld );

	Stream.WriteUInt32( WORLD_VERSION_CURRENT );

	pWorld->Save( Stream );
}

void RosaSaveLoad::LoadWorld( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	ASSERT( pFramework );

	RosaWorld* const		pWorld			= pFramework->GetWorld();
	ASSERT( pWorld );

	const bool ForceResetToGameScreens = true;
	pFramework->PrepareForLoad( ForceResetToGameScreens );

	const uint Version = Stream.ReadUInt32();

	if( Version >= WORLD_VERSION_WORLD )
	{
		pWorld->Load( Stream );
		pFramework->InitializeTools();
	}
}
