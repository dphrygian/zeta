#include "core.h"
#include "wbcomprosacharacter.h"
#include "wbcomprosacharacterconfig.h"
#include "configmanager.h"
#include "idatastream.h"
#include "wbeventmanager.h"
#include "wbcomprosahands.h"
#include "rosagame.h"
#include "rosapersistence.h"
#include "hsv.h"
#include "mathcore.h"
#include "rosaframework.h"
#include "Common/uimanagercommon.h"
#include "Screens/uiscreen-rosacharacterconfig.h"

WBCompRosaCharacter::WBCompRosaCharacter()
:	m_HeadOptions()
,	m_BodyOptions()
,	m_SkinPresets()
,	m_NailsPresets()
,	m_CurrentSkin()
,	m_CurrentNails()
{
}

WBCompRosaCharacter::~WBCompRosaCharacter()
{
}

/*virtual*/ void WBCompRosaCharacter::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( NumSkinPresets );
	const uint NumSkinPresets = ConfigManager::GetInheritedInt( sNumSkinPresets, 0, sDefinitionName );
	for( uint SkinPresetIndex = 0; SkinPresetIndex < NumSkinPresets; ++SkinPresetIndex )
	{
		Vector& SkinPreset = m_SkinPresets.PushBack();

		const SimpleString SkinPresetDef = ConfigManager::GetInheritedSequenceString( "SkinPreset%d", SkinPresetIndex, "", sDefinitionName );

		MAKEHASH( SkinPresetDef );
		SkinPreset = HSV::GetConfigHSV( "Color", sSkinPresetDef, Vector() );
	}

	STATICHASH( NumNailsPresets );
	const uint NumNailsPresets = ConfigManager::GetInheritedInt( sNumNailsPresets, 0, sDefinitionName );
	for( uint NailsPresetIndex = 0; NailsPresetIndex < NumNailsPresets; ++NailsPresetIndex )
	{
		Vector4& NailsPreset = m_NailsPresets.PushBack();

		const SimpleString NailsPresetDef = ConfigManager::GetInheritedSequenceString( "NailsPreset%d", NailsPresetIndex, "", sDefinitionName );

		MAKEHASH( NailsPresetDef );
		NailsPreset = HSV::GetConfigHSVA( "Color", sNailsPresetDef, Vector4() );
	}

	{
		STATICHASH( SkinDefault );
		const SimpleString SkinDefaultDef = ConfigManager::GetInheritedString( sSkinDefault, "", sDefinitionName );

		MAKEHASH( SkinDefaultDef );
		m_CurrentSkin = HSV::GetConfigHSV( "Color", SkinDefaultDef, Vector() );
	}

	{
		STATICHASH( NailsDefault );
		const SimpleString NailsDefaultDef = ConfigManager::GetInheritedString( sNailsDefault, "", sDefinitionName );

		MAKEHASH( NailsDefaultDef );
		m_CurrentNails = HSV::GetConfigHSVA( "Color", NailsDefaultDef, Vector4() );
	}
}

