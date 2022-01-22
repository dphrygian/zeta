#ifndef INPUTSYSTEM_H
#define INPUTSYSTEM_H

// Data-driven input mapping system, subsumes and replaces InputMap for new projects.

#include "icontroller.h"
#include "hashedstring.h"
#include "simplestring.h"
#include "array.h"
#include "map.h"
#include "inputcontext.h"
#include "set.h"

class Keyboard;
class Mouse;
class XInputController;
class IInputSystemObserver;
class Clock;

#define INPUT_NONE		0x0
#define INPUT_ISLOW		0x0
#define INPUT_ISHIGH	0x1
#define INPUT_ONRISE	0x2
#define INPUT_ONHOLD	0x4
#define INPUT_ONFALL	0x8
#define INPUT_TEST( i, f ) ( ( ( i ) & ( f ) ) == ( f ) )

class InputSystem
{
public:
	InputSystem();
	~InputSystem();

	void	Initialize( const SimpleString& DefinitionName );
	void	InitializeSignalMaps();

	void	SetKeyboard( Keyboard* const pKeyboard )				{ m_Keyboard = pKeyboard; }
	void	SetMouse( Mouse* const pMouse )							{ m_Mouse = pMouse; }
	void	SetController( XInputController* const pController )	{ m_Controller = pController; }
	void	SetClock( Clock* const pClock )							{ m_Clock = pClock; }

	Keyboard*			GetKeyboard() const		{ return m_Keyboard; }
	Mouse*				GetMouse() const		{ return m_Mouse; }
	XInputController*	GetController() const	{ return m_Controller; }
	Clock*				GetClock() const		{ return m_Clock; }

	void	SetRelativeFrameTime( const float RelativeFrameTime ) { m_RcpRelativeFrameTime = ( RelativeFrameTime > 0.0f ) ? ( 1.0f / RelativeFrameTime ) : 1.0f; }

	void	Tick();

	void	UpdateIsUsingControllerExclusively();

	bool	IsBinding() const { return m_Binding; }

	void	PushContext( const HashedString& ContextName );
	void	PopContext( const HashedString& ContextName );	// Removes the topmost instance of context regardless of position in stack
	void	PopAllContexts();
	uint	GetNumActiveContexts() { return m_ActiveInputContexts.Size(); }

	bool	IsSuppressed( const HashedString& Input ) const;

	bool	IsHigh( const HashedString& Signal ) const;
	bool	IsLow( const HashedString& Signal ) const;
	bool	OnRise( const HashedString& Signal ) const;
	bool	OnHold( const HashedString& Signal ) const;
	bool	OnFall( const HashedString& Signal ) const;

	// Convenience function for things that deal with any input (high, rise, hold, or fall).
	uint	OnInput( const HashedString& Signal ) const;

	float	GetPosition( const HashedString& Axis ) const;
	float	GetVelocity( const HashedString& Axis ) const;

	void	SetMouseScale( const HashedString& Axis, const float Scale );
	void	SetMouseInvert( const HashedString& Axis, const bool Invert );
	void	SetControllerScale( const HashedString& Axis, const float Scale );
	void	SetControllerPower( const HashedString& Axis, const float Power );
	void	SetControllerInvert( const HashedString& Axis, const bool Invert );

	bool	GetMouseInvert( const HashedString& Axis );
	bool	GetControllerInvert( const HashedString& Axis );

	void	BindInput( const SimpleString& InputName );
	void	WriteConfigBinds( const IDataStream& Stream );

	const Array<SimpleString>&	GetExposedInputs() const { return m_ExposedInputs; }

	uint					GetBoundKeyboardSignal( const HashedString& Input ) const;		// E.g. "Jump" -> Keyboard::EB_Space
	uint					GetBoundMouseSignal( const HashedString& Input ) const;
	uint					GetBoundControllerSignal( const HashedString& Input ) const;
	SimpleString			GetInputBoundToKeyboard( const uint Signal ) const;				// E.g. Keyboard::EB_Space -> "Jump"
	SimpleString			GetInputBoundToMouse( const uint Signal ) const;
	SimpleString			GetInputBoundToController( const uint Signal ) const;

	uint					GetKeyboardSignal( const HashedString& Name ) const;			// E.g. "Space" -> Keyboard::EB_Space
	uint					GetMouseSignal( const HashedString& Name ) const;
	uint					GetControllerSignal( const HashedString& Name ) const;
	const SimpleString&		GetKeyboardSignalName( const uint Signal ) const;				// E.g. Keyboard::EB_Space -> "Space"
	const SimpleString&		GetMouseSignalName( const uint Signal ) const;
	const SimpleString&		GetControllerSignalName( const uint Signal ) const;
	const SimpleString&		GetPreferredControllerSignalName( const SimpleString& Name ) const;	// E.g. "xA" -> "ps_X"

	uint					GetBoundMouseAnalogSignal( const HashedString& Input ) const;
	uint					GetBoundControllerAnalogSignal( const HashedString& Input ) const;
	SimpleString			GetAnalogInputBoundToMouse( const uint Signal ) const;
	SimpleString			GetAnalogInputBoundToController( const uint Signal ) const;

