#ifndef WBCOMPROSAMARKUP_H
#define WBCOMPROSAMARKUP_H

#include "wbrosacomponent.h"
#include "hashedstring.h"

class WBCompRosaMarkup : public WBRosaComponent
{
public:
	WBCompRosaMarkup();
	virtual ~WBCompRosaMarkup();

	DEFINE_WBCOMP( RosaMarkup, WBRosaComponent );

	virtual bool	BelongsInComponentArray() { return true; }

	virtual int		GetTickOrder() { return ETO_NoTick; }

	HashedString	GetMarkup() { return m_Markup; }

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	HashedString	m_Markup;
};

#endif // WBCOMPROSAMARKUP_H
