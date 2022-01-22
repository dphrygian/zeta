#ifndef WBCOMPROSAICICLES_H
#define WBCOMPROSAICICLES_H

#include "wbrosacomponent.h"

class WBCompRosaIcicles : public WBRosaComponent
{
public:
	WBCompRosaIcicles();
	virtual ~WBCompRosaIcicles();

	DEFINE_WBCOMP( RosaIcicles, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_TickDefault; }
	virtual void	Tick( const float DeltaTime );

	virtual void	HandleEvent( const WBEvent& Event );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	float	m_CheckDistance;	// Config
};

#endif // WBCOMPROSAICICLES_H
