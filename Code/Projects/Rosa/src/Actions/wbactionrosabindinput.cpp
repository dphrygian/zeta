#include "core.h"
#include "wbactionrosabindinput.h"
#include "configmanager.h"
#include "rosaframework.h"
#include "inputsystem.h"
#include "Common/uimanagercommon.h"
#include "uistack.h"
#include "uiscreen.h"

WBActionRosaBindInput::WBActionRosaBindInput()
:	m_Input()
{
}

WBActionRosaBindInput::~WBActionRosaBindInput()
{
}

/*virtual*/ void WBActionRosaBindInput::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Input );
	m_Input = ConfigManager::GetString( sInput, "", sDefinitionName );
}

/*virtual*/ void WBActionRosaBindInput::Execute()
{
	WBAction::Execute();

	RosaFramework* const		pFramework		= RosaFramework::GetInstance();
	ASSERT( pFramework );

	UIManager* const			pUIManager		= pFramework->GetUIManager();
	ASSERT( pUIManager );

	UIStack* const				pUIStack		= pUIManager->GetUIStack();
	ASSERT( pUIStack );

	STATIC_HASHED_STRING( BindDialog );

	UIScreen* const				pUIBindDialog	= pUIManager->GetScreen( sBindDialog );
	ASSERT( pUIBindDialog );

	InputSystem* const			pInputSystem	= pFramework->GetInputSystem();
	ASSERT( pInputSystem );

	pUIStack->Push( pUIBindDialog );

	pInputSystem->BindInput( m_Input );
}
