#include "core.h"
#include "inputsystem.h"
#include "keyboard.h"
#include "mouse.h"
#include "xinputcontroller.h"
#include "simplestring.h"
#include "configmanager.h"
#include "mathcore.h"
#include "iinputsystemobserver.h"
#include "clock.h"
#include "reversehash.h"

// TODO: Incorporate control modes (legacy/southpaw) here (could be done with axial bindings).

InputSystem::InputSystem()
:	m_Keyboard( NULL )
,	m_Mouse( NULL )
,	m_Controller( NULL )
,	m_Clock( NULL )
,	m_DefinitionName()
,	m_IsUsingControllerExclusively( false )
,	m_ShouldPauseDueToLostDevice( false )
,	m_KeyboardContext()
,	m_MouseContext()
,	m_ControllerContext()
,	m_ControllerTypeContext()
,	m_RcpRelativeFrameTime( 0.0f )
,	m_Inputs()
,	m_AnalogInputs()
,	m_ExposedInputs()
,	m_DisplayInputs()
,	m_InputContexts()
,	m_ActiveInputContexts()
,	m_KeyboardBindings()
,	m_MouseBindings()
,	m_ControllerBindings()
,	m_MouseAnalogBindings()
,	m_ControllerAnalogBindings()
,	m_MouseAdjustments()
,	m_ControllerAdjustments()
,	m_ControllerTypes()
,	m_ControllerType()
,	m_ControllerGears()
,	m_UnbindableKeyboardInputs()
,	m_UnbindableMouseInputs()
,	m_UnbindableControllerInputs()
,	m_UnbindableMouseAnalogInputs()
,	m_UnbindableControllerAnalogInputs()
,	m_KeyboardMap()
,	m_ReverseKeyboardMap()
,	m_MouseMap()
,	m_ReverseMouseMap()
,	m_ControllerMap()
,	m_ReverseControllerMap()
,	m_MouseAnalogMap()
,	m_ReverseMouseAnalogMap()
,	m_ControllerAnalogMap()
,	m_ReverseControllerAnalogMap()
,	m_HoldTime( 0.0f )
,	m_StateA()
,	m_StateB()
,	m_CurrentState( NULL )
,	m_LastState( NULL )
,	m_HoldState()
,	m_AnalogStateA()
,	m_AnalogStateB()
,	m_CurrentAnalogState()
,	m_LastAnalogState()
,	m_Binding( false )
,	m_BindingInput()
,	m_CanBindKeyboard( false )
,	m_CanBindMouse( false )
,	m_CanBindController( false )
,	m_CanBindMouseAnalog( false )
,	m_CanBindControllerAnalog( false )
,	m_InputSystemObservers()
{
}

InputSystem::~InputSystem()
{
	FOR_EACH_MAP( InputContextIter, m_InputContexts, HashedString, InputContext* )
	{
		InputContext* pInputContext = InputContextIter.GetValue();
		SafeDelete( pInputContext );
	}
}

