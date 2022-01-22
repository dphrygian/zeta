#include "core.h"
#include "uiscreen-rosacharacterconfig.h"
#include "configmanager.h"
#include "Components/wbcomprosacharacter.h"
#include "rosagame.h"
#include "Widgets/uiwidget-slider.h"
#include "uifactory.h"
#include "hsv.h"
#include "mathcore.h"

UIScreenRosaCharacterConfig::UIScreenRosaCharacterConfig()
:	m_PresetButtonIndex( 0 )
,	m_ButtonWidgetDefinitionName()
,	m_ActionDefinitionName()
,	m_ParentWX( 0.0f )
,	m_SkinPresetButtonArchetype()
,	m_NailsPresetButtonArchetype()
,	m_PresetButtonParentWXSpacing( 0.0f )
{
}

UIScreenRosaCharacterConfig::~UIScreenRosaCharacterConfig()
{
}

/*virtual*/ void UIScreenRosaCharacterConfig::RegisterForEvents()
{
	STATIC_HASHED_STRING( OnSliderChanged );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sOnSliderChanged, this, NULL );
}

/*virtual*/ void UIScreenRosaCharacterConfig::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Unused( DefinitionName );

	Flush();

	UIScreen::InitializeFromDefinition( DefinitionName );

	WBCompRosaCharacter* const	pCharacter		= GetCharacterComponent();
	if( !pCharacter )
	{
		// We can't initialize presets until we have a character
		return;
	}

	InitializeRules();

	const Array<Vector>&		SkinPresets		= pCharacter->GetSkinPresets();
	const Array<Vector4>&		NailsPresets	= pCharacter->GetNailsPresets();

	m_ParentWX = 0.5f - m_PresetButtonParentWXSpacing * 0.5f * static_cast<float>( SkinPresets.Size() - 1 );
	FOR_EACH_ARRAY( SkinPresetIter, SkinPresets, Vector )
	{
		m_PresetButtonIndex	= SkinPresetIter.GetIndex();
		CreateSkinPresetActionDefinition();
		CreateSkinPresetButtonWidgetDefinition();
		CreatePresetButtonWidget();
		m_ParentWX += m_PresetButtonParentWXSpacing;
	}

	m_ParentWX = 0.5f - m_PresetButtonParentWXSpacing * 0.5f * static_cast<float>( NailsPresets.Size() - 1 );
	FOR_EACH_ARRAY( NailsPresetIter, NailsPresets, Vector4 )
	{
		m_PresetButtonIndex	= NailsPresetIter.GetIndex();
		CreateNailsPresetActionDefinition();
		CreateNailsPresetButtonWidgetDefinition();
		CreatePresetButtonWidget();
		m_ParentWX += m_PresetButtonParentWXSpacing;
	}

	UpdateRender();
	ResetFocus();
	RefreshWidgets();
}

void UIScreenRosaCharacterConfig::InitializeRules()
{
	MAKEHASH( m_Name );

	STATICHASH( Rules );
	const SimpleString UsingRules = ConfigManager::GetString( sRules, "", sm_Name );

	MAKEHASH( UsingRules );

	STATICHASH( SkinPresetButtonArchetype );
	m_SkinPresetButtonArchetype = ConfigManager::GetString( sSkinPresetButtonArchetype, "", sUsingRules );

	STATICHASH( NailsPresetButtonArchetype );
	m_NailsPresetButtonArchetype = ConfigManager::GetString( sNailsPresetButtonArchetype, "", sUsingRules );

	STATICHASH( PresetButtonParentWXSpacing );
	m_PresetButtonParentWXSpacing = ConfigManager::GetFloat( sPresetButtonParentWXSpacing, 0.0f, sUsingRules );
}

void UIScreenRosaCharacterConfig::CreateSkinPresetActionDefinition()
{
	m_ActionDefinitionName = SimpleString::PrintF( "_SkinPresetAction%d", m_PresetButtonIndex );

	MAKEHASH( m_ActionDefinitionName );

	STATICHASH( ActionType );
	ConfigManager::SetString( sActionType, "RosaSelectCharacterPreset", sm_ActionDefinitionName );

	STATICHASH( SkinPreset );
	ConfigManager::SetInt( sSkinPreset, m_PresetButtonIndex, sm_ActionDefinitionName );
}

