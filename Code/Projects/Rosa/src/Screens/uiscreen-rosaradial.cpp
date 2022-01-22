#include "core.h"
#include "uiscreen-rosaradial.h"
#include "keyboard.h"
#include "mouse.h"
#include "xinputcontroller.h"
#include "inputsystem.h"
#include "rosaframework.h"
#include "rosagame.h"
#include "wbeventmanager.h"
#include "wbworld.h"
#include "configmanager.h"
#include "matrix.h"
#include "vector.h"
#include "mathcore.h"
#include "Components/wbcomprosainventory.h"
#include "Components/wbcomprosaweapon.h"
#include "Widgets/uiwidget-image.h"

UIScreenRosaRadial::UIScreenRosaRadial()
:	m_Slots()
,	m_SlotDotThreshold( 0.0f )
,	m_SelectedSlot( 0 )
{
}

UIScreenRosaRadial::~UIScreenRosaRadial()
{
}

/*virtual*/ void UIScreenRosaRadial::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	UIScreen::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( NumSlots );
	const uint	NumSlots	= ConfigManager::GetInheritedInt( sNumSlots, 0, sDefinitionName );
	const float	RcpSlots	= 1.0f / static_cast<float>( NumSlots );
	m_SlotDotThreshold		= Cos( PI * RcpSlots );

	for( uint SlotIndex = 0; SlotIndex < NumSlots; ++SlotIndex )
	{
		SSlot& Slot = m_Slots.PushBack();
		Slot.m_Name = ConfigManager::GetInheritedSequenceHash( "Slot%d", SlotIndex, HashedString::NullString, sDefinitionName );

		const float		RotationRadians	= TWOPI * SlotIndex * RcpSlots;
		const Matrix	RotationMatrix	= Matrix::CreateRotationAboutZ( -RotationRadians );	// Rotate clockwise, it's more intuitive for players
		const Vector	UpVector		= Vector( 0.0f, 1.0f, 0.0f );
		const Vector	Direction		= UpVector * RotationMatrix;
		Slot.m_Direction = Vector2( Direction );

		// HACKHACK: Make assumptions about names of radial widgets
		const HashedString WidgetName = SimpleString::PrintF( "RadialSlot%d", SlotIndex );
		Slot.m_Widget = GetWidget<UIWidgetImage>( WidgetName );
		DEVASSERT( Slot.m_Widget );

		const float X = 0.5f * ( 1.0f + Slot.m_Direction.x );
		const float Y = 0.5f * ( 1.0f - Slot.m_Direction.y );
		Slot.m_Widget->SetParentLocation( X, Y );
	}
}

/*virtual*/ void UIScreenRosaRadial::Pushed()
{
	UIScreen::Pushed();

	WBEntity* const				pPlayer		= RosaGame::GetPlayer();
	DEVASSERT( pPlayer );

	WBCompRosaInventory* const	pInventory	= WB_GETCOMP( pPlayer, RosaInventory );
	DEVASSERT( pInventory );

	const HashedString			SlotName	= pInventory->GetCycleSlot();
	DEVASSERT( SlotName != HashedString::NullString );

	FOR_EACH_ARRAY( SlotIter, m_Slots, SSlot )
	{
		SSlot&			Slot		= SlotIter.GetValue();
		const uint		SlotIndex	= SlotIter.GetIndex();

		WBEntity* const	pItem		= pInventory->GetItem( Slot.m_Name );
		if( !pItem )
		{
			Slot.m_Widget->SetHidden( true );
			Slot.m_Hidden = true;
			continue;
		}

		WBCompRosaWeapon* const pWeapon = WB_GETCOMP( pItem, RosaWeapon );
		DEVASSERT( pWeapon );
		Slot.m_Widget->SetTexture( pWeapon->GetIcon(), 0 );
		Slot.m_Widget->SetHidden( false );
		Slot.m_Hidden = false;

		if( Slot.m_Name == SlotName )
		{
			DEVASSERT( pItem );
			SelectSlot( SlotIndex );
		}
	}
}