void InputSystem::Initialize( const SimpleString& DefinitionName )
{
	XTRACE_FUNCTION;

	m_DefinitionName = DefinitionName;

	InitializeSignalMaps();

	MAKEHASH( DefinitionName );

	STATICHASH( HoldTime );
	m_HoldTime = ConfigManager::GetFloat( sHoldTime, 0.0f, sDefinitionName );
	DEVASSERT( m_HoldTime > 0.0f );

	STATICHASH( NumExposedInputs );
	const uint NumExposedInputs = ConfigManager::GetInt( sNumExposedInputs, 0, sDefinitionName );
	for( uint ExposedInputIndex = 0; ExposedInputIndex < NumExposedInputs; ++ExposedInputIndex )
	{
		const SimpleString ExposedInput = ConfigManager::GetSequenceString( "ExposedInput%d", ExposedInputIndex, "", sDefinitionName );
		m_ExposedInputs.PushBack( ExposedInput );
	}

	STATICHASH( NumDisplayInputs );
	const uint NumDisplayInputs = ConfigManager::GetInt( sNumDisplayInputs, 0, sDefinitionName );
	for( uint DisplayInputIndex = 0; DisplayInputIndex < NumDisplayInputs; ++DisplayInputIndex )
	{
		SDisplayInput& DisplayInput = m_DisplayInputs.PushBack();
		DisplayInput.m_Name				= ConfigManager::GetSequenceString( "DisplayInput%dName", DisplayInputIndex, "", sDefinitionName );
		DisplayInput.m_KeyboardInput	= GetKeyboardSignal(	ConfigManager::GetSequenceHash( "DisplayInput%dKeyboardInput",		DisplayInputIndex, HashedString::NullString, sDefinitionName ) );
		DisplayInput.m_MouseInput		= GetMouseSignal(		ConfigManager::GetSequenceHash( "DisplayInput%dMouseInput",			DisplayInputIndex, HashedString::NullString, sDefinitionName ) );
		DisplayInput.m_ControllerInput	= GetControllerSignal(	ConfigManager::GetSequenceHash( "DisplayInput%dControllerInput",	DisplayInputIndex, HashedString::NullString, sDefinitionName ) );
	}

	STATICHASH( NumInputs );
	const uint NumInputs = ConfigManager::GetInt( sNumInputs, 0, sDefinitionName );
	for( uint InputIndex = 0; InputIndex < NumInputs; ++InputIndex )
	{
		const SimpleString Input = ConfigManager::GetSequenceString( "Input%d", InputIndex, "", sDefinitionName );

		SInput NewInput;
		NewInput.m_String = Input;
		NewInput.m_Hash = Input;

		m_Inputs.PushBack( NewInput );
		m_StateA[ NewInput.m_Hash ] = false;
		m_StateB[ NewInput.m_Hash ] = false;
	}

	STATICHASH( NumAnalogInputs );
	const uint NumAnalogInputs = ConfigManager::GetInt( sNumAnalogInputs, 0, sDefinitionName );
	for( uint AnalogInputIndex = 0; AnalogInputIndex < NumAnalogInputs; ++AnalogInputIndex )
	{
		const SimpleString Input = ConfigManager::GetSequenceString( "AnalogInput%d", AnalogInputIndex, "", sDefinitionName );

		SAnalogInput NewInput;
		NewInput.m_String = Input;
		NewInput.m_Hash = Input;
		NewInput.m_Clamp	= ConfigManager::GetSequenceBool( "AnalogInput%dClamp", AnalogInputIndex, false, sDefinitionName );
		NewInput.m_ClampMin	= ConfigManager::GetSequenceFloat( "AnalogInput%dClampMin", AnalogInputIndex, 0.0f, sDefinitionName );
		NewInput.m_ClampMax	= ConfigManager::GetSequenceFloat( "AnalogInput%dClampMax", AnalogInputIndex, 0.0f, sDefinitionName );

		m_AnalogInputs.PushBack( NewInput );
		m_AnalogStateA[ NewInput.m_Hash ] = 0.0f;
		m_AnalogStateB[ NewInput.m_Hash ] = 0.0f;

		m_MouseAdjustments.Insert(		NewInput.m_Hash, SAnalogAdjustment() );
		m_ControllerAdjustments.Insert(	NewInput.m_Hash, SAnalogAdjustment() );

		m_ControllerGears.Insert(		NewInput.m_Hash, SControllerGear() );
	}

	STATICHASH( NumInputContexts );
	const uint NumInputContexts = ConfigManager::GetInt( sNumInputContexts, 0, sDefinitionName );
	for( uint InputContextIndex = 0; InputContextIndex < NumInputContexts; ++InputContextIndex )
	{
		const SimpleString InputContextName = ConfigManager::GetSequenceString( "InputContext%d", InputContextIndex, "", sDefinitionName );
		DEVASSERT( m_InputContexts.Search( InputContextName ).IsNull() );

		InputContext* const pInputContext = new InputContext;
		pInputContext->Initialize( InputContextName, this );
		m_InputContexts[ InputContextName ] = pInputContext;
	}

	STATICHASH( NumControllerTypes );
	const uint NumControllerTypes = ConfigManager::GetInt( sNumControllerTypes, 0, sDefinitionName );
	for( uint ControllerTypeIndex = 0; ControllerTypeIndex < NumControllerTypes; ++ControllerTypeIndex )
	{
		const SimpleString ControllerTypeName = ConfigManager::GetSequenceString( "ControllerType%d", ControllerTypeIndex, "", sDefinitionName );

		SControllerType& ControllerType = m_ControllerTypes.PushBack();
		ControllerType.m_Name = ControllerTypeName;

		MAKEHASH( ControllerTypeName );

#define INSERT_PREFERRED_SIGNAL_NAME( signal ) \
		{ \
			STATICHASH( signal ); \
			const SimpleString PreferredSignalName = ConfigManager::GetString( s##signal, "", sControllerTypeName ); \
			if( PreferredSignalName != "" ) \
			{ \
				ControllerType.m_Map.Insert( s##signal, PreferredSignalName ); \
			} \
		}

		INSERT_PREFERRED_SIGNAL_NAME( xA );
		INSERT_PREFERRED_SIGNAL_NAME( xB );
		INSERT_PREFERRED_SIGNAL_NAME( xX );
		INSERT_PREFERRED_SIGNAL_NAME( xY );
		INSERT_PREFERRED_SIGNAL_NAME( xUp );
		INSERT_PREFERRED_SIGNAL_NAME( xDown );
		INSERT_PREFERRED_SIGNAL_NAME( xLeft );
		INSERT_PREFERRED_SIGNAL_NAME( xRight );
		INSERT_PREFERRED_SIGNAL_NAME( LT );
		INSERT_PREFERRED_SIGNAL_NAME( RT );
		INSERT_PREFERRED_SIGNAL_NAME( LB );
		INSERT_PREFERRED_SIGNAL_NAME( RB );
		INSERT_PREFERRED_SIGNAL_NAME( LS );
		INSERT_PREFERRED_SIGNAL_NAME( RS );

#undef INSERT_PREFERRED_SIGNAL_NAME
	}

	STATICHASH( ControllerType );
	const HashedString ControllerType = ConfigManager::GetHash( sControllerType, HashedString::NullString, sDefinitionName );
	FOR_EACH_ARRAY( ControllerTypeIter, m_ControllerTypes, SControllerType )
	{
		if( ControllerType == ControllerTypeIter.GetValue().m_Name )
		{
			m_ControllerType = ControllerTypeIter.GetIndex();
			break;
		}
	}

	STATICHASH( KeyboardContext );
	m_KeyboardContext	= ConfigManager::GetString( sKeyboardContext, "", sDefinitionName );

	STATICHASH( MouseContext );
	m_MouseContext		= ConfigManager::GetString( sMouseContext, "", sDefinitionName );

	STATICHASH( ControllerContext );
	m_ControllerContext	= ConfigManager::GetString( sControllerContext, "", sDefinitionName );

	STATICHASH( ControllerTypeContext );
	m_ControllerTypeContext = ConfigManager::GetString( sControllerTypeContext, "", sDefinitionName );

	STATICHASH( NumUnbindableInputs );
	STATICHASH( NumUnbindableAnalogInputs );

	MAKEHASH( m_KeyboardContext );
	const uint NumUnbindableKeyboardInputs = ConfigManager::GetInt( sNumUnbindableInputs, 0, sm_KeyboardContext );
	for( uint UnbindableInputIndex = 0; UnbindableInputIndex < NumUnbindableKeyboardInputs; ++UnbindableInputIndex )
	{
		m_UnbindableKeyboardInputs.Insert( ConfigManager::GetSequenceHash( "UnbindableInput%d", UnbindableInputIndex, HashedString::NullString, sm_KeyboardContext ) );
	}

	MAKEHASH( m_MouseContext );
	const uint NumUnbindableMouseInputs = ConfigManager::GetInt( sNumUnbindableInputs, 0, sm_MouseContext );
	for( uint UnbindableInputIndex = 0; UnbindableInputIndex < NumUnbindableMouseInputs; ++UnbindableInputIndex )
	{
		m_UnbindableMouseInputs.Insert( ConfigManager::GetSequenceHash( "UnbindableInput%d", UnbindableInputIndex, HashedString::NullString, sm_MouseContext ) );
	}
	const uint NumUnbindableMouseAnalogInputs = ConfigManager::GetInt( sNumUnbindableAnalogInputs, 0, sm_MouseContext );
	for( uint UnbindableInputIndex = 0; UnbindableInputIndex < NumUnbindableMouseAnalogInputs; ++UnbindableInputIndex )
	{
		m_UnbindableMouseAnalogInputs.Insert( ConfigManager::GetSequenceHash( "UnbindableAnalogInput%d", UnbindableInputIndex, HashedString::NullString, sm_MouseContext ) );
	}

	MAKEHASH( m_ControllerContext );
	const uint NumUnbindableControllerInputs = ConfigManager::GetInt( sNumUnbindableInputs, 0, sm_ControllerContext );
	for( uint UnbindableInputIndex = 0; UnbindableInputIndex < NumUnbindableControllerInputs; ++UnbindableInputIndex )
	{
		m_UnbindableControllerInputs.Insert( ConfigManager::GetSequenceHash( "UnbindableInput%d", UnbindableInputIndex, HashedString::NullString, sm_ControllerContext ) );
	}
	const uint NumUnbindableControllerAnalogInputs = ConfigManager::GetInt( sNumUnbindableAnalogInputs, 0, sm_ControllerContext );
	for( uint UnbindableInputIndex = 0; UnbindableInputIndex < NumUnbindableControllerAnalogInputs; ++UnbindableInputIndex )
	{
		m_UnbindableControllerAnalogInputs.Insert( ConfigManager::GetSequenceHash( "UnbindableAnalogInput%d", UnbindableInputIndex, HashedString::NullString, sm_ControllerContext ) );
	}

	STATICHASH( NumGearConfigs );
	const uint NumGearConfigs = ConfigManager::GetInt( sNumGearConfigs, 0, sm_ControllerContext );
	for( uint GearConfigIndex = 0; GearConfigIndex < NumGearConfigs; ++GearConfigIndex )
	{
		const HashedString	GearConfigInput	= ConfigManager::GetSequenceHash( "GearConfig%dInput", GearConfigIndex, HashedString::NullString, sm_ControllerContext );

		DEVASSERT( m_ControllerGears.Search( GearConfigInput ).IsValid() );
		SControllerGear& Gear = m_ControllerGears[ GearConfigInput ];

		Gear.m_HighGearScalar		= ConfigManager::GetSequenceFloat( "GearConfig%dHighScalar", GearConfigIndex, 1.0f, sm_ControllerContext );
		Gear.m_HighGearThreshold	= ConfigManager::GetSequenceFloat( "GearConfig%dHighThreshold", GearConfigIndex, 0.0f, sm_ControllerContext );
		Gear.m_HighGearShiftTime	= ConfigManager::GetSequenceFloat( "GearConfig%dHighShiftTime", GearConfigIndex, 0.0f, sm_ControllerContext );
		Gear.m_LowGearScalar		= ConfigManager::GetSequenceFloat( "GearConfig%dLowScalar", GearConfigIndex, 1.0f, sm_ControllerContext );
		Gear.m_LowGearThreshold		= ConfigManager::GetSequenceFloat( "GearConfig%dLowThreshold", GearConfigIndex, 0.0f, sm_ControllerContext );
		Gear.m_LowGearShiftTime		= ConfigManager::GetSequenceFloat( "GearConfig%dLowShiftTime", GearConfigIndex, 0.0f, sm_ControllerContext );
	}

	UpdateBindingsFromConfig();
}

void InputSystem::UpdateBindingsFromConfig()
{
	XTRACE_FUNCTION;

	MAKEHASH( m_KeyboardContext );
	MAKEHASH( m_MouseContext );
	MAKEHASH( m_ControllerContext );

	const uint NumInputs = m_Inputs.Size();
	for( uint InputIndex = 0; InputIndex < NumInputs; ++InputIndex )
	{
		const SInput& Input = m_Inputs[ InputIndex ];
		MAKEHASHFROM( InputName, Input.m_String );

		m_KeyboardBindings[ Input.m_Hash ]		= GetKeyboardSignal(	ConfigManager::GetHash( sInputName, HashedString::NullString, sm_KeyboardContext ) );
		m_MouseBindings[ Input.m_Hash ]			= GetMouseSignal(		ConfigManager::GetHash( sInputName, HashedString::NullString, sm_MouseContext ) );
		m_ControllerBindings[ Input.m_Hash ]	= GetControllerSignal(	ConfigManager::GetHash( sInputName, HashedString::NullString, sm_ControllerContext ) );
	}

	const uint NumAnalogInputs = m_AnalogInputs.Size();
	for( uint AnalogInputIndex = 0; AnalogInputIndex < NumAnalogInputs; ++AnalogInputIndex )
	{
		const SAnalogInput& Input = m_AnalogInputs[ AnalogInputIndex ];
		MAKEHASHFROM( InputName, Input.m_String );

		m_MouseAnalogBindings[ Input.m_Hash ]		= GetMouseAnalogSignal(			ConfigManager::GetHash( sInputName, HashedString::NullString, sm_MouseContext ) );
		m_ControllerAnalogBindings[ Input.m_Hash ]	= GetControllerAnalogSignal(	ConfigManager::GetHash( sInputName, HashedString::NullString, sm_ControllerContext ) );
	}

	PublishInputStrings();
}

void InputSystem::Tick()
{
	PROFILE_FUNCTION;

	UpdateIsUsingControllerExclusively();

	if( m_Binding )
	{
		TickBinding();
		return;
	}

	m_CurrentState	= ( m_CurrentState == &m_StateA ) ? &m_StateB : &m_StateA;
	m_LastState		= ( m_CurrentState == &m_StateA ) ? &m_StateB : &m_StateA;

	m_CurrentAnalogState	= ( m_CurrentAnalogState == &m_AnalogStateA ) ? &m_AnalogStateB : &m_AnalogStateA;
	m_LastAnalogState		= ( m_CurrentAnalogState == &m_AnalogStateA ) ? &m_AnalogStateB : &m_AnalogStateA;

	// Assume that devices have already been ticked. InputSystem doesn't own them, it just utilizes them.
	InputState&			CurrentState		= *m_CurrentState;
	AnalogInputState&	CurrentAnalogState	= *m_CurrentAnalogState;

	const uint NumInputs = m_Inputs.Size();
	const uint NumAnalogInputs = m_AnalogInputs.Size();

	// Clear all states first, in case their inputs are suppressed or remapped or whatever.
	for( uint InputIndex = 0; InputIndex < NumInputs; ++InputIndex )
	{
		const SInput& Input = m_Inputs[ InputIndex ];
		CurrentState[ Input.m_Hash ] = false;
	}

	for( uint AnalogInputIndex = 0; AnalogInputIndex < NumAnalogInputs; ++AnalogInputIndex )
	{
		const SAnalogInput& Input = m_AnalogInputs[ AnalogInputIndex ];
		CurrentAnalogState[ Input.m_Hash ] = 0.0f;
	}

	// Then update the states from contextual inputs.
	for( uint InputIndex = 0; InputIndex < NumInputs; ++InputIndex )
	{
		const SInput&		Input			= m_Inputs[ InputIndex ];
		const HashedString	OriginalInput	= Input.m_Hash;
		const HashedString	ContextualInput	= GetContextualInput( OriginalInput );

		if( ContextualInput == HashedString::NullString )
		{
			continue;
		}

		const uint KeyboardSignal	= GetBoundKeyboardSignal(	OriginalInput );
		const uint MouseSignal		= GetBoundMouseSignal(		OriginalInput );
		const uint ControllerSignal	= GetBoundControllerSignal(	OriginalInput );

		CurrentState[ ContextualInput ] =
			( m_Keyboard	&& KeyboardSignal > 0	&& m_Keyboard->IsHigh(		KeyboardSignal		) ) ||
			( m_Mouse		&& MouseSignal > 0		&& m_Mouse->IsHigh(			MouseSignal			) ) ||
			( m_Controller	&& ControllerSignal > 0	&& m_Controller->IsHigh(	ControllerSignal	) );

		// Update hold state
		SHoldState& HoldState = m_HoldState[ ContextualInput ];
		HoldState.m_Held = false;
		if( IsHigh( ContextualInput ) )
		{
			const float CurrentTime = GetTime();

			// On rise, mark the rise time for comparing to hold time.
			if( OnRise( ContextualInput ) )
			{
				HoldState.m_RiseTime		= CurrentTime;
				HoldState.m_WaitingForHold	= true;
			}

			const float HeldTime = CurrentTime - HoldState.m_RiseTime;
			if( HoldState.m_WaitingForHold && HeldTime >= m_HoldTime )
			{
				// Held flag will be high for only a single tick.
				HoldState.m_Held			= true;
				HoldState.m_WaitingForHold	= false;
			}
		}
	}

	// HACKHACK: Normalize relative to expected frame time (e.g. 60fps).
	// This is so that mouse should behave correctly when fixed frame time is disabled.
	const float kDeltaTime			= GetClock()->GetGameDeltaTime();
	const float kMouseAdjustment	= ( m_RcpRelativeFrameTime > 0.0f && kDeltaTime > 0.0f ) ? ( 1.0f / ( m_RcpRelativeFrameTime * kDeltaTime ) ) : 1.0f;

	for( uint AnalogInputIndex = 0; AnalogInputIndex < NumAnalogInputs; ++AnalogInputIndex )
	{
		const SAnalogInput&	Input			= m_AnalogInputs[ AnalogInputIndex ];
		const HashedString	OriginalInput	= Input.m_Hash;
		const HashedString	ContextualInput	= GetContextualInput( OriginalInput );

		if( ContextualInput == HashedString::NullString )
		{
			continue;
		}

		float& State = CurrentAnalogState[ ContextualInput ];
		State = 0.0f;

		const uint MouseAnalogSignal = GetBoundMouseAnalogSignal( OriginalInput );
		if( m_Mouse && MouseAnalogSignal > 0 )
		{
			DEBUGASSERT( m_MouseAdjustments.Search( OriginalInput ).IsValid() );
			const SAnalogAdjustment& Adjustment = m_MouseAdjustments[ OriginalInput ];

			State +=
				m_Mouse->GetPosition( MouseAnalogSignal ) *
				Adjustment.m_ScaleFactor *
				( Adjustment.m_InvertFactor ? -1.0f : 1.0f ) *
				kMouseAdjustment;
		}

		const uint ControllerAnalogSignal = GetBoundControllerAnalogSignal( OriginalInput );
		if( m_Controller && ControllerAnalogSignal > 0 )
		{
			const float InputPosition			= m_Controller->GetPosition( ControllerAnalogSignal );

			DEBUGASSERT( m_ControllerAdjustments.Search( OriginalInput ).IsValid() );
			const SAnalogAdjustment& Adjustment = m_ControllerAdjustments[ OriginalInput ];
			const float InvertFactor			= Adjustment.m_InvertFactor ? -1.0f : 1.0f;
			const float ModifiedInputPosition	= ModifyControllerInputPosition( InputPosition, Adjustment );

			SControllerGear& Gear = m_ControllerGears[ OriginalInput ];
			TickControllerGear( Gear, InputPosition );
			const float GearScalar				= Gear.m_HighGear ? Gear.m_HighGearScalar : Gear.m_LowGearScalar;

			State +=
				ModifiedInputPosition *
				Adjustment.m_ScaleFactor *
				GearScalar *
				InvertFactor;
		}

		if( Input.m_Clamp )
		{
			State = Clamp( State, Input.m_ClampMin, Input.m_ClampMax );
		}
	}
}

float InputSystem::ModifyControllerInputPosition( const float InputPosition, const SAnalogAdjustment& Adjustment )
{
	if( Adjustment.m_PowerFactor == 1.0f )
	{
		return InputPosition;
	}

	const float AbsInput	= Abs( InputPosition );
	const float PowInput	= Pow( AbsInput, Adjustment.m_PowerFactor );
	const float InputSign	= Sign( InputPosition );

	return InputSign * PowInput;
}

void InputSystem::TickControllerGear( SControllerGear& Gear, const float InputPosition )
{
	// HACKHACK: If high gear threshold is zero, we're ignoring gears
	if( Gear.m_HighGearThreshold <= 0.0f )
	{
		return;
	}

	if( Gear.m_HighGear )
	{
		if( Abs( InputPosition ) <= Gear.m_LowGearThreshold )
		{
			if( !Gear.m_Shifting )
			{
				Gear.m_Shifting			= true;
				Gear.m_ShiftStartTime	= GetTime();
			}

			if( GetTime() - Gear.m_ShiftStartTime >= Gear.m_LowGearShiftTime )
			{
				Gear.m_HighGear = false;
				Gear.m_Shifting = false;
			}
		}
		else
		{
			Gear.m_Shifting = false;
		}
	}
	else
	{
		if( Abs( InputPosition ) >= Gear.m_HighGearThreshold )
		{
			if( !Gear.m_Shifting )
			{
				Gear.m_Shifting			= true;
				Gear.m_ShiftStartTime	= GetTime();
			}

			if( GetTime() - Gear.m_ShiftStartTime >= Gear.m_HighGearShiftTime )
			{
				Gear.m_HighGear = true;
				Gear.m_Shifting = false;
			}
		}
		else
		{
			Gear.m_Shifting = false;
		}
	}
}

void InputSystem::TickBinding()
{
	if(
		( m_Keyboard && m_Keyboard->OnRise( Keyboard::EB_Escape ) ) ||
		( m_Controller && m_Controller->OnRise( XInputController::EB_Start ) ) ||
		( m_Controller && !m_CanBindController && m_Controller->OnRise( XInputController::EB_B ) )
		)
	{
		// Cancel
		m_Binding = false;
	}
#if 0	// Removing unbinding for now
	else if(
		( m_Keyboard && m_Keyboard->OnRise( Keyboard::EB_Backspace ) ) ||
		( m_Controller && m_Controller->OnRise( XInputController::EB_Back ) )
		)
	{
		// Unbind
		MAKEHASH( m_KeyboardContext );
		MAKEHASH( m_MouseContext );
		MAKEHASH( m_ControllerContext );
		MAKEHASH( m_BindingInput );

		if( m_BindingInputIsAnalog )
		{
			ConfigManager::SetString( sm_BindingInput, GetMouseAnalogSignalName( Mouse::EA_None ).CStr(), sm_MouseContext );
			ConfigManager::SetString( sm_BindingInput, GetControllerAnalogSignalName( XInputController::EA_None ).CStr(), sm_ControllerContext );
		}
		else
		{
			ConfigManager::SetString( sm_BindingInput, GetKeyboardSignalName( 0 ).CStr(), sm_KeyboardContext );
			ConfigManager::SetString( sm_BindingInput, GetMouseSignalName( Mouse::EB_None ).CStr(), sm_MouseContext );
			ConfigManager::SetString( sm_BindingInput, GetControllerSignalName( XInputController::EB_None ).CStr(), sm_ControllerContext );
		}

		m_Binding = false;
	}
#endif
	else
	{
		if( m_Keyboard && m_CanBindKeyboard )
		{
			for( uint Key = Keyboard::EB_A; Key <= Keyboard::EB_Z; ++Key )
			{
				TryKeyboardBind( Key );
			}
			TryKeyboardBind( Keyboard::EB_Space );
			TryKeyboardBind( Keyboard::EB_LeftShift );
			TryKeyboardBind( Keyboard::EB_LeftControl );
			TryKeyboardBind( Keyboard::EB_LeftAlt );
			TryKeyboardBind( Keyboard::EB_RightShift );
			TryKeyboardBind( Keyboard::EB_RightControl );
			TryKeyboardBind( Keyboard::EB_RightAlt );
			TryKeyboardBind( Keyboard::EB_CapsLock );
			TryKeyboardBind( Keyboard::EB_Tab );
			TryKeyboardBind( Keyboard::EB_Comma );
			TryKeyboardBind( Keyboard::EB_Period );
			TryKeyboardBind( Keyboard::EB_Slash );
			TryKeyboardBind( Keyboard::EB_Semicolon );
			TryKeyboardBind( Keyboard::EB_Apostrophe );
			TryKeyboardBind( Keyboard::EB_LeftBrace );
			TryKeyboardBind( Keyboard::EB_RightBrace );
			TryKeyboardBind( Keyboard::EB_Backslash );
			TryKeyboardBind( Keyboard::EB_Grave );
			TryKeyboardBind( Keyboard::EB_Up );
			TryKeyboardBind( Keyboard::EB_Down );
			TryKeyboardBind( Keyboard::EB_Left );
			TryKeyboardBind( Keyboard::EB_Right );
			TryKeyboardBind( Keyboard::EB_Num0 );
			TryKeyboardBind( Keyboard::EB_Num1 );
			TryKeyboardBind( Keyboard::EB_Num2 );
			TryKeyboardBind( Keyboard::EB_Num3 );
			TryKeyboardBind( Keyboard::EB_Num4 );
			TryKeyboardBind( Keyboard::EB_Num5 );
			TryKeyboardBind( Keyboard::EB_Num6 );
			TryKeyboardBind( Keyboard::EB_Num7 );
			TryKeyboardBind( Keyboard::EB_Num8 );
			TryKeyboardBind( Keyboard::EB_Num9 );
			TryKeyboardBind( Keyboard::EB_NumDecimal );
			TryKeyboardBind( Keyboard::EB_NumMultiply );
			TryKeyboardBind( Keyboard::EB_NumAdd );
			TryKeyboardBind( Keyboard::EB_NumSubtract );
			TryKeyboardBind( Keyboard::EB_NumDivide );
			TryKeyboardBind( Keyboard::EB_Enter );
			TryKeyboardBind( Keyboard::EB_Insert );
			TryKeyboardBind( Keyboard::EB_Home );
			TryKeyboardBind( Keyboard::EB_PageUp );
			TryKeyboardBind( Keyboard::EB_Delete );
			TryKeyboardBind( Keyboard::EB_End );
			TryKeyboardBind( Keyboard::EB_PageDown );
			TryKeyboardBind( Keyboard::EB_1 );
			TryKeyboardBind( Keyboard::EB_2 );
			TryKeyboardBind( Keyboard::EB_3 );
			TryKeyboardBind( Keyboard::EB_4 );
			TryKeyboardBind( Keyboard::EB_5 );
			TryKeyboardBind( Keyboard::EB_6 );
			TryKeyboardBind( Keyboard::EB_7 );
			TryKeyboardBind( Keyboard::EB_8 );
			TryKeyboardBind( Keyboard::EB_9 );
			TryKeyboardBind( Keyboard::EB_0 );
			TryKeyboardBind( Keyboard::EB_F1 );
			TryKeyboardBind( Keyboard::EB_F2 );
			TryKeyboardBind( Keyboard::EB_F3 );
			TryKeyboardBind( Keyboard::EB_F4 );
			TryKeyboardBind( Keyboard::EB_F5 );
			TryKeyboardBind( Keyboard::EB_F6 );
			TryKeyboardBind( Keyboard::EB_F7 );
			TryKeyboardBind( Keyboard::EB_F8 );
			TryKeyboardBind( Keyboard::EB_F9 );
			TryKeyboardBind( Keyboard::EB_F10 );
			TryKeyboardBind( Keyboard::EB_F11 );
			TryKeyboardBind( Keyboard::EB_F12 );
			TryKeyboardBind( Keyboard::EB_Minus );
			TryKeyboardBind( Keyboard::EB_Plus );
			TryKeyboardBind( Keyboard::EB_Backspace );
		}

		if( m_Mouse && m_CanBindMouse )
		{
			TryMouseBind( Keyboard::EB_Mouse_Left,		Mouse::EB_Left );
			TryMouseBind( Keyboard::EB_Mouse_Middle,	Mouse::EB_Middle );
			TryMouseBind( Keyboard::EB_Mouse_Right,		Mouse::EB_Right );
			TryMouseBind( Keyboard::EB_Mouse_X1,		Mouse::EB_X1 );
			TryMouseBind( Keyboard::EB_Mouse_X2,		Mouse::EB_X2 );
			TryMouseBind( Keyboard::EB_None,			Mouse::EB_WheelUp );
			TryMouseBind( Keyboard::EB_None,			Mouse::EB_WheelDown );
		}

		if( m_Mouse && m_CanBindMouseAnalog )
		{
			TryMouseBindAnalog( Mouse::EA_X );
			TryMouseBindAnalog( Mouse::EA_Y );
		}

		if( m_Controller && m_CanBindController )
		{
			TryControllerBind( XInputController::EB_A );
			TryControllerBind( XInputController::EB_B );
			TryControllerBind( XInputController::EB_X );
			TryControllerBind( XInputController::EB_Y );
			TryControllerBind( XInputController::EB_Up );
			TryControllerBind( XInputController::EB_Down );
			TryControllerBind( XInputController::EB_Left );
			TryControllerBind( XInputController::EB_Right );
			TryControllerBind( XInputController::EB_LeftBumper );
			TryControllerBind( XInputController::EB_RightBumper );
			TryControllerBind( XInputController::EB_LeftThumb );
			TryControllerBind( XInputController::EB_RightThumb );
			TryControllerBind( XInputController::EB_LeftTrigger );
			TryControllerBind( XInputController::EB_RightTrigger );
		}

		if( m_Controller && m_CanBindControllerAnalog )
		{
			TryControllerBindAnalog( XInputController::EA_LeftThumbX );
			TryControllerBindAnalog( XInputController::EA_LeftThumbY );
			TryControllerBindAnalog( XInputController::EA_RightThumbX );
			TryControllerBindAnalog( XInputController::EA_RightThumbY );
			TryControllerBindAnalog( XInputController::EA_LeftTrigger );
			TryControllerBindAnalog( XInputController::EA_RightTrigger );
		}
	}

	if( !m_Binding )
	{
		UpdateBindingsFromConfig();
	}
}

// DLP 31 Mar 2020: This is implemented differently than what I did on TBC.
// Here, the topmost input context has absolute control; whatever it redirects
// to is *not* evaluated further down the stack. Contexts further down the
// stack only matter where the topmost context does not suppress or redirect.
// This is probably a more useful and elegant way to do it, but it confused
// me upon revisiting it, which is why I'm leaving this note.
HashedString InputSystem::GetContextualInput( const HashedString& Input ) const
{
	for( uint InputContextIndex = m_ActiveInputContexts.Size(); InputContextIndex > 0; --InputContextIndex )
	{
		InputContext* const pInputContext = m_ActiveInputContexts[ InputContextIndex - 1 ];
		if( pInputContext->HasRedirect( Input ) )
		{
			return pInputContext->GetRedirect( Input );
		}
		else if( pInputContext->Suppresses() )
		{
			return HashedString::NullString;
		}
	}

	// No redirects, and not suppressed, so pass through.
	return Input;
}

void InputSystem::PushContext( const HashedString& ContextName )
{
	DEVASSERT( m_InputContexts.Search( ContextName ).IsValid() );
	m_ActiveInputContexts.PushBack( m_InputContexts[ ContextName ] );

	// Whenever we change contexts, we need to retick to push the changes through immediately.
	Tick();
	NotifyObserversOnInputContextsChanged();
	PublishInputStrings();
}

void InputSystem::PopContext( const HashedString& ContextName )
{
	DEVASSERT( m_InputContexts.Search( ContextName ).IsValid() );

	const InputContext* const pPopContext = m_InputContexts[ ContextName ];
	FOR_EACH_ARRAY_REVERSE( ContextIter, m_ActiveInputContexts, InputContext* )
	{
		const InputContext* const pActiveContext = ContextIter.GetValue();
		if( pActiveContext == pPopContext )
		{
			m_ActiveInputContexts.Remove( ContextIter );

			// Whenever we change contexts, we need to retick to push the changes through immediately.
			Tick();
			NotifyObserversOnInputContextsChanged();
			PublishInputStrings();

			return;
		}
	}
}

void InputSystem::PopAllContexts()
{
	m_ActiveInputContexts.Clear();

	// Whenever we change contexts, we need to retick to push the changes through immediately.
	Tick();
	NotifyObserversOnInputContextsChanged();
	PublishInputStrings();
}

float InputSystem::GetTime() const
{
	DEBUGASSERT( m_Clock );

	// NOTE: Changed from GetGameCurrentTime so this behaves correctly regardless of clock multipliers
	return m_Clock->GetRealUnpausedCurrentTime();
}

bool InputSystem::IsSuppressed( const HashedString& Input ) const
{
	return GetContextualInput( Input ) == HashedString::NullString;
}

bool InputSystem::IsHigh( const HashedString& Signal ) const
{
	DEBUGASSERT( m_CurrentState );
	DEBUGASSERT( m_CurrentState->Search( Signal ).IsValid() );

	return ( *m_CurrentState )[ Signal ];
}

bool InputSystem::IsLow( const HashedString& Signal ) const
{
	DEBUGASSERT( m_CurrentState );
	DEBUGASSERT( m_CurrentState->Search( Signal ).IsValid() );

	return ( false == ( *m_CurrentState )[ Signal ] );
}

bool InputSystem::OnRise( const HashedString& Signal ) const
{
	DEBUGASSERT( m_CurrentState );
	DEBUGASSERT( m_CurrentState->Search( Signal ).IsValid() );
	DEBUGASSERT( m_LastState );
	DEBUGASSERT( m_LastState->Search( Signal ).IsValid() );

	return ( *m_CurrentState )[ Signal ] && ( false == ( *m_LastState )[ Signal ] );
}

// m_Held flag is only high for a single tick, so we don't need to compare with last state.
bool InputSystem::OnHold( const HashedString& Signal ) const
{
	DEBUGASSERT( m_HoldState.Search( Signal ).IsValid() );

	return m_HoldState[ Signal ].m_Held;
}

bool InputSystem::OnFall( const HashedString& Signal ) const
{
	DEBUGASSERT( m_CurrentState );
	DEBUGASSERT( m_CurrentState->Search( Signal ).IsValid() );
	DEBUGASSERT( m_LastState );
	DEBUGASSERT( m_LastState->Search( Signal ).IsValid() );

	return ( false == ( *m_CurrentState )[ Signal ] ) && ( *m_LastState )[ Signal ];
}

uint InputSystem::OnInput( const HashedString& Signal ) const
{
	uint InputFlags = INPUT_NONE;

	if( IsHigh( Signal ) ) { InputFlags |= INPUT_ISHIGH; }
	if( OnRise( Signal ) ) { InputFlags |= INPUT_ONRISE; }
	if( OnHold( Signal ) ) { InputFlags |= INPUT_ONHOLD; }
	if( OnFall( Signal ) ) { InputFlags |= INPUT_ONFALL; }

	return InputFlags;
}

float InputSystem::GetPosition( const HashedString& Axis ) const
{
	return ( *m_CurrentAnalogState )[ Axis ];
}

float InputSystem::GetVelocity( const HashedString& Axis ) const
{
	return ( *m_CurrentAnalogState )[ Axis ] - ( *m_LastAnalogState )[ Axis ];
}

void InputSystem::SetMouseScale( const HashedString& Axis, const float Scale )
{
	DEBUGASSERT( m_MouseAdjustments.Search( Axis ).IsValid() );
	m_MouseAdjustments[ Axis ].m_ScaleFactor = Scale;
}

bool InputSystem::GetMouseInvert( const HashedString& Axis )
{
	DEBUGASSERT( m_MouseAdjustments.Search( Axis ).IsValid() );
	return m_MouseAdjustments[ Axis ].m_InvertFactor;
}

void InputSystem::SetMouseInvert( const HashedString& Axis, const bool Invert )
{
	DEBUGASSERT( m_MouseAdjustments.Search( Axis ).IsValid() );
	m_MouseAdjustments[ Axis ].m_InvertFactor = Invert;
}

void InputSystem::SetControllerScale( const HashedString& Axis, const float Scale )
{
	DEBUGASSERT( m_ControllerAdjustments.Search( Axis ).IsValid() );
	m_ControllerAdjustments[ Axis ].m_ScaleFactor = Scale;
}

void InputSystem::SetControllerPower( const HashedString& Axis, const float Power )
{
	DEBUGASSERT( m_ControllerAdjustments.Search( Axis ).IsValid() );
	m_ControllerAdjustments[ Axis ].m_PowerFactor = Power;
}

bool InputSystem::GetControllerInvert( const HashedString& Axis )
{
	DEBUGASSERT( m_ControllerAdjustments.Search( Axis ).IsValid() );
	return m_ControllerAdjustments[ Axis ].m_InvertFactor;
}

void InputSystem::SetControllerInvert( const HashedString& Axis, const bool Invert )
{
	DEBUGASSERT( m_ControllerAdjustments.Search( Axis ).IsValid() );
	m_ControllerAdjustments[ Axis ].m_InvertFactor = Invert;
}

void InputSystem::InitializeSignalMaps()
{
	XTRACE_FUNCTION;

	m_KeyboardMap[ "" ]				= Keyboard::EB_None;
	m_KeyboardMap[ "A" ]			= Keyboard::EB_A;
	m_KeyboardMap[ "B" ]			= Keyboard::EB_B;
	m_KeyboardMap[ "C" ]			= Keyboard::EB_C;
	m_KeyboardMap[ "D" ]			= Keyboard::EB_D;
	m_KeyboardMap[ "E" ]			= Keyboard::EB_E;
	m_KeyboardMap[ "F" ]			= Keyboard::EB_F;
	m_KeyboardMap[ "G" ]			= Keyboard::EB_G;
	m_KeyboardMap[ "H" ]			= Keyboard::EB_H;
	m_KeyboardMap[ "I" ]			= Keyboard::EB_I;
	m_KeyboardMap[ "J" ]			= Keyboard::EB_J;
	m_KeyboardMap[ "K" ]			= Keyboard::EB_K;
	m_KeyboardMap[ "L" ]			= Keyboard::EB_L;
	m_KeyboardMap[ "M" ]			= Keyboard::EB_M;
	m_KeyboardMap[ "N" ]			= Keyboard::EB_N;
	m_KeyboardMap[ "O" ]			= Keyboard::EB_O;
	m_KeyboardMap[ "P" ]			= Keyboard::EB_P;
	m_KeyboardMap[ "Q" ]			= Keyboard::EB_Q;
	m_KeyboardMap[ "R" ]			= Keyboard::EB_R;
	m_KeyboardMap[ "S" ]			= Keyboard::EB_S;
	m_KeyboardMap[ "T" ]			= Keyboard::EB_T;
	m_KeyboardMap[ "U" ]			= Keyboard::EB_U;
	m_KeyboardMap[ "V" ]			= Keyboard::EB_V;
	m_KeyboardMap[ "W" ]			= Keyboard::EB_W;
	m_KeyboardMap[ "X" ]			= Keyboard::EB_X;
	m_KeyboardMap[ "Y" ]			= Keyboard::EB_Y;
	m_KeyboardMap[ "Z" ]			= Keyboard::EB_Z;
	m_KeyboardMap[ "Space" ]		= Keyboard::EB_Space;
	m_KeyboardMap[ "Left Shift" ]	= Keyboard::EB_LeftShift;
	m_KeyboardMap[ "Left Ctrl" ]	= Keyboard::EB_LeftControl;
	m_KeyboardMap[ "Left Alt" ]		= Keyboard::EB_LeftAlt;
	m_KeyboardMap[ "Right Shift" ]	= Keyboard::EB_RightShift;
	m_KeyboardMap[ "Right Ctrl" ]	= Keyboard::EB_RightControl;
	m_KeyboardMap[ "Right Alt" ]	= Keyboard::EB_RightAlt;
	m_KeyboardMap[ "Caps Lock" ]	= Keyboard::EB_CapsLock;
	m_KeyboardMap[ "Tab" ]			= Keyboard::EB_Tab;
	m_KeyboardMap[ "," ]			= Keyboard::EB_Comma;
	m_KeyboardMap[ "." ]			= Keyboard::EB_Period;
	m_KeyboardMap[ "/" ]			= Keyboard::EB_Slash;
	m_KeyboardMap[ ";" ]			= Keyboard::EB_Semicolon;
	m_KeyboardMap[ "'" ]			= Keyboard::EB_Apostrophe;
	m_KeyboardMap[ "[" ]			= Keyboard::EB_LeftBrace;
	m_KeyboardMap[ "]" ]			= Keyboard::EB_RightBrace;
	m_KeyboardMap[ "\\" ]			= Keyboard::EB_Backslash;
	m_KeyboardMap[ "`" ]			= Keyboard::EB_Grave;
	m_KeyboardMap[ "Up" ]			= Keyboard::EB_Up;
	m_KeyboardMap[ "Down" ]			= Keyboard::EB_Down;
	m_KeyboardMap[ "Left" ]			= Keyboard::EB_Left;
	m_KeyboardMap[ "Right" ]		= Keyboard::EB_Right;
	m_KeyboardMap[ "Num 0" ]		= Keyboard::EB_Num0;
	m_KeyboardMap[ "Num 1" ]		= Keyboard::EB_Num1;
	m_KeyboardMap[ "Num 2" ]		= Keyboard::EB_Num2;
	m_KeyboardMap[ "Num 3" ]		= Keyboard::EB_Num3;
	m_KeyboardMap[ "Num 4" ]		= Keyboard::EB_Num4;
	m_KeyboardMap[ "Num 5" ]		= Keyboard::EB_Num5;
	m_KeyboardMap[ "Num 6" ]		= Keyboard::EB_Num6;
	m_KeyboardMap[ "Num 7" ]		= Keyboard::EB_Num7;
	m_KeyboardMap[ "Num 8" ]		= Keyboard::EB_Num8;
	m_KeyboardMap[ "Num 9" ]		= Keyboard::EB_Num9;
	m_KeyboardMap[ "Num ." ]		= Keyboard::EB_NumDecimal;
	m_KeyboardMap[ "Num *" ]		= Keyboard::EB_NumMultiply;
	m_KeyboardMap[ "Num +" ]		= Keyboard::EB_NumAdd;
	m_KeyboardMap[ "Num -" ]		= Keyboard::EB_NumSubtract;
	m_KeyboardMap[ "Num /" ]		= Keyboard::EB_NumDivide;
	m_KeyboardMap[ "Enter" ]		= Keyboard::EB_Enter;
	m_KeyboardMap[ "Insert" ]		= Keyboard::EB_Insert;
	m_KeyboardMap[ "Home" ]			= Keyboard::EB_Home;
	m_KeyboardMap[ "Page Up" ]		= Keyboard::EB_PageUp;
	m_KeyboardMap[ "Delete" ]		= Keyboard::EB_Delete;
	m_KeyboardMap[ "End" ]			= Keyboard::EB_End;
	m_KeyboardMap[ "Page Down" ]	= Keyboard::EB_PageDown;
	m_KeyboardMap[ "1" ]			= Keyboard::EB_1;
	m_KeyboardMap[ "2" ]			= Keyboard::EB_2;
	m_KeyboardMap[ "3" ]			= Keyboard::EB_3;
	m_KeyboardMap[ "4" ]			= Keyboard::EB_4;
	m_KeyboardMap[ "5" ]			= Keyboard::EB_5;
	m_KeyboardMap[ "6" ]			= Keyboard::EB_6;
	m_KeyboardMap[ "7" ]			= Keyboard::EB_7;
	m_KeyboardMap[ "8" ]			= Keyboard::EB_8;
	m_KeyboardMap[ "9" ]			= Keyboard::EB_9;
	m_KeyboardMap[ "0" ]			= Keyboard::EB_0;
	m_KeyboardMap[ "F1" ]			= Keyboard::EB_F1;
	m_KeyboardMap[ "F2" ]			= Keyboard::EB_F2;
	m_KeyboardMap[ "F3" ]			= Keyboard::EB_F3;
	m_KeyboardMap[ "F4" ]			= Keyboard::EB_F4;
	m_KeyboardMap[ "F5" ]			= Keyboard::EB_F5;
	m_KeyboardMap[ "F6" ]			= Keyboard::EB_F6;
	m_KeyboardMap[ "F7" ]			= Keyboard::EB_F7;
	m_KeyboardMap[ "F8" ]			= Keyboard::EB_F8;
	m_KeyboardMap[ "F9" ]			= Keyboard::EB_F9;
	m_KeyboardMap[ "F10" ]			= Keyboard::EB_F10;
	m_KeyboardMap[ "F11" ]			= Keyboard::EB_F11;
	m_KeyboardMap[ "F12" ]			= Keyboard::EB_F12;
	m_KeyboardMap[ "-" ]			= Keyboard::EB_Minus;
	m_KeyboardMap[ "=" ]			= Keyboard::EB_Plus;
	m_KeyboardMap[ "Backspace" ]	= Keyboard::EB_Backspace;
	m_KeyboardMap[ "Esc" ]			= Keyboard::EB_Escape;		// Not bindable, but may be displayed in UI

	m_MouseMap[ "" ]			= Mouse::EB_None;
	m_MouseMap[ "LMB" ]			= Mouse::EB_Left;
	m_MouseMap[ "MMB" ]			= Mouse::EB_Middle;
	m_MouseMap[ "RMB" ]			= Mouse::EB_Right;
	m_MouseMap[ "Mouse4" ]		= Mouse::EB_X1;
	m_MouseMap[ "Mouse5" ]		= Mouse::EB_X2;
	m_MouseMap[ "WheelUp" ]		= Mouse::EB_WheelUp;
	m_MouseMap[ "WheelDown" ]	= Mouse::EB_WheelDown;

	m_MouseAnalogMap[ "" ]			= Mouse::EA_None;
	m_MouseAnalogMap[ "MouseX" ]	= Mouse::EA_X;
	m_MouseAnalogMap[ "MouseY" ]	= Mouse::EA_Y;

	m_ControllerMap[ "" ]		= XInputController::EB_None;
	m_ControllerMap[ "xA" ]		= XInputController::EB_A;
	m_ControllerMap[ "xB" ]		= XInputController::EB_B;
	m_ControllerMap[ "xX" ]		= XInputController::EB_X;
	m_ControllerMap[ "xY" ]		= XInputController::EB_Y;
	m_ControllerMap[ "LB" ]		= XInputController::EB_LeftBumper;
	m_ControllerMap[ "RB" ]		= XInputController::EB_RightBumper;
	m_ControllerMap[ "xUp" ]	= XInputController::EB_Up;
	m_ControllerMap[ "xDown" ]	= XInputController::EB_Down;
	m_ControllerMap[ "xLeft" ]	= XInputController::EB_Left;
	m_ControllerMap[ "xRight" ]	= XInputController::EB_Right;
	m_ControllerMap[ "LS" ]		= XInputController::EB_LeftThumb;
	m_ControllerMap[ "RS" ]		= XInputController::EB_RightThumb;
	m_ControllerMap[ "LT" ]		= XInputController::EB_LeftTrigger;
	m_ControllerMap[ "RT" ]		= XInputController::EB_RightTrigger;
	// Intentionally not allowing binding of Start or Back buttons.
	// Intentionally not allowing binding of bool analog stick signals.

	m_ControllerAnalogMap[ "" ]			= XInputController::EA_None;
	m_ControllerAnalogMap[ "xLeftX" ]	= XInputController::EA_LeftThumbX;
	m_ControllerAnalogMap[ "xLeftY" ]	= XInputController::EA_LeftThumbY;
	m_ControllerAnalogMap[ "xRightX" ]	= XInputController::EA_RightThumbX;
	m_ControllerAnalogMap[ "xRightY" ]	= XInputController::EA_RightThumbY;
	m_ControllerAnalogMap[ "xLT" ]		= XInputController::EA_LeftTrigger;
	m_ControllerAnalogMap[ "xRT" ]		= XInputController::EA_RightTrigger;

	m_ReverseKeyboardMap[ Keyboard::EB_None ]			= "";
	m_ReverseKeyboardMap[ Keyboard::EB_A ]				= "A";
	m_ReverseKeyboardMap[ Keyboard::EB_B ]				= "B";
	m_ReverseKeyboardMap[ Keyboard::EB_C ]				= "C";
	m_ReverseKeyboardMap[ Keyboard::EB_D ]				= "D";
	m_ReverseKeyboardMap[ Keyboard::EB_E ]				= "E";
	m_ReverseKeyboardMap[ Keyboard::EB_F ]				= "F";
	m_ReverseKeyboardMap[ Keyboard::EB_G ]				= "G";
	m_ReverseKeyboardMap[ Keyboard::EB_H ]				= "H";
	m_ReverseKeyboardMap[ Keyboard::EB_I ]				= "I";
	m_ReverseKeyboardMap[ Keyboard::EB_J ]				= "J";
	m_ReverseKeyboardMap[ Keyboard::EB_K ]				= "K";
	m_ReverseKeyboardMap[ Keyboard::EB_L ]				= "L";
	m_ReverseKeyboardMap[ Keyboard::EB_M ]				= "M";
	m_ReverseKeyboardMap[ Keyboard::EB_N ]				= "N";
	m_ReverseKeyboardMap[ Keyboard::EB_O ]				= "O";
	m_ReverseKeyboardMap[ Keyboard::EB_P ]				= "P";
	m_ReverseKeyboardMap[ Keyboard::EB_Q ]				= "Q";
	m_ReverseKeyboardMap[ Keyboard::EB_R ]				= "R";
	m_ReverseKeyboardMap[ Keyboard::EB_S ]				= "S";
	m_ReverseKeyboardMap[ Keyboard::EB_T ]				= "T";
	m_ReverseKeyboardMap[ Keyboard::EB_U ]				= "U";
	m_ReverseKeyboardMap[ Keyboard::EB_V ]				= "V";
	m_ReverseKeyboardMap[ Keyboard::EB_W ]				= "W";
	m_ReverseKeyboardMap[ Keyboard::EB_X ]				= "X";
	m_ReverseKeyboardMap[ Keyboard::EB_Y ]				= "Y";
	m_ReverseKeyboardMap[ Keyboard::EB_Z ]				= "Z";
	m_ReverseKeyboardMap[ Keyboard::EB_Space ]			= "Space";
	m_ReverseKeyboardMap[ Keyboard::EB_LeftShift ]		= "Left Shift";
	m_ReverseKeyboardMap[ Keyboard::EB_LeftControl ]	= "Left Ctrl";
	m_ReverseKeyboardMap[ Keyboard::EB_LeftAlt ]		= "Left Alt";
	m_ReverseKeyboardMap[ Keyboard::EB_RightShift ]		= "Right Shift";
	m_ReverseKeyboardMap[ Keyboard::EB_RightControl ]	= "Right Ctrl";
	m_ReverseKeyboardMap[ Keyboard::EB_RightAlt ]		= "Right Alt";
	m_ReverseKeyboardMap[ Keyboard::EB_CapsLock ]		= "Caps Lock";
	m_ReverseKeyboardMap[ Keyboard::EB_Tab ]			= "Tab";
	m_ReverseKeyboardMap[ Keyboard::EB_Comma ]			= ",";
	m_ReverseKeyboardMap[ Keyboard::EB_Period ]			= ".";
	m_ReverseKeyboardMap[ Keyboard::EB_Slash ]			= "/";
	m_ReverseKeyboardMap[ Keyboard::EB_Semicolon ]		= ";";
	m_ReverseKeyboardMap[ Keyboard::EB_Apostrophe ]		= "'";
	m_ReverseKeyboardMap[ Keyboard::EB_LeftBrace ]		= "[";
	m_ReverseKeyboardMap[ Keyboard::EB_RightBrace ]		= "]";
	m_ReverseKeyboardMap[ Keyboard::EB_Backslash ]		= "\\";
	m_ReverseKeyboardMap[ Keyboard::EB_Grave ]			= "`";
	m_ReverseKeyboardMap[ Keyboard::EB_Up ]				= "Up";
	m_ReverseKeyboardMap[ Keyboard::EB_Down ]			= "Down";
	m_ReverseKeyboardMap[ Keyboard::EB_Left ]			= "Left";
	m_ReverseKeyboardMap[ Keyboard::EB_Right ]			= "Right";
	m_ReverseKeyboardMap[ Keyboard::EB_Num0 ]			= "Num 0";
	m_ReverseKeyboardMap[ Keyboard::EB_Num1 ]			= "Num 1";
	m_ReverseKeyboardMap[ Keyboard::EB_Num2 ]			= "Num 2";
	m_ReverseKeyboardMap[ Keyboard::EB_Num3 ]			= "Num 3";
	m_ReverseKeyboardMap[ Keyboard::EB_Num4 ]			= "Num 4";
	m_ReverseKeyboardMap[ Keyboard::EB_Num5 ]			= "Num 5";
	m_ReverseKeyboardMap[ Keyboard::EB_Num6 ]			= "Num 6";
	m_ReverseKeyboardMap[ Keyboard::EB_Num7 ]			= "Num 7";
	m_ReverseKeyboardMap[ Keyboard::EB_Num8 ]			= "Num 8";
	m_ReverseKeyboardMap[ Keyboard::EB_Num9 ]			= "Num 9";
	m_ReverseKeyboardMap[ Keyboard::EB_NumDecimal ]		= "Num .";
	m_ReverseKeyboardMap[ Keyboard::EB_NumMultiply ]	= "Num *";
	m_ReverseKeyboardMap[ Keyboard::EB_NumAdd ]			= "Num +";
	m_ReverseKeyboardMap[ Keyboard::EB_NumSubtract ]	= "Num -";
	m_ReverseKeyboardMap[ Keyboard::EB_NumDivide ]		= "Num /";
	m_ReverseKeyboardMap[ Keyboard::EB_Enter ]			= "Enter";
	m_ReverseKeyboardMap[ Keyboard::EB_Insert ]			= "Insert";
	m_ReverseKeyboardMap[ Keyboard::EB_Home ]			= "Home";
	m_ReverseKeyboardMap[ Keyboard::EB_PageUp ]			= "Page Up";
	m_ReverseKeyboardMap[ Keyboard::EB_Delete ]			= "Delete";
	m_ReverseKeyboardMap[ Keyboard::EB_End ]			= "End";
	m_ReverseKeyboardMap[ Keyboard::EB_PageDown ]		= "Page Down";
	m_ReverseKeyboardMap[ Keyboard::EB_1 ]				= "1";
	m_ReverseKeyboardMap[ Keyboard::EB_2 ]				= "2";
	m_ReverseKeyboardMap[ Keyboard::EB_3 ]				= "3";
	m_ReverseKeyboardMap[ Keyboard::EB_4 ]				= "4";
	m_ReverseKeyboardMap[ Keyboard::EB_5 ]				= "5";
	m_ReverseKeyboardMap[ Keyboard::EB_6 ]				= "6";
	m_ReverseKeyboardMap[ Keyboard::EB_7 ]				= "7";
	m_ReverseKeyboardMap[ Keyboard::EB_8 ]				= "8";
	m_ReverseKeyboardMap[ Keyboard::EB_9 ]				= "9";
	m_ReverseKeyboardMap[ Keyboard::EB_0 ]				= "0";
	m_ReverseKeyboardMap[ Keyboard::EB_F1 ]				= "F1";
	m_ReverseKeyboardMap[ Keyboard::EB_F2 ]				= "F2";
	m_ReverseKeyboardMap[ Keyboard::EB_F3 ]				= "F3";
	m_ReverseKeyboardMap[ Keyboard::EB_F4 ]				= "F4";
	m_ReverseKeyboardMap[ Keyboard::EB_F5 ]				= "F5";
	m_ReverseKeyboardMap[ Keyboard::EB_F6 ]				= "F6";
	m_ReverseKeyboardMap[ Keyboard::EB_F7 ]				= "F7";
	m_ReverseKeyboardMap[ Keyboard::EB_F8 ]				= "F8";
	m_ReverseKeyboardMap[ Keyboard::EB_F9 ]				= "F9";
	m_ReverseKeyboardMap[ Keyboard::EB_F10 ]			= "F10";
	m_ReverseKeyboardMap[ Keyboard::EB_F11 ]			= "F11";
	m_ReverseKeyboardMap[ Keyboard::EB_F12 ]			= "F12";
	m_ReverseKeyboardMap[ Keyboard::EB_Minus ]			= "-";
	m_ReverseKeyboardMap[ Keyboard::EB_Plus ]			= "=";
	m_ReverseKeyboardMap[ Keyboard::EB_Backspace ]		= "Backspace";
	m_ReverseKeyboardMap[ Keyboard::EB_Escape ]			= "Esc";		// Not bindable, but may be displayed in UI

	m_ReverseMouseMap[ Mouse::EB_None ]			= "";
	m_ReverseMouseMap[ Mouse::EB_Left ]			= "LMB";
	m_ReverseMouseMap[ Mouse::EB_Middle ]		= "MMB";
	m_ReverseMouseMap[ Mouse::EB_Right ]		= "RMB";
	m_ReverseMouseMap[ Mouse::EB_X1 ]			= "Mouse4";
	m_ReverseMouseMap[ Mouse::EB_X2 ]			= "Mouse5";
	m_ReverseMouseMap[ Mouse::EB_WheelUp ]		= "WheelUp";
	m_ReverseMouseMap[ Mouse::EB_WheelDown ]	= "WheelDown";

	m_ReverseMouseAnalogMap[ Mouse::EA_None ]	= "";
	m_ReverseMouseAnalogMap[ Mouse::EA_X ]		= "MouseX";
	m_ReverseMouseAnalogMap[ Mouse::EA_Y ]		= "MouseY";

	m_ReverseControllerMap[ XInputController::EB_None ]			= "";
	m_ReverseControllerMap[ XInputController::EB_A ]			= "xA";
	m_ReverseControllerMap[ XInputController::EB_B ]			= "xB";
	m_ReverseControllerMap[ XInputController::EB_X ]			= "xX";
	m_ReverseControllerMap[ XInputController::EB_Y ]			= "xY";
	m_ReverseControllerMap[ XInputController::EB_LeftBumper ]	= "LB";
	m_ReverseControllerMap[ XInputController::EB_RightBumper ]	= "RB";
	m_ReverseControllerMap[ XInputController::EB_Up ]			= "xUp";
	m_ReverseControllerMap[ XInputController::EB_Down ]			= "xDown";
	m_ReverseControllerMap[ XInputController::EB_Left ]			= "xLeft";
	m_ReverseControllerMap[ XInputController::EB_Right ]		= "xRight";
	m_ReverseControllerMap[ XInputController::EB_LeftThumb ]	= "LS";
	m_ReverseControllerMap[ XInputController::EB_RightThumb ]	= "RS";
	m_ReverseControllerMap[ XInputController::EB_LeftTrigger ]	= "LT";
	m_ReverseControllerMap[ XInputController::EB_RightTrigger ]	= "RT";

	m_ReverseControllerAnalogMap[ XInputController::EA_None ]			= "";
	m_ReverseControllerAnalogMap[ XInputController::EA_LeftThumbX ]		= "xLeftX";
	m_ReverseControllerAnalogMap[ XInputController::EA_LeftThumbY ]		= "xLeftY";
	m_ReverseControllerAnalogMap[ XInputController::EA_RightThumbX ]	= "xRightX";
	m_ReverseControllerAnalogMap[ XInputController::EA_RightThumbY ]	= "xRightY";
	m_ReverseControllerAnalogMap[ XInputController::EA_LeftTrigger ]	= "xLT";
	m_ReverseControllerAnalogMap[ XInputController::EA_RightTrigger ]	= "xRT";
}

// NOTE: This used to be inline, but not every compiler would link it properly.
/*inline*/ uint InputSystem::GetBoundKeyboardSignal( const HashedString& Input ) const
{
	DEVASSERT( m_KeyboardBindings.Search( Input ).IsValid() );
	return m_KeyboardBindings[ Input ];
}

// NOTE: This used to be inline, but not every compiler would link it properly.
/*inline*/ uint InputSystem::GetBoundMouseSignal( const HashedString& Input ) const
{
	DEVASSERT( m_MouseBindings.Search( Input ).IsValid() );
	return m_MouseBindings[ Input ];
}

// NOTE: This used to be inline, but not every compiler would link it properly.
/*inline*/ uint InputSystem::GetBoundControllerSignal( const HashedString& Input ) const
{
	DEVASSERT( m_ControllerBindings.Search( Input ).IsValid() );
	return m_ControllerBindings[ Input ];
}

inline uint InputSystem::GetBoundMouseAnalogSignal( const HashedString& Input ) const
{
	DEVASSERT( m_MouseAnalogBindings.Search( Input ).IsValid() );
	return m_MouseAnalogBindings[ Input ];
}

inline uint InputSystem::GetBoundControllerAnalogSignal( const HashedString& Input ) const
{
	DEVASSERT( m_ControllerAnalogBindings.Search( Input ).IsValid() );
	return m_ControllerAnalogBindings[ Input ];
}

SimpleString InputSystem::GetInputBoundToKeyboard( const uint Signal ) const
{
	const uint NumInputs = m_Inputs.Size();
	for( uint InputIndex = 0; InputIndex < NumInputs; ++InputIndex )
	{
		const SInput& Input = m_Inputs[ InputIndex ];
		if( Signal == m_KeyboardBindings[ Input.m_Hash ] )
		{
			return Input.m_String;
		}
	}
	return "";
}

SimpleString InputSystem::GetInputBoundToMouse( const uint Signal ) const
{
	const uint NumInputs = m_Inputs.Size();
	for( uint InputIndex = 0; InputIndex < NumInputs; ++InputIndex )
	{
		const SInput& Input = m_Inputs[ InputIndex ];
		if( Signal == m_MouseBindings[ Input.m_Hash ] )
		{
			return Input.m_String;
		}
	}
	return "";
}

SimpleString InputSystem::GetInputBoundToController( const uint Signal ) const
{
	const uint NumInputs = m_Inputs.Size();
	for( uint InputIndex = 0; InputIndex < NumInputs; ++InputIndex )
	{
		const SInput& Input = m_Inputs[ InputIndex ];
		if( Signal == m_ControllerBindings[ Input.m_Hash ] )
		{
			return Input.m_String;
		}
	}
	return "";
}

SimpleString InputSystem::GetAnalogInputBoundToMouse( const uint Signal ) const
{
	const uint NumAnalogInputs = m_AnalogInputs.Size();
	for( uint InputIndex = 0; InputIndex < NumAnalogInputs; ++InputIndex )
	{
		const SAnalogInput& Input = m_AnalogInputs[ InputIndex ];
		if( Signal == m_MouseAnalogBindings[ Input.m_Hash ] )
		{
			return Input.m_String;
		}
	}
	return "";
}

SimpleString InputSystem::GetAnalogInputBoundToController( const uint Signal ) const
{
	const uint NumAnalogInputs = m_AnalogInputs.Size();
	for( uint InputIndex = 0; InputIndex < NumAnalogInputs; ++InputIndex )
	{
		const SAnalogInput& Input = m_AnalogInputs[ InputIndex ];
		if( Signal == m_ControllerAnalogBindings[ Input.m_Hash ] )
		{
			return Input.m_String;
		}
	}
	return "";
}

inline uint InputSystem::GetKeyboardSignal( const HashedString& Name ) const
{
	DEVASSERT( m_KeyboardMap.Search( Name ).IsValid() );
	return m_KeyboardMap[ Name ];
}

inline uint InputSystem::GetMouseSignal( const HashedString& Name ) const
{
	DEVASSERT( m_MouseMap.Search( Name ).IsValid() );
	return m_MouseMap[ Name ];
}

inline uint InputSystem::GetControllerSignal( const HashedString& Name ) const
{
	DEVASSERT( m_ControllerMap.Search( Name ).IsValid() );
	return m_ControllerMap[ Name ];
}

inline const SimpleString& InputSystem::GetKeyboardSignalName( const uint Signal ) const
{
	DEVASSERT( m_ReverseKeyboardMap.Search( Signal ).IsValid() );
	return m_ReverseKeyboardMap[ Signal ];
}

inline const SimpleString& InputSystem::GetMouseSignalName( const uint Signal ) const
{
	DEVASSERT( m_ReverseMouseMap.Search( Signal ).IsValid() );
	return m_ReverseMouseMap[ Signal ];
}

inline const SimpleString& InputSystem::GetControllerSignalName( const uint Signal ) const
{
	DEVASSERT( m_ReverseControllerMap.Search( Signal ).IsValid() );
	return m_ReverseControllerMap[ Signal ];
}

const SimpleString& InputSystem::GetPreferredControllerSignalName( const SimpleString& Name ) const
{
	const SControllerType& PreferredControllerType = m_ControllerTypes[ m_ControllerType ];

	TControllerType::Iterator PreferredSignalNameIter = PreferredControllerType.m_Map.Search( Name );
	if( PreferredSignalNameIter.IsValid() )
	{
		return PreferredSignalNameIter.GetValue();
	}
	else
	{
		// Fall back to the default if no preferred signal name is specified for this signal
		return Name;
	}
}

inline uint InputSystem::GetMouseAnalogSignal( const HashedString& Name ) const
{
	DEVASSERT( m_MouseAnalogMap.Search( Name ).IsValid() );
	return m_MouseAnalogMap[ Name ];
}

inline uint InputSystem::GetControllerAnalogSignal( const HashedString& Name ) const
{
	DEVASSERT( m_ControllerAnalogMap.Search( Name ).IsValid() );
	return m_ControllerAnalogMap[ Name ];
}

inline const SimpleString& InputSystem::GetMouseAnalogSignalName( const uint Signal ) const
{
	DEVASSERT( m_ReverseMouseAnalogMap.Search( Signal ).IsValid() );
	return m_ReverseMouseAnalogMap[ Signal ];
}

inline const SimpleString& InputSystem::GetControllerAnalogSignalName( const uint Signal ) const
{
	DEVASSERT( m_ReverseControllerAnalogMap.Search( Signal ).IsValid() );
	return m_ReverseControllerAnalogMap[ Signal ];
}

// TODO: Also write adjustments (scale/invert) for analog inputs?
// Might not be needed if there's just a few things that calling code can manage.
void InputSystem::WriteConfigBinds( const IDataStream& Stream )
{
	// NOTE: Also write preferred controller type
	ConfigManager::Write( Stream, "ControllerType", m_DefinitionName );

	const uint NumInputs		= m_Inputs.Size();
	const uint NumAnalogInputs	= m_AnalogInputs.Size();

	for( uint InputIndex = 0; InputIndex < NumInputs; ++InputIndex )
	{
		const SInput& Input = m_Inputs[ InputIndex ];
		if( CanBindToKeyboard( Input.m_Hash ) )
		{
			ConfigManager::Write( Stream, Input.m_String, m_KeyboardContext );
		}
	}

	for( uint InputIndex = 0; InputIndex < NumInputs; ++InputIndex )
	{
		const SInput& Input = m_Inputs[ InputIndex ];
		if( CanBindToMouse( Input.m_Hash ) )
		{
			ConfigManager::Write( Stream, Input.m_String, m_MouseContext );
		}
	}

	for( uint InputIndex = 0; InputIndex < NumAnalogInputs; ++InputIndex )
	{
		const SAnalogInput& Input = m_AnalogInputs[ InputIndex ];
		if( CanBindToMouseAnalog( Input.m_Hash ) )
		{
			ConfigManager::Write( Stream, Input.m_String, m_MouseContext );
		}
	}

	for( uint InputIndex = 0; InputIndex < NumInputs; ++InputIndex )
	{
		const SInput& Input = m_Inputs[ InputIndex ];
		if( CanBindToController( Input.m_Hash ) )
		{
			ConfigManager::Write( Stream, Input.m_String, m_ControllerContext );
		}
	}

	for( uint InputIndex = 0; InputIndex < NumAnalogInputs; ++InputIndex )
	{
		const SAnalogInput& Input = m_AnalogInputs[ InputIndex ];
		if( CanBindToControllerAnalog( Input.m_Hash ) )
		{
			ConfigManager::Write( Stream, Input.m_String, m_ControllerContext );
		}
	}
}

void InputSystem::BindInput( const SimpleString& InputName )
{
	m_Binding = true;
	m_BindingInputIsAnalog = false;
	m_BindingInput = InputName;

	for( uint AnalogInputIndex = 0; AnalogInputIndex < m_AnalogInputs.Size(); ++AnalogInputIndex )
	{
		SAnalogInput& Input = m_AnalogInputs[ AnalogInputIndex ];
		if( Input.m_String == InputName )
		{
			m_BindingInputIsAnalog = true;
			break;
		}
	}

	// HACK: Tick all devices so we ignore currently rising inputs.
	if( m_Keyboard )	{ m_Keyboard->Tick( 0.0f ); }
	if( m_Mouse )		{ m_Mouse->Tick( 0.0f ); }
	if( m_Controller )	{ m_Controller->Tick( 0.0f ); }

	const HashedString InputHash = InputName;
	m_CanBindKeyboard			= !m_BindingInputIsAnalog && CanBindToKeyboard( InputHash );
	m_CanBindMouse				= !m_BindingInputIsAnalog && CanBindToMouse( InputHash );
	m_CanBindController			= !m_BindingInputIsAnalog && CanBindToController( InputHash );
	m_CanBindMouseAnalog		= m_BindingInputIsAnalog && CanBindToMouseAnalog( InputHash );
	m_CanBindControllerAnalog	= m_BindingInputIsAnalog && CanBindToControllerAnalog( InputHash );
}

bool InputSystem::CanBindToKeyboard( const HashedString& Input ) const
{
	return m_UnbindableKeyboardInputs.Search( Input ).IsNull();
}

bool InputSystem::CanBindToMouse( const HashedString& Input ) const
{
	return m_UnbindableMouseInputs.Search( Input ).IsNull();
}

bool InputSystem::CanBindToController( const HashedString& Input ) const
{
	return m_UnbindableControllerInputs.Search( Input ).IsNull();
}

bool InputSystem::CanBindToMouseAnalog( const HashedString& Input ) const
{
	return m_UnbindableMouseAnalogInputs.Search( Input ).IsNull();
}

bool InputSystem::CanBindToControllerAnalog( const HashedString& Input ) const
{
	return m_UnbindableControllerAnalogInputs.Search( Input ).IsNull();
}

bool InputSystem::TryKeyboardBind( const uint Signal )
{
	MAKEHASH( m_KeyboardContext );
	MAKEHASH( m_BindingInput );

	if( m_Binding && m_Keyboard->OnRise( Signal ) )
	{
		// Swap inputs if binding a key that's already in use
		const SimpleString CurrentInputForKey = GetInputBoundToKeyboard( Signal );
		if( CurrentInputForKey != "" )
		{
			const SimpleString CurrentKeyForBindingInput = ConfigManager::GetString( sm_BindingInput, "", sm_KeyboardContext );
			ConfigManager::SetString( CurrentInputForKey, CurrentKeyForBindingInput.CStr(), sm_KeyboardContext );
		}

		ConfigManager::SetString( sm_BindingInput, GetKeyboardSignalName( Signal ).CStr(), sm_KeyboardContext );
		m_Binding = false;

		return true;
	}
	else
	{
		return false;
	}
}

bool InputSystem::TryMouseBind( const uint KeyboardSignal, const uint MouseSignal )
{
	MAKEHASH( m_MouseContext );
	MAKEHASH( m_BindingInput );

	if( m_Binding &&
		( ( KeyboardSignal > Keyboard::EB_None && m_Keyboard->OnRise( KeyboardSignal ) ) ||
		m_Mouse->OnRise( MouseSignal ) ) )
	{
		// Swap inputs if binding a key that's already in use
		const SimpleString CurrentInputForKey = GetInputBoundToMouse( MouseSignal );
		if( CurrentInputForKey != "" )
		{
			const SimpleString CurrentKeyForBindingInput = ConfigManager::GetString( sm_BindingInput, "", sm_MouseContext );
			ConfigManager::SetString( CurrentInputForKey, CurrentKeyForBindingInput.CStr(), sm_MouseContext );
		}

		ConfigManager::SetString( sm_BindingInput, GetMouseSignalName( MouseSignal ).CStr(), sm_MouseContext );
		m_Binding = false;

		return true;
	}
	else
	{
		return false;
	}
}

bool InputSystem::TryControllerBind( const uint Signal )
{
	MAKEHASH( m_ControllerContext );
	MAKEHASH( m_BindingInput );

	if( m_Binding && m_Controller->OnRise( Signal ) )
	{
		// Swap inputs if binding a key that's already in use
		const SimpleString CurrentInputForKey = GetInputBoundToController( Signal );
		if( CurrentInputForKey != "" )
		{
			const SimpleString CurrentKeyForBindingInput = ConfigManager::GetString( sm_BindingInput, "", sm_ControllerContext );
			ConfigManager::SetString( CurrentInputForKey, CurrentKeyForBindingInput.CStr(), sm_ControllerContext );
		}

		ConfigManager::SetString( sm_BindingInput, GetControllerSignalName( Signal ).CStr(), sm_ControllerContext );
		m_Binding = false;

		return true;
	}
	else
	{
		return false;
	}
}

bool InputSystem::TryMouseBindAnalog( const uint Signal )
{
	MAKEHASH( m_MouseContext );
	MAKEHASH( m_BindingInput );

	// TODO: Use the sign to invert the input?
	static const float skMouseBindAnalogThreshold = 5.0f;
	if( m_Binding && Abs( m_Mouse->GetPosition( Signal ) ) > skMouseBindAnalogThreshold )
	{
		// Swap inputs if binding a key that's already in use
		const SimpleString CurrentInputForKey = GetAnalogInputBoundToMouse( Signal );
		if( CurrentInputForKey != "" )
		{
			const SimpleString CurrentKeyForBindingInput = ConfigManager::GetString( sm_BindingInput, "", sm_MouseContext );
			ConfigManager::SetString( CurrentInputForKey, CurrentKeyForBindingInput.CStr(), sm_MouseContext );
		}

		ConfigManager::SetString( sm_BindingInput, GetMouseAnalogSignalName( Signal ).CStr(), sm_MouseContext );
		m_Binding = false;

		return true;
	}
	else
	{
		return false;
	}
}

bool InputSystem::TryControllerBindAnalog( const uint Signal )
{
	MAKEHASH( m_ControllerContext );
	MAKEHASH( m_BindingInput );

	// TODO: Use the sign to invert the input?
	static const float skControllerBindAnalogThreshold = 0.1f;
	if( m_Binding && Abs( m_Controller->GetPosition( Signal ) ) > skControllerBindAnalogThreshold )
	{
		// Swap inputs if binding a key that's already in use
		const SimpleString CurrentInputForKey = GetAnalogInputBoundToController( Signal );
		if( CurrentInputForKey != "" )
		{
			const SimpleString CurrentKeyForBindingInput = ConfigManager::GetString( sm_BindingInput, "", sm_ControllerContext );
			ConfigManager::SetString( CurrentInputForKey, CurrentKeyForBindingInput.CStr(), sm_ControllerContext );
		}

		ConfigManager::SetString( sm_BindingInput, GetControllerAnalogSignalName( Signal ).CStr(), sm_ControllerContext );
		m_Binding = false;

		return true;
	}
	else
	{
		return false;
	}
}

void InputSystem::AddInputSystemObserver( IInputSystemObserver* const pObserver )
{
	DEVASSERT( pObserver );
	m_InputSystemObservers.PushBack( pObserver );
}

void InputSystem::RemoveInputSystemObserver( IInputSystemObserver* const pObserver )
{
	DEVASSERT( pObserver );
	m_InputSystemObservers.RemoveItem( pObserver );
}

void InputSystem::NotifyObserversOnInputContextsChanged() const
{
	FOR_EACH_ARRAY( ObserverIter, m_InputSystemObservers, IInputSystemObserver* )
	{
		IInputSystemObserver* const pObserver = ObserverIter.GetValue();
		pObserver->OnInputContextsChanged();
	}
}

void InputSystem::SetControllerType( const HashedString& ControllerType )
{
	FOR_EACH_ARRAY( ControllerTypeIter, m_ControllerTypes, SControllerType )
	{
		if( ControllerType == ControllerTypeIter.GetValue().m_Name )
		{
			m_ControllerType = ControllerTypeIter.GetIndex();
			break;
		}
	}

	// Republish input strings since our controller type has changed
	PublishInputStrings();
}

void InputSystem::CycleControllerType()
{
	m_ControllerType = ( m_ControllerType + 1 ) % m_ControllerTypes.Size();

	// Republish input strings since our controller type has changed
	PublishInputStrings();

	// Publish config var for prefs.cfg
	STATICHASH( ControllerType );
	MAKEHASH( m_DefinitionName );
	ConfigManager::SetString( sControllerType, m_ControllerTypes[ m_ControllerType ].m_Name.CStr(), sm_DefinitionName );
}

void InputSystem::UpdateIsUsingControllerExclusively()
{
	const bool IsUsingController			= m_Controller->ReceivedInputThisTick();
	const bool IsUsingMouse					= m_Mouse->ReceivedInputThisTick();
	const bool IsUsingKeyboard				= m_Keyboard->ReceivedInputThisTick();
	const bool IsUsingControllerExclusively	= IsUsingController && !IsUsingMouse && !IsUsingKeyboard;
	const bool IsUsingAnyInput				= IsUsingController || IsUsingMouse || IsUsingKeyboard;

	if( IsUsingAnyInput && IsUsingControllerExclusively != m_IsUsingControllerExclusively )
	{
		m_IsUsingControllerExclusively = IsUsingControllerExclusively;
		PublishInputStrings();
	}

	// Currently only checking gamepad for lost status; there's no standard expectation
	// for keyboard/mouse failures and if your batteries in a wireless mouse dies, you
	// can still pause with your keyboard. But it's something I theoretically could do.
	const bool IsControllerConnected		= m_Controller->WasConnectedThisTick();
	if( m_IsUsingControllerExclusively && !IsControllerConnected )
	{
		m_ShouldPauseDueToLostDevice = true;
	}
	else
	{
		m_ShouldPauseDueToLostDevice = false;
	}
}

void InputSystem::PublishInputStrings()
{
	FOR_EACH_ARRAY( InputIter, m_ExposedInputs, SimpleString )
	{
		const SimpleString& Input = InputIter.GetValue();
		PublishControllerGlyphString( Input );
	}

	FOR_EACH_ARRAY( InputIter, m_ExposedInputs, SimpleString )
	{
		const SimpleString& Input = InputIter.GetValue();
		PublishInputString( Input );
	}

	FOR_EACH_ARRAY( DisplayInputIter, m_DisplayInputs, SDisplayInput )
	{
		const SDisplayInput& DisplayInput = DisplayInputIter.GetValue();
		PublishDisplayInputString( DisplayInput );
	}
}

void InputSystem::PublishControllerGlyphString( const SimpleString& Input )
{
	const HashedString	OriginalInputHash	= Input;
	SimpleString		ControllerString	= "";

	const uint ControllerSignal	= GetBoundControllerSignal( OriginalInputHash );
	if( ControllerSignal != XInputController::EB_None )
	{
		const SimpleString&	DefaultName	= GetControllerSignalName( ControllerSignal );		// e.g. "xA"
		ControllerString				= GetPreferredControllerSignalName( DefaultName );	// e.g. "ps_X"
	}

	MAKEHASH( m_ControllerTypeContext );
	ConfigManager::SetString( OriginalInputHash, ControllerString.CStr(), sm_ControllerTypeContext );
}

void InputSystem::PublishInputString( const SimpleString& Input )
{
	const HashedString	OriginalInputHash	= Input;
	const HashedString	ContextualInputHash	= GetContextualInput( OriginalInputHash );
	const HashedString	UsingInputHash		= ( ContextualInputHash == HashedString::NullString ) ? OriginalInputHash : ContextualInputHash;	// HACKHACK: Show original input if suppressed
	const SimpleString	UsingInput			= ReverseHash::ReversedHash( UsingInputHash );
	SimpleString		InputString			= "";

	if( UsingInputHash != HashedString::NullString )
	{
		if( m_IsUsingControllerExclusively )
		{
			const uint ControllerSignal	= GetBoundControllerSignal( OriginalInputHash );
			if( ControllerSignal != XInputController::EB_None )
			{
				InputString				+= SimpleString::PrintF( "#{l:%s:%s}", m_ControllerTypeContext.CStr(), Input.CStr() );
			}
		}
		else
		{
			const uint KeyboardSignal	= GetBoundKeyboardSignal( OriginalInputHash );
			const uint MouseSignal		= GetBoundMouseSignal( OriginalInputHash );

			if( KeyboardSignal != Keyboard::EB_None )
			{
				// Enclosing keys in brackets to render them better?
				InputString				+= SimpleString::PrintF( "[#{s:%s:%s}]", m_KeyboardContext.CStr(), Input.CStr() );
			}

			if( KeyboardSignal != Keyboard::EB_None && MouseSignal != Mouse::EB_None )
			{
				InputString += "/";
			}

			if( MouseSignal != Mouse::EB_None )
			{
				InputString				+= SimpleString::PrintF( "#{l:%s:%s}", m_MouseContext.CStr(), Input.CStr() );
			}
		}
	}

	MAKEHASH( m_DefinitionName );
	MAKEHASHFROM( InputName, UsingInput );
	ConfigManager::SetString( sInputName, InputString.CStr(), sm_DefinitionName );
}

void InputSystem::PublishDisplayInputString( const SDisplayInput& DisplayInput )
{
	SimpleString InputString = "";

	if( m_IsUsingControllerExclusively )
	{
		if( DisplayInput.m_ControllerInput != XInputController::EB_None )
		{
			const SimpleString&	ControllerSignalName	= GetControllerSignalName( DisplayInput.m_ControllerInput );
			const SimpleString&	PreferredName			= GetPreferredControllerSignalName( ControllerSignalName );
			MAKEHASH( PreferredName );
			const SimpleString	ControllerInput			= ConfigManager::GetLocalizedString( sPreferredName, "" );
			InputString += ControllerInput;
		}
	}
	else
	{
		if( DisplayInput.m_KeyboardInput != Keyboard::EB_None )
		{
			// Enclosing keys in brackets to render them better
			InputString += SimpleString::PrintF( "[%s]", GetKeyboardSignalName( DisplayInput.m_KeyboardInput ).CStr() );
		}

		if( DisplayInput.m_KeyboardInput != Keyboard::EB_None && DisplayInput.m_MouseInput != Mouse::EB_None )
		{
			InputString += "/";
		}

		if( DisplayInput.m_MouseInput != Mouse::EB_None )
		{
			const SimpleString&	MouseSignalName	= GetMouseSignalName( DisplayInput.m_MouseInput );
			MAKEHASH( MouseSignalName );
			const SimpleString	MouseInput		= ConfigManager::GetLocalizedString( sMouseSignalName, "" );
			InputString += MouseInput;
		}
	}

	MAKEHASH( m_DefinitionName );
	MAKEHASHFROM( InputName, DisplayInput.m_Name );
	ConfigManager::SetString( sInputName, InputString.CStr(), sm_DefinitionName );
}
