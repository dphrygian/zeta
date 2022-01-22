#ifndef WBCOMPROSAHACKABLE_H
#define WBCOMPROSAHACKABLE_H

#include "wbrosacomponent.h"

class WBCompRosaHackable : public WBRosaComponent
{
public:
	WBCompRosaHackable();
	virtual ~WBCompRosaHackable();

	DEFINE_WBCOMP( RosaHackable, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );
	virtual void	AddContextToEvent( WBEvent& Event ) const;

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	HashedString	m_BoardDef;
};

#endif // WBCOMPROSAHACKABLE_H
