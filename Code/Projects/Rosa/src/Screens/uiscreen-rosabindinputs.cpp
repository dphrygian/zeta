#include "core.h"
#include "uiscreen-rosabindinputs.h"
#include "configmanager.h"
#include "uifactory.h"
#include "inputsystem.h"
#include "rosaframework.h"

UIScreenRosaBindInputs::UIScreenRosaBindInputs()
:	m_ExposedInputIndex( 0 )
,	m_ExposedInput()
,	m_Y( 0.0f )
,	m_YBase( 0.0f )
,	m_YStep( 0.0f )
,	m_Column0X( 0.0f )
,	m_Column1X( 0.0f )
,	m_Column2X( 0.0f )
,	m_Column3X( 0.0f )
,	m_ArchetypeName()
,	m_ControllerArchetypeName()
,	m_Parent()
,	m_LabelWidgetDefinitionName()
,	m_KeyboardWidgetDefinitionName()
,	m_MouseWidgetDefinitionName()
,	m_ControllerWidgetDefinitionName()
,	m_BindActionDefinitionName()
,	m_CompositeWidgetDefinitionName()
{
}

UIScreenRosaBindInputs::~UIScreenRosaBindInputs()
{
}

void UIScreenRosaBindInputs::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Flush();

	UIScreen::InitializeFromDefinition( DefinitionName );

	RosaFramework* const		pFramework		= RosaFramework::GetInstance();
	ASSERT( pFramework );

	InputSystem* const			pInputSystem	= pFramework->GetInputSystem();
	ASSERT( pInputSystem );

	const Array<SimpleString>&	ExposedInputs	= pInputSystem->GetExposedInputs();
	ASSERT( ExposedInputs.Size() );

	MAKEHASH( DefinitionName );

	InitializeRules();

	const uint NumExposedInputs = ExposedInputs.Size();
	for( m_ExposedInputIndex = 0; m_ExposedInputIndex < NumExposedInputs; ++m_ExposedInputIndex )
	{
		m_ExposedInput	= ExposedInputs[ m_ExposedInputIndex ];
		m_Y				= m_YBase + m_ExposedInputIndex * m_YStep;

		CreateLabelWidgetDefinition();
		CreateKeyboardWidgetDefinition();
		CreateMouseWidgetDefinition();
		CreateControllerWidgetDefinition();
		CreateBindingActionDefinition();
		CreateCompositeWidgetDefinition();
		CreateCompositeWidget();
	}

	UpdateRender();
	ResetFocus();
	RefreshWidgets();
}

void UIScreenRosaBindInputs::InitializeRules()
{
	MAKEHASH( m_Name );

	STATICHASH( Rules );
	const SimpleString UsingRules = ConfigManager::GetString( sRules, "", sm_Name );

	MAKEHASH( UsingRules );

	STATICHASH( Archetype );
	m_ArchetypeName = ConfigManager::GetString( sArchetype, "", sUsingRules );

	STATICHASH( ControllerArchetype );
	m_ControllerArchetypeName = ConfigManager::GetString( sControllerArchetype, "", sUsingRules );

	STATICHASH( Parent );
	m_Parent = ConfigManager::GetString( sParent, "", sUsingRules );

	STATICHASH( ParentHYBase );
	m_YBase = ConfigManager::GetFloat( sParentHYBase, 0.0f, sUsingRules );

	STATICHASH( ParentHYStep );
	m_YStep = ConfigManager::GetFloat( sParentHYStep, 0.0f, sUsingRules );

	STATICHASH( Column0ParentWX );
	m_Column0X = ConfigManager::GetFloat( sColumn0ParentWX, 0.0f, sUsingRules );

	STATICHASH( Column1ParentWX );
	m_Column1X = ConfigManager::GetFloat( sColumn1ParentWX, 0.0f, sUsingRules );

	STATICHASH( Column2ParentWX );
	m_Column2X = ConfigManager::GetFloat( sColumn2ParentWX, 0.0f, sUsingRules );

	STATICHASH( Column3ParentWX );
	m_Column3X = ConfigManager::GetFloat( sColumn3ParentWX, 0.0f, sUsingRules );
}