void UIScreenRosaCharacterConfig::CreateSkinPresetButtonWidgetDefinition()
{
	const Vector	SkinHSV	= GetCharacterComponent()->GetSkinPresets()[ m_PresetButtonIndex ];
	const Vector	SkinRGB	= HSV::HSVToRGB( SkinHSV );

	m_ButtonWidgetDefinitionName = SimpleString::PrintF( "_SkinPresetButton%d", m_PresetButtonIndex );

	MAKEHASH( m_ButtonWidgetDefinitionName );

	STATICHASH( Extends );
	ConfigManager::SetString( sExtends, m_SkinPresetButtonArchetype.CStr(), sm_ButtonWidgetDefinitionName );

	STATICHASH( ParentWX );
	ConfigManager::SetFloat( sParentWX, m_ParentWX, sm_ButtonWidgetDefinitionName );

	STATICHASH( ColorR );
	ConfigManager::SetFloat( sColorR, SkinRGB.r, sm_ButtonWidgetDefinitionName );

	STATICHASH( ColorG );
	ConfigManager::SetFloat( sColorG, SkinRGB.g, sm_ButtonWidgetDefinitionName );

	STATICHASH( ColorB );
	ConfigManager::SetFloat( sColorB, SkinRGB.b, sm_ButtonWidgetDefinitionName );

	STATICHASH( NumActions );
	ConfigManager::SetInt( sNumActions, 1, sm_ButtonWidgetDefinitionName );

	STATICHASH( Action0 );
	ConfigManager::SetString( sAction0, m_ActionDefinitionName.CStr(), sm_ButtonWidgetDefinitionName );
}

void UIScreenRosaCharacterConfig::CreateNailsPresetActionDefinition()
{
	m_ActionDefinitionName = SimpleString::PrintF( "_NailsPresetAction%d", m_PresetButtonIndex );

	MAKEHASH( m_ActionDefinitionName );

	STATICHASH( ActionType );
	ConfigManager::SetString( sActionType, "RosaSelectCharacterPreset", sm_ActionDefinitionName );

	STATICHASH( NailsPreset );
	ConfigManager::SetInt( sNailsPreset, m_PresetButtonIndex, sm_ActionDefinitionName );
}

void UIScreenRosaCharacterConfig::CreateNailsPresetButtonWidgetDefinition()
{
	const Vector4	NailsHSVA		= GetCharacterComponent()->GetNailsPresets()[ m_PresetButtonIndex ];
	const Vector	NailPolishRGB	= HSV::HSVToRGB( NailsHSVA );
	const Vector	NailWhiteRGB	= Vector( 1.0f, 1.0f, 1.0f );
	const Vector	NailsRGB		= Lerp<Vector>( NailWhiteRGB, NailPolishRGB, NailsHSVA.a );

	m_ButtonWidgetDefinitionName = SimpleString::PrintF( "_NailsPresetButton%d", m_PresetButtonIndex );

	MAKEHASH( m_ButtonWidgetDefinitionName );

	STATICHASH( Extends );
	ConfigManager::SetString( sExtends, m_NailsPresetButtonArchetype.CStr(), sm_ButtonWidgetDefinitionName );

	STATICHASH( ParentWX );
	ConfigManager::SetFloat( sParentWX, m_ParentWX, sm_ButtonWidgetDefinitionName );

	STATICHASH( ColorR );
	ConfigManager::SetFloat( sColorR, NailsRGB.r, sm_ButtonWidgetDefinitionName );

	STATICHASH( ColorG );
	ConfigManager::SetFloat( sColorG, NailsRGB.g, sm_ButtonWidgetDefinitionName );

	STATICHASH( ColorB );
	ConfigManager::SetFloat( sColorB, NailsRGB.b, sm_ButtonWidgetDefinitionName );

	STATICHASH( NumActions );
	ConfigManager::SetInt( sNumActions, 1, sm_ButtonWidgetDefinitionName );

	STATICHASH( Action0 );
	ConfigManager::SetString( sAction0, m_ActionDefinitionName.CStr(), sm_ButtonWidgetDefinitionName );
}

void UIScreenRosaCharacterConfig::CreatePresetButtonWidget()
{
	UIWidget* const pButtonWidget = UIFactory::CreateWidget( m_ButtonWidgetDefinitionName, this, NULL );
	DEVASSERT( pButtonWidget );

	AddWidget( pButtonWidget );
}