	uint					GetMouseAnalogSignal( const HashedString& Name ) const;
	uint					GetControllerAnalogSignal( const HashedString& Name ) const;
	const SimpleString&		GetMouseAnalogSignalName( const uint Signal ) const;
	const SimpleString&		GetControllerAnalogSignalName( const uint Signal ) const;

	void					AddInputSystemObserver( IInputSystemObserver* const pObserver );
	void					RemoveInputSystemObserver( IInputSystemObserver* const pObserver );

	void					CycleControllerType();
	bool					IsUsingControllerExclusively() const { return m_IsUsingControllerExclusively; }
	bool					ShouldPauseDueToLostDevice() const { return m_ShouldPauseDueToLostDevice; }

	// Made public so this can be called when resetting everything
	void					UpdateBindingsFromConfig();

	void					SetControllerType( const HashedString& ControllerType );

private:
	friend class InputContext;
	struct SControllerGear;
	struct SAnalogAdjustment;
	struct SDisplayInput;

	HashedString		GetContextualInput( const HashedString& Input ) const;			// Get the current contextual input for a base input, e.g., Jump -> HackUse

	void				TickBinding();

	// Return true if binding input was bound to signal
	bool				TryKeyboardBind( const uint Signal );
	bool				TryMouseBind( const uint KeyboardSignal, const uint MouseSignal );
	bool				TryControllerBind( const uint Signal );
	bool				TryMouseBindAnalog( const uint Signal );
	bool				TryControllerBindAnalog( const uint Signal );

	bool				CanBindToKeyboard( const HashedString& Input ) const;
	bool				CanBindToMouse( const HashedString& Input ) const;
	bool				CanBindToController( const HashedString& Input ) const;
	bool				CanBindToMouseAnalog( const HashedString& Input ) const;
	bool				CanBindToControllerAnalog( const HashedString& Input ) const;

	void				NotifyObserversOnInputContextsChanged() const;

	float				GetTime() const;

	void				TickControllerGear( SControllerGear& Gear, const float InputPosition );
	float				ModifyControllerInputPosition( const float InputPosition, const SAnalogAdjustment& Adjustment );

	void				PublishInputStrings();
	void				PublishControllerGlyphString( const SimpleString& Input );
	void				PublishInputString( const SimpleString& Input );
	void				PublishDisplayInputString( const SDisplayInput& DisplayInput );

	Keyboard*			m_Keyboard;
	Mouse*				m_Mouse;
	XInputController*	m_Controller;
	Clock*				m_Clock;

	SimpleString		m_DefinitionName;
	bool				m_IsUsingControllerExclusively;
	bool				m_ShouldPauseDueToLostDevice;

	// Which section of config file each device reads/writes in
	SimpleString		m_KeyboardContext;
	SimpleString		m_MouseContext;
	SimpleString		m_ControllerContext;
	SimpleString		m_ControllerTypeContext;	// For storing the input-to-glyph map (e.g., "Jump" to "ps_X")

	// HACKHACK for mouse adjustment
	float				m_RcpRelativeFrameTime;

	struct SInput
	{
		HashedString	m_Hash;
		SimpleString	m_String;
	};

	struct SAnalogInput
	{
		HashedString	m_Hash;
		SimpleString	m_String;
		bool			m_Clamp;
		float			m_ClampMin;
		float			m_ClampMax;
		// TODO: Maybe provide southpaw/legacy options as an axial inversion in here?
	};

	struct SAnalogAdjustment
	{
		SAnalogAdjustment()
		:	m_ScaleFactor( 1.0f )
		,	m_PowerFactor( 1.0f )
		,	m_InvertFactor( false )
		{
		}

		float	m_ScaleFactor;
		float	m_PowerFactor;
		bool	m_InvertFactor;
	};

	// HACKHACK for things that only need to be displayed (e.g. UI input prompts)
	struct SDisplayInput
	{
		SDisplayInput()
		:	m_Name()
		,	m_KeyboardInput( 0 )
		,	m_MouseInput( 0 )
		,	m_ControllerInput( 0 )
		{
		}

		SimpleString	m_Name;
		uint			m_KeyboardInput;
		uint			m_MouseInput;
		uint			m_ControllerInput;
	};

	// Array of named inputs (e.g., "Jump", "Fire")
	Array<SInput>						m_Inputs;
	Array<SAnalogInput>					m_AnalogInputs;
	Array<SimpleString>					m_ExposedInputs;	// For player-facing stuff like the bind screen
	Array<SDisplayInput>				m_DisplayInputs;	// HACKHACK for things that only need to be displayed (e.g. UI input prompts)

	// Input contexts to filter or redirect inputs (e.g., "Jump" remapped to "Fire")
	Map<HashedString, InputContext*>	m_InputContexts;
	Array<InputContext*>				m_ActiveInputContexts;