void UIScreenRosaBindInputs::CreateLabelWidgetDefinition()
{
	m_LabelWidgetDefinitionName		= SimpleString::PrintF( "_BindLabel%d", m_ExposedInputIndex );
	const SimpleString	LabelString	= SimpleString::PrintF( "Bind%s", m_ExposedInput.CStr() );

	MAKEHASH( m_LabelWidgetDefinitionName );

	STATICHASH( UIWidgetType );
	ConfigManager::SetString( sUIWidgetType, "Text", sm_LabelWidgetDefinitionName );

	STATICHASH( Extends );
	ConfigManager::SetString( sExtends, m_ArchetypeName.CStr(), sm_LabelWidgetDefinitionName );

	STATICHASH( Parent );
	ConfigManager::SetString( sParent, m_Parent.CStr(), sm_LabelWidgetDefinitionName );

	STATICHASH( String );
	ConfigManager::SetString( sString, LabelString.CStr(), sm_LabelWidgetDefinitionName );

	STATICHASH( ParentWX );
	ConfigManager::SetFloat( sParentWX, m_Column0X, sm_LabelWidgetDefinitionName );

	STATICHASH( ParentHY );
	ConfigManager::SetFloat( sParentHY, m_Y, sm_LabelWidgetDefinitionName );
}

void UIScreenRosaBindInputs::CreateKeyboardWidgetDefinition()
{
	m_KeyboardWidgetDefinitionName	= SimpleString::PrintF( "_BindKeyboard%d", m_ExposedInputIndex );
	const SimpleString InputString	= SimpleString::PrintF( "#{s:RosaKeyboard:%s}", m_ExposedInput.CStr() );

	MAKEHASH( m_KeyboardWidgetDefinitionName );

	STATICHASH( UIWidgetType );
	ConfigManager::SetString( sUIWidgetType, "Text", sm_KeyboardWidgetDefinitionName );

	STATICHASH( Extends );
	ConfigManager::SetString( sExtends, m_ArchetypeName.CStr(), sm_KeyboardWidgetDefinitionName );

	STATICHASH( Parent );
	ConfigManager::SetString( sParent, m_Parent.CStr(), sm_KeyboardWidgetDefinitionName );

	STATICHASH( IsLiteral );
	ConfigManager::SetBool( sIsLiteral, true, sm_KeyboardWidgetDefinitionName );

	STATICHASH( DynamicString );
	ConfigManager::SetString( sDynamicString, InputString.CStr(), sm_KeyboardWidgetDefinitionName );

	STATICHASH( ParentWX );
	ConfigManager::SetFloat( sParentWX, m_Column1X, sm_KeyboardWidgetDefinitionName );

	STATICHASH( ParentHY );
	ConfigManager::SetFloat( sParentHY, m_Y, sm_KeyboardWidgetDefinitionName );
}

void UIScreenRosaBindInputs::CreateMouseWidgetDefinition()
{
	m_MouseWidgetDefinitionName	= SimpleString::PrintF( "_BindMouse%d", m_ExposedInputIndex );
	const SimpleString InputString	= SimpleString::PrintF( "#{l:RosaMouse:%s}", m_ExposedInput.CStr() );

	MAKEHASH( m_MouseWidgetDefinitionName );

	STATICHASH( UIWidgetType );
	ConfigManager::SetString( sUIWidgetType, "Text", sm_MouseWidgetDefinitionName );

	// ROSANOTE: Using "controller archetype" for mouse too because they both use button glyphs.
	// I actually had this wrong on Rosa, I think.
	STATICHASH( Extends );
	ConfigManager::SetString( sExtends, m_ControllerArchetypeName.CStr(), sm_MouseWidgetDefinitionName );

	STATICHASH( Parent );
	ConfigManager::SetString( sParent, m_Parent.CStr(), sm_MouseWidgetDefinitionName );

	STATICHASH( IsLiteral );
	ConfigManager::SetBool( sIsLiteral, true, sm_MouseWidgetDefinitionName );

	STATICHASH( DynamicString );
	ConfigManager::SetString( sDynamicString, InputString.CStr(), sm_MouseWidgetDefinitionName );

	STATICHASH( ParentWX );
	ConfigManager::SetFloat( sParentWX, m_Column2X, sm_MouseWidgetDefinitionName );

	STATICHASH( ParentHY );
	ConfigManager::SetFloat( sParentHY, m_Y, sm_MouseWidgetDefinitionName );
}

