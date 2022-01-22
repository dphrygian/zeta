#include "core.h"
#include "wbactionstack.h"
#include "array.h"
#include "wbevent.h"

static Array<const WBEvent*>*	sStack;
static Array<WBEntity*>*		sActingEntityStack;

void WBActionStack::Initialize()
{
	DEVASSERT( !sStack );
	sStack = new Array<const WBEvent*>;
	sStack->SetDeflate( false );

	DEVASSERT( !sActingEntityStack );
	sActingEntityStack = new Array<WBEntity*>;
	sActingEntityStack->SetDeflate( false );
}

void WBActionStack::ShutDown()
{
	SafeDelete( sStack );

	SafeDelete( sActingEntityStack );
}

void WBActionStack::Push( const WBEvent& Event, WBEntity* const pActingEntity )
{
	DEVASSERT( sStack );
	sStack->PushBack( &Event );

	DEVASSERT( sActingEntityStack );
	sActingEntityStack->PushBack( pActingEntity );
}

void WBActionStack::Pop()
{
	DEVASSERT( sStack );
	sStack->PopBack();

	DEVASSERT( sActingEntityStack );
	sActingEntityStack->PopBack();
}

const WBEvent& WBActionStack::TopEvent()
{
	DEVASSERT( sStack );
	DEVASSERT( sStack->Size() );
	return *(sStack->Last());
}

WBEntity* WBActionStack::TopActingEntity()
{
	DEVASSERT( sActingEntityStack );
	DEVASSERT( sActingEntityStack->Size() );
	return sActingEntityStack->Last();
}
