#ifndef WBCOMPROSASPIKES_H
#define WBCOMPROSASPIKES_H

#include "wbrosacomponent.h"

class WBCompRosaSpikes : public WBRosaComponent
{
public:
	WBCompRosaSpikes();
	virtual ~WBCompRosaSpikes();

	DEFINE_WBCOMP( RosaSpikes, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void	HandleOnTouched( WBEntity* const pTouched );
	bool	ShouldSendSpikedEvent( WBEntity* const pTouched );
	void	SendSpikedEvent( WBEntity* const pTouched );

	float	m_SpeedThresholdSq;			// Config
	bool	m_CheckMovingDown;			// Config
	float	m_RecentlyLandedThreshold;	// Config, only applies to CheckMovingDown
};

#endif // WBCOMPROSASPIKES_H