void UIScreenRosaBindInputs::CreateControllerWidgetDefinition()
{
	m_ControllerWidgetDefinitionName	= SimpleString::PrintF( "_BindController%d", m_ExposedInputIndex );
	const SimpleString InputString		= SimpleString::PrintF( "#{l:RosaControllerType:%s}", m_ExposedInput.CStr() );	// Using localization to map name to preferred glyph

	MAKEHASH( m_ControllerWidgetDefinitionName );

	STATICHASH( UIWidgetType );
	ConfigManager::SetString( sUIWidgetType, "Text", sm_ControllerWidgetDefinitionName );

	STATICHASH( Extends );
	ConfigManager::SetString( sExtends, m_ControllerArchetypeName.CStr(), sm_ControllerWidgetDefinitionName );

	STATICHASH( Parent );
	ConfigManager::SetString( sParent, m_Parent.CStr(), sm_ControllerWidgetDefinitionName );

	STATICHASH( IsLiteral );
	ConfigManager::SetBool( sIsLiteral, true, sm_ControllerWidgetDefinitionName );

	STATICHASH( DynamicString );
	ConfigManager::SetString( sDynamicString, InputString.CStr(), sm_ControllerWidgetDefinitionName );

	STATICHASH( ParentWX );
	ConfigManager::SetFloat( sParentWX, m_Column3X, sm_ControllerWidgetDefinitionName );

	STATICHASH( ParentHY );
	ConfigManager::SetFloat( sParentHY, m_Y, sm_ControllerWidgetDefinitionName );
}

void UIScreenRosaBindInputs::CreateBindingActionDefinition()
{
	m_BindActionDefinitionName = SimpleString::PrintF( "_BindAction%d", m_ExposedInputIndex );

	MAKEHASH( m_BindActionDefinitionName );

	STATICHASH( ActionType );
	ConfigManager::SetString( sActionType, "RosaBindInput", sm_BindActionDefinitionName );

	STATICHASH( Input );
	ConfigManager::SetString( sInput, m_ExposedInput.CStr(), sm_BindActionDefinitionName );
}

void UIScreenRosaBindInputs::CreateCompositeWidgetDefinition()
{
	m_CompositeWidgetDefinitionName = SimpleString::PrintF( "_BindComposite%d", m_ExposedInputIndex );

	MAKEHASH( m_CompositeWidgetDefinitionName );

	STATICHASH( UIWidgetType );
	ConfigManager::SetString( sUIWidgetType, "Composite", sm_CompositeWidgetDefinitionName );

	STATICHASH( Focus );
	ConfigManager::SetBool( sFocus, true, sm_CompositeWidgetDefinitionName );

	STATICHASH( NumChildren );
	ConfigManager::SetInt( sNumChildren, 4, sm_CompositeWidgetDefinitionName );

	STATICHASH( Child0 );
	ConfigManager::SetString( sChild0, m_LabelWidgetDefinitionName.CStr(), sm_CompositeWidgetDefinitionName );

	STATICHASH( Child1 );
	ConfigManager::SetString( sChild1, m_KeyboardWidgetDefinitionName.CStr(), sm_CompositeWidgetDefinitionName );

	STATICHASH( Child2 );
	ConfigManager::SetString( sChild2, m_MouseWidgetDefinitionName.CStr(), sm_CompositeWidgetDefinitionName );

	STATICHASH( Child3 );
	ConfigManager::SetString( sChild3, m_ControllerWidgetDefinitionName.CStr(), sm_CompositeWidgetDefinitionName );

	STATICHASH( NumActions );
	ConfigManager::SetInt( sNumActions, 1, sm_CompositeWidgetDefinitionName );

	STATICHASH( Action0 );
	ConfigManager::SetString( sAction0, m_BindActionDefinitionName.CStr(), sm_CompositeWidgetDefinitionName );
}

void UIScreenRosaBindInputs::CreateCompositeWidget()
{
	UIWidget* const pCompositeWidget = UIFactory::CreateWidget( m_CompositeWidgetDefinitionName, this, NULL );
	ASSERT( pCompositeWidget );

	AddWidget( pCompositeWidget );
}
