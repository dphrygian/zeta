#ifndef WBCOMPROSAREADABLE_H
#define WBCOMPROSAREADABLE_H

#include "wbrosacomponent.h"
#include "simplestring.h"
#include "wbparamevaluator.h"

class WBCompRosaReadable : public WBRosaComponent
{
public:
	WBCompRosaReadable();
	virtual ~WBCompRosaReadable();

	DEFINE_WBCOMP( RosaReadable, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	SimpleString		m_String;
	WBParamEvaluator	m_StringPE;
	bool				m_IsDynamic;
};

#endif // WBCOMPROSAREADABLE_H