/*virtual*/ void UIScreenRosaCharacterConfig::Pushed()
{
	UIScreen::Pushed();

	// Reinitialize whenever this screen is pushed.
	// (When UI is first initialized, player entity doesn't exist so preset buttons can't be made.)
	InitializeFromDefinition( m_Name );

	// Update slider positions
	GetCharacterComponent()->PushSkinAndNailsToUI();
}

/*virtual*/ void UIScreenRosaCharacterConfig::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	STATIC_HASHED_STRING( OnSliderChanged );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnSliderChanged )
	{
		STATIC_HASHED_STRING( SliderName );
		const HashedString SliderName = Event.GetHash( sSliderName );

		HandleUISliderEvent( SliderName );
	}
}

WBCompRosaCharacter* UIScreenRosaCharacterConfig::GetCharacterComponent() const
{
	WBEntity* const				pPlayer		= RosaGame::GetPlayer();
	WBCompRosaCharacter* const	pCharacter	= WB_GETCOMP_SAFE( pPlayer, RosaCharacter );
	return pCharacter;
}

void UIScreenRosaCharacterConfig::HandleUISliderEvent( const HashedString& SliderName )
{
	WBCompRosaCharacter* const	pCharacter	= GetCharacterComponent();
	DEVASSERT( pCharacter );

	UIWidgetSlider* const		pSlider		= GetWidget<UIWidgetSlider>( SliderName );
	// Don't assert; the active slider may not belong to this screen.

	STATIC_HASHED_STRING( CharacterConfigSkinHSlider );
	STATIC_HASHED_STRING( CharacterConfigSkinSSlider );
	STATIC_HASHED_STRING( CharacterConfigSkinVSlider );
	STATIC_HASHED_STRING( CharacterConfigNailsHSlider );
	STATIC_HASHED_STRING( CharacterConfigNailsSSlider );
	STATIC_HASHED_STRING( CharacterConfigNailsVSlider );
	STATIC_HASHED_STRING( CharacterConfigNailsASlider );

	if( SliderName == sCharacterConfigSkinHSlider )
	{
		Vector SkinHSV	= pCharacter->GetCurrentSkin();
		SkinHSV.x		= pSlider->GetSliderValue();
		pCharacter->SetCurrentSkin( SkinHSV );
		pCharacter->PushSkinAndNailsToConfig();
	}
	else if( SliderName == sCharacterConfigSkinSSlider )
	{
		Vector SkinHSV	= pCharacter->GetCurrentSkin();
		SkinHSV.y		= pSlider->GetSliderValue();
		pCharacter->SetCurrentSkin( SkinHSV );
		pCharacter->PushSkinAndNailsToConfig();
	}
	else if( SliderName == sCharacterConfigSkinVSlider )
	{
		Vector SkinHSV	= pCharacter->GetCurrentSkin();
		SkinHSV.z		= pSlider->GetSliderValue();
		pCharacter->SetCurrentSkin( SkinHSV );
		pCharacter->PushSkinAndNailsToConfig();
	}
	else if( SliderName == sCharacterConfigNailsHSlider )
	{
		Vector4 NailsHSVA	= pCharacter->GetCurrentNails();
		NailsHSVA.x			= pSlider->GetSliderValue();
		pCharacter->SetCurrentNails( NailsHSVA );
		pCharacter->PushSkinAndNailsToConfig();
	}
	else if( SliderName == sCharacterConfigNailsSSlider )
	{
		Vector4 NailsHSVA	= pCharacter->GetCurrentNails();
		NailsHSVA.y			= pSlider->GetSliderValue();
		pCharacter->SetCurrentNails( NailsHSVA );
		pCharacter->PushSkinAndNailsToConfig();
	}
	else if( SliderName == sCharacterConfigNailsVSlider )
	{
		Vector4 NailsHSVA	= pCharacter->GetCurrentNails();
		NailsHSVA.z			= pSlider->GetSliderValue();
		pCharacter->SetCurrentNails( NailsHSVA );
		pCharacter->PushSkinAndNailsToConfig();
	}
	else if( SliderName == sCharacterConfigNailsASlider )
	{
		Vector4 NailsHSVA	= pCharacter->GetCurrentNails();
		NailsHSVA.w			= pSlider->GetSliderValue();
		pCharacter->SetCurrentNails( NailsHSVA );
		pCharacter->PushSkinAndNailsToConfig();
	}
}