void UIScreenRosaRadial::SelectSlot( const uint SlotIndex )
{
	m_SelectedSlot	= SlotIndex;
	SetFocus( m_Slots[ SlotIndex ].m_Widget );
}

// NOTE: Because I *may* want this screen to pause the game, I can't rely
// on the input system. But it also shouldn't use conventional UI controls.
// So instead, query the input system for bindings but check input locally.
// Ugly, but necessary, and I don't see any reason this will grow worse.
/*virtual*/ UIScreen::ETickReturn UIScreenRosaRadial::Tick( const float DeltaTime, bool HasFocus )
{
	// The radial screen should use the input system, and as such, should never receive UI focus.
	DEVASSERT( !HasFocus );

	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	Keyboard* const			pKeyboard		= pFramework->GetKeyboard();
	Mouse* const			pMouse			= pFramework->GetMouse();
	XInputController* const	pController		= pFramework->GetController();
	InputSystem* const		pInputSystem	= pFramework->GetInputSystem();

	static const float sMouseScalar = 0.0625f;
	const float X =
		sMouseScalar * pMouse->GetPosition( Mouse::EA_X ) +
		pController->GetPosition( XInputController::EA_LeftThumbX ) +
		pController->GetPosition( XInputController::EA_RightThumbX );
	const float Y =
		-1.0f * sMouseScalar * pMouse->GetPosition( Mouse::EA_Y ) +
		pController->GetPosition( XInputController::EA_LeftThumbY ) +
		pController->GetPosition( XInputController::EA_RightThumbY );

	static const float	sMagSqThreshold	= Square( 0.333f );
	const Vector2		Offset			= Vector2( X, Y );
	const float			MagSq			= Offset.LengthSquared();
	if( MagSq >= sMagSqThreshold )
	{
		const Vector2	Normal			= Offset.GetNormalized();
		FOR_EACH_ARRAY( SlotIter, m_Slots, SSlot )
		{
			const SSlot&	Slot		= SlotIter.GetValue();
			const uint		SlotIndex	= SlotIter.GetIndex();

			if( Slot.m_Hidden )
			{
				continue;
			}

			if( SlotIndex == m_SelectedSlot )
			{
				continue;
			}

			if( Normal.Dot( Slot.m_Direction ) < m_SlotDotThreshold )
			{
				continue;
			}

			SelectSlot( SlotIndex );
		}
	}

	STATIC_HASHED_STRING( Radial );
	const uint Keyboard_Radial		= pInputSystem->GetBoundKeyboardSignal(		sRadial );
	const uint Mouse_Radial			= pInputSystem->GetBoundMouseSignal(		sRadial );
	const uint Controller_Radial	= pInputSystem->GetBoundControllerSignal(	sRadial );

	const bool RadialIsHigh =
		( Keyboard_Radial	> Keyboard::EB_None			&& pKeyboard->IsHigh(	Keyboard_Radial ) )		||
		( Mouse_Radial		> Mouse::EB_None			&& pMouse->IsHigh(		Mouse_Radial ) )		||
		( Controller_Radial	> XInputController::EB_None	&& pController->IsHigh(	Controller_Radial ) );

	if( !RadialIsHigh )
	{
		// It should be fine to just pop the screen, but let's be safe and explicitly remove it.
		STATIC_HASHED_STRING( RadialScreen );
		WB_MAKE_EVENT( RemoveUIScreen, NULL );
		WB_SET_AUTO( RemoveUIScreen, Hash, Screen, sRadialScreen );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), RemoveUIScreen, NULL );

		// Cycle to the selected slot
		// ROSATODO: Maybe introduce a neutral state that doesn't cycle?
		WB_MAKE_EVENT( RequestCycleToSlot, NULL );
		WB_SET_AUTO( RequestCycleToSlot, Hash, Slot, m_Slots[ m_SelectedSlot ].m_Name );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), RequestCycleToSlot, RosaGame::GetPlayer() );
	}

	return UIScreen::Tick( DeltaTime, HasFocus );
}