/*virtual*/ void WBCompRosaCharacter::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnSpawned );
	STATIC_HASHED_STRING( OnLoaded );
	STATIC_HASHED_STRING( OnRespawnedInventory );
	STATIC_HASHED_STRING( SelectCharacterPreset );
	STATIC_HASHED_STRING( PushPersistence );
	STATIC_HASHED_STRING( PullPersistence );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnSpawned )
	{
		PushCharacterOptions();
	}
	else if( EventName == sOnLoaded )
	{
		// Everything in the world should already be serialized properly, but we need to push to UI screen.
		PushCharacterOptions();
	}
	else if( EventName == sOnRespawnedInventory )
	{
		// Push hand meshes onto newly spawned hands.
		PushCharacterOptions();
	}
	else if( EventName == sSelectCharacterPreset )
	{
		STATIC_HASHED_STRING( SkinPresetIndex );
		int SkinPresetIndex = Event.GetInt( sSkinPresetIndex );

		STATIC_HASHED_STRING( NailsPresetIndex );
		int NailsPresetIndex = Event.GetInt( sNailsPresetIndex );

		SelectCharacterPreset( SkinPresetIndex, NailsPresetIndex );
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

void WBCompRosaCharacter::SelectCharacterPreset( const int SkinPresetIndex, const int NailsPresetIndex )
{
	// This is a bit of a hack. We select presets by setting UI sliders,
	// which ends up setting the current values and keeps UI in sync.

	if( SkinPresetIndex >= 0 )
	{
		SetCurrentSkin( m_SkinPresets[ SkinPresetIndex ] );
	}

	if( NailsPresetIndex >= 0 )
	{
		SetCurrentNails( m_NailsPresets[ NailsPresetIndex ] );
	}

	PushSkinAndNailsToUI();
}

void WBCompRosaCharacter::PushSkinAndNailsToConfig() const
{
	WBCompRosaCharacterConfig* const pConfig = WB_GETCOMP( GetEntity(), RosaCharacterConfig );

	// Blend skin and nail tone for Rosa player hands
	const Vector	NailNaturalHSV	= Vector( 0.0f, 0.0f, 1.0f );							// Hard-coded, nails are pure white
	const Vector4	SkinRGB			= Vector4( HSV::HSVToRGB( m_CurrentSkin ), 0.0f );		// Hard-coded skin bit (see character shader)
	const Vector4	NailPolishRGB	= Vector4( HSV::HSVToRGB( m_CurrentNails ), 1.0f );		// Hard-coded no-skin bit (see character shader)
	const Vector4	NailNaturalRGB	= Vector4( HSV::HSVToRGB( NailNaturalHSV ), 0.0f );		// Hard-coded skin bit (see character shader)
	const Vector4	NailBlendRGB	= Lerp<Vector4>( SkinRGB,		NailNaturalRGB,	0.5f );	// Hard-coded, nails are 50% blended at most

	const Vector4	SecondaryRGB	= Lerp<Vector4>( SkinRGB,		NailPolishRGB,	m_CurrentNails.a );
	const Vector4	AccentRGB		= Lerp<Vector4>( NailBlendRGB,	NailPolishRGB,	m_CurrentNails.a );

	pConfig->SetCharacterColor( HashedString::NullString, 0, SkinRGB );
	pConfig->SetCharacterColor( HashedString::NullString, 1, SecondaryRGB );
	pConfig->SetCharacterColor( HashedString::NullString, 2, AccentRGB );
}

void WBCompRosaCharacter::PushSkinAndNailsToUI() const
{
	RosaFramework* const	pFramework	= RosaFramework::GetInstance();
	UIManager* const		pUIManager	= pFramework->GetUIManager();

	pUIManager->SetSliderValue( "CharacterConfigScreen", "CharacterConfigSkinHSlider",	m_CurrentSkin.x );
	pUIManager->SetSliderValue( "CharacterConfigScreen", "CharacterConfigSkinSSlider",	m_CurrentSkin.y );
	pUIManager->SetSliderValue( "CharacterConfigScreen", "CharacterConfigSkinVSlider",	m_CurrentSkin.z );
	pUIManager->SetSliderValue( "CharacterConfigScreen", "CharacterConfigNailsHSlider",	m_CurrentNails.x );
	pUIManager->SetSliderValue( "CharacterConfigScreen", "CharacterConfigNailsSSlider",	m_CurrentNails.y );
	pUIManager->SetSliderValue( "CharacterConfigScreen", "CharacterConfigNailsVSlider",	m_CurrentNails.z );
	pUIManager->SetSliderValue( "CharacterConfigScreen", "CharacterConfigNailsASlider",	m_CurrentNails.w );
}

void WBCompRosaCharacter::PushCharacterOptions() const
{
	// Update character config
	PushSkinAndNailsToUI();
}

void WBCompRosaCharacter::PushPersistence() const
{
	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	//STATIC_HASHED_STRING( HeadIndex );
	//Persistence.SetInt( sHeadIndex, m_CurrentHeadIndex );

	//STATIC_HASHED_STRING( BodyIndex );
	//Persistence.SetInt( sBodyIndex, m_CurrentBodyIndex );

	STATIC_HASHED_STRING( SkinHSV );
	Persistence.SetVector( sSkinHSV, m_CurrentSkin );

	STATIC_HASHED_STRING( NailsHSV );
	Persistence.SetVector( sNailsHSV, m_CurrentNails );

	STATIC_HASHED_STRING( NailsA );
	Persistence.SetFloat( sNailsA, m_CurrentNails.a );
}

void WBCompRosaCharacter::PullPersistence()
{
	TPersistence& Persistence = RosaGame::StaticGetTravelPersistence();

	STATIC_HASHED_STRING( SkinHSV );
	m_CurrentSkin = Persistence.GetVector( sSkinHSV );

	STATIC_HASHED_STRING( NailsHSV );
	m_CurrentNails = Persistence.GetVector( sNailsHSV );

	STATIC_HASHED_STRING( NailsA );
	m_CurrentNails.a = Persistence.GetFloat( sNailsA );

	PushCharacterOptions();
}

#define VERSION_EMPTY			0
#define VERSION_SKINANDNAILS	1
#define VERSION_CURRENT			1

uint WBCompRosaCharacter::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;					// Version
	Size += sizeof( Vector );	// m_CurrentSkin
	Size += sizeof( Vector4 );	// m_CurrentNails

	return Size;
}

void WBCompRosaCharacter::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.Write<Vector>( m_CurrentSkin );
	Stream.Write<Vector4>( m_CurrentNails );
}

void WBCompRosaCharacter::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_SKINANDNAILS )
	{
		m_CurrentSkin	= Stream.Read<Vector>();
		m_CurrentNails	= Stream.Read<Vector4>();
		PushSkinAndNailsToUI();
	}
}
