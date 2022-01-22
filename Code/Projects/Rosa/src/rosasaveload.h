#ifndef ROSASAVELOAD_H
#define ROSASAVELOAD_H

#include "array.h"
#include "simplestring.h"
#include "rosagame.h"	// For ROSA_USE_ACTIVESAVESLOT and ROSA_USE_PERSISTENT_WORLDS, I could just make a defines header

class RosaGame;
class IDataStream;

class RosaSaveLoad
{
public:
	RosaSaveLoad();
	~RosaSaveLoad();

	void				FlushWorldFiles();

	bool				HasMRUSave();

	bool				TryLoadSave( const uint Index );				// Don't call this directly anymore, go through RosaFramework::RequestLoadSlot to schedule everything correctly.
	void				SaveSave( const uint Index );

	bool				TryLoadAutosave( const uint Index );			// Don't call this directly anymore, go through RosaFramework::RequestLoadSlot to schedule everything correctly.
	void				SaveAutosave();

	bool				TryLoadQuicksave();								// Don't call this directly anymore, go through RosaFramework::RequestLoadSlot to schedule everything correctly.
	void				SaveQuicksave();

	bool				TryLoadSlot( const SimpleString& SlotName );	// Don't call this directly anymore, go through RosaFramework::RequestLoadSlot to schedule everything correctly.
	void				SaveSlot( const SimpleString& SlotName );
	bool				IsSlotFull( const SimpleString& SlotName );

	// These expect save files with the full path
	bool				TryLoadMaster( const SimpleString& MasterFile );
	void				SaveMaster( const SimpleString& MasterFile, const bool TrySaveWorldState );

	bool				TryLoadWorld( const SimpleString& WorldFile );
	void				SaveWorld( const SimpleString& WorldFile );

	bool				ShouldSaveCurrentWorld() const;

	void				SetPendingSaveSlot( const SimpleString& SlotName ) { m_PendingSaveSlot = SlotName; }
	SimpleString		GetPendingSaveSlot() const { return m_PendingSaveSlot; }

#if ROSA_USE_ACTIVESAVESLOT
	void				SetActiveSaveSlot( const SimpleString& SlotName ) { m_ActiveSaveSlot = SlotName; }
	SimpleString		GetActiveSaveSlot() const { return m_ActiveSaveSlot; }
#endif

	void				Report() const;

	static void			WriteSaveIndexFile();

	static SimpleString	GetSaveIndexFile();
	static SimpleString	GetMRUSaveSlot();

	enum ESaveSlotType
	{
		ESST_None,
		ESST_Quicksave,
		ESST_Autosave,
		ESST_Save,
	};

	struct SSaveSlotInfo
	{
		SSaveSlotInfo()
		:	m_SlotName()
		,	m_Empty( false )
		,	m_Type( ESST_None )
		,	m_TypeIndex( 0 )
		,	m_Version( 0 )
		,	m_LevelName()
		,	m_Legacy( 0 )
		,	m_Season( 0 )
		,	m_Episode( 0 )
		,	m_DateY( 0 )
		,	m_DateM( 0 )
		,	m_DateD( 0 )
		,	m_TimeH( 0 )
		,	m_TimeM( 0 )
		,	m_TimeS( 0 )
		{
		}

		SimpleString	m_SlotName;		// E.g., "Quicksave", "Autosave0", "Save1"; probably not used for much, but these are the context names for the index file
		bool			m_Empty;
		ESaveSlotType	m_Type;
		uint			m_TypeIndex;
		int				m_Version;

		// Game-specific presentation stuff (restored from Noir)
		// SAVELOADTODO: Revisit for what's actually needed for Zeta.
		SimpleString	m_LevelName;	// E.g., "Level_Streets1"
		uint			m_Legacy;
		uint			m_Season;
		uint			m_Episode;
		uint			m_DateY;
		uint			m_DateM;
		uint			m_DateD;
		uint			m_TimeH;
		uint			m_TimeM;
		uint			m_TimeS;
	};

	// This just pulls from the config manager, and is initialized from the save index file
	static void			GetSaveSlotInfos( Array<SSaveSlotInfo>& OutSaveSlotInfos, const bool FilterQuickAndAutosaves );
	static void			GetSaveSlotInfo( const SimpleString& Slot, SSaveSlotInfo& OutSaveSlotInfo );

private:
	void				MoveCommon( const SimpleString& OldSlotName, const SimpleString& NewSlotName );
	void				DeleteCommon( const SimpleString& SlotName );

	void				SaveCommon( const SimpleString& SlotName, const bool TrySaveWorldState );
	bool				TryLoadCommon( const SimpleString& SlotName );

	static SimpleString	GetRawSaveIndexFile();
	static SimpleString	GetRawSlotSaveFile( const SimpleString& SlotName );

	static void			WriteSaveSlotInfos( const IDataStream& Stream );
	static void			WriteSaveSlotInfo( const SimpleString& Slot, const IDataStream& Stream );

	static void			SetMRUSaveSlot( const SimpleString& MRUSaveSlot );
	static void			UpdateSaveSlot( const SimpleString& SlotName );
	static void			MoveSaveSlot( const SimpleString& OldSlotName, const SimpleString& NewSlotName );

	void				SaveMaster( const IDataStream& Stream, const bool TrySaveWorldState );
	bool				LoadMaster( const IDataStream& Stream );

	void				SaveWorld( const IDataStream& Stream );
	void				LoadWorld( const IDataStream& Stream );

	Array<SimpleString>	m_WorldFiles;	// Names of current loose world files
	SimpleString		m_PendingSaveSlot;

#if ROSA_USE_ACTIVESAVESLOT
	SimpleString		m_ActiveSaveSlot;	// ZETAHACK
#endif
};

#endif // ROSASAVELOAD_H