	// Map from input name to the device signals (e.g., "Jump" to space bar)
	Map<HashedString, uint>				m_KeyboardBindings;
	Map<HashedString, uint>				m_MouseBindings;
	Map<HashedString, uint>				m_ControllerBindings;
	Map<HashedString, uint>				m_MouseAnalogBindings;		// Analog bindings need to be separate only because the devices
	Map<HashedString, uint>				m_ControllerAnalogBindings;	// have different enumerations for bool and float signals.

	Map<HashedString, SAnalogAdjustment>	m_MouseAdjustments;
	Map<HashedString, SAnalogAdjustment>	m_ControllerAdjustments;

	// See also GetPreferredControllerSignalName
	typedef Map<HashedString, SimpleString>	TControllerType;	// Map from default to preferred signal name, e.g. "xA" -> "ps_X"
	struct SControllerType
	{
		SControllerType()
		:	m_Name()
		,	m_Map()
		{
		}

		SimpleString	m_Name;
		TControllerType	m_Map;
	};

	Array<SControllerType>	m_ControllerTypes;	// Map from controller type name (e.g., "Controller_PS") to data
	uint					m_ControllerType;	// Current preference

	// "Gear" in the car sense. Shifting between high and low speeds.
	// NOTE: If gear config is not defined for an input, it is assumed
	// to always be in low gear.
	struct SControllerGear
	{
		SControllerGear()
		:	m_HighGear( false )
		,	m_Shifting( false )
		,	m_HighGearScalar( 1.0f )
		,	m_HighGearThreshold( 0.0f )
		,	m_LowGearScalar( 1.0f )
		,	m_LowGearThreshold( 0.0f )
		,	m_HighGearShiftTime( 0.0f )
		,	m_LowGearShiftTime( 0.0f )
		,	m_ShiftStartTime( 0.0f )
		{
		}

		bool	m_HighGear;				// Transient; are we in high gear
		bool	m_Shifting;				// Transient; are we currently shifting to the other gear
		float	m_HighGearScalar;		// Config; analog input scalar in high gear
		float	m_HighGearThreshold;	// Config; threshold to shift into high gear (after applying controller dead zones, but before applying any scale/power/invert stuff)
		float	m_LowGearScalar;		// Config; analog input scalar in low gear
		float	m_LowGearThreshold;		// Config; threshold to shift into low gear
		float	m_HighGearShiftTime;	// Config; time spent shifting *into* high gear
		float	m_LowGearShiftTime;		// Config; time spent shifting *into* low gear
		float	m_ShiftStartTime;		// Transient; when shift began
	};

	Map<HashedString, SControllerGear>	m_ControllerGears;

	Set<HashedString>					m_UnbindableKeyboardInputs;
	Set<HashedString>					m_UnbindableMouseInputs;
	Set<HashedString>					m_UnbindableControllerInputs;
	Set<HashedString>					m_UnbindableMouseAnalogInputs;
	Set<HashedString>					m_UnbindableControllerAnalogInputs;

	// Map from key names to signals and back (e.g., "Space" <-> space bar)
	Map<HashedString, uint>				m_KeyboardMap;
	Map<uint, SimpleString>				m_ReverseKeyboardMap;
	Map<HashedString, uint>				m_MouseMap;
	Map<uint, SimpleString>				m_ReverseMouseMap;
	Map<HashedString, uint>				m_ControllerMap;
	Map<uint, SimpleString>				m_ReverseControllerMap;
	Map<HashedString, uint>				m_MouseAnalogMap;
	Map<uint, SimpleString>				m_ReverseMouseAnalogMap;
	Map<HashedString, uint>				m_ControllerAnalogMap;
	Map<uint, SimpleString>				m_ReverseControllerAnalogMap;

	float								m_HoldTime;

	// Cached state of each high-level input.
	// Instead of copying current to last state, just swap pointers each tick.
	typedef Map<HashedString, bool> InputState;
	InputState	m_StateA;
	InputState	m_StateB;
	InputState*	m_CurrentState;
	InputState*	m_LastState;

	struct SHoldState
	{
		SHoldState()
		:	m_Held( false )
		,	m_WaitingForHold( false )
		,	m_RiseTime( 0.0f )
		{
		}

		bool	m_Held;
		bool	m_WaitingForHold;
		float	m_RiseTime;
	};

	typedef Map<HashedString, SHoldState> HoldState;
	HoldState	m_HoldState;	// No need to double buffer this; and in fact, it would cause problems!

	typedef Map<HashedString, float> AnalogInputState;
	AnalogInputState	m_AnalogStateA;
	AnalogInputState	m_AnalogStateB;
	AnalogInputState*	m_CurrentAnalogState;
	AnalogInputState*	m_LastAnalogState;

	// For run-time input binding
	bool			m_Binding;
	bool			m_BindingInputIsAnalog;
	SimpleString	m_BindingInput;

	bool			m_CanBindKeyboard;
	bool			m_CanBindMouse;
	bool			m_CanBindController;
	bool			m_CanBindMouseAnalog;
	bool			m_CanBindControllerAnalog;

	Array<IInputSystemObserver*>	m_InputSystemObservers;
};

#endif // INPUTSYSTEM_H
