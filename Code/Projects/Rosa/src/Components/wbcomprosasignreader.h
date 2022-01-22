#ifndef WBCOMPROSASIGNREADER_H
#define WBCOMPROSASIGNREADER_H

#include "wbrosacomponent.h"
#include "wbentityref.h"

class WBCompRosaSignReader : public WBRosaComponent
{
public:
	WBCompRosaSignReader();
	virtual ~WBCompRosaSignReader();

	DEFINE_WBCOMP( RosaSignReader, WBRosaComponent );

	virtual void	Tick( const float DeltaTime );
	virtual int		GetTickOrder() { return ETO_TickDefault; }	// Needs to tick after transform.

private:
	WBEntity*	FindTargetSign() const;

	void		OnSetSignTarget( WBEntity* const pSignTarget );
	void		OnUnsetSignTarget( WBEntity* const pSignTarget );

	WBEntityRef	m_SignTarget;		// Transient
};

#endif // WBCOMPROSASIGNREADER_H
