#ifndef WBACTIONSTACK_H
#define WBACTIONSTACK_H

// Stack frame for the currently executed WBAction.
// To simplify a bunch of code, this just reuses WBEvents as the stack frame.

class WBEvent;
class WBEntity;

namespace WBActionStack
{
	void Initialize();
	void ShutDown();

	void Push( const WBEvent& Event, WBEntity* const pActingEntity );
	void Pop();

	const WBEvent&	TopEvent();
	WBEntity*		TopActingEntity();
}

#endif // WBACTIONSTACK_H
