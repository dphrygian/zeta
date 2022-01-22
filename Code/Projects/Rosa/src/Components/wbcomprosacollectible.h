#ifndef WBCOMPROSACOLLECTIBLE_H
#define WBCOMPROSACOLLECTIBLE_H

#include "wbrosacomponent.h"

class WBCompRosaCollectible : public WBRosaComponent
{
public:
	WBCompRosaCollectible();
	virtual ~WBCompRosaCollectible();

	DEFINE_WBCOMP( RosaCollectible, WBRosaComponent );

	virtual bool	BelongsInComponentArray() { return true; }

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );
};

#endif // WBCOMPROSACOLLECTIBLE_H
