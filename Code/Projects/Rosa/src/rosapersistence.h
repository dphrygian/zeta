#ifndef ROSAPERSISTENCE_H
#define ROSAPERSISTENCE_H

//#include "set.h"
#include "hashedstring.h"
#include "wbevent.h"
#include "iwbeventobserver.h"

// ROSANOTE: Eldritch used this class to persist values across
// multiple permadeath generations. Neon used it to persist values
// between levels without needing a player component to handle it.
// Vamp mostly replaced the latter with the campaign.

// ZETANOTE: This will be used for things that persist beyond one run,
// while RosaCampaign will be used for things that reset after a run.

// Old head/body stuff from Eldritch would now be done with push/pull
// persistence in the RosaCharacter component (a la skin/nails).

class IDataStream;

class RosaPersistence : public IWBEventObserver
{
public:
	RosaPersistence();
	~RosaPersistence();

	void			Save( const IDataStream& Stream ) const;
	void			Load( const IDataStream& Stream );

	// IWBEventObserver
	virtual void	HandleEvent( const WBEvent& Event );
	void			RegisterForEvents();

	void			Reset();

	void			Report() const;

	WBEvent&		GetVariableMap() { return m_VariableMap; }

private:
	// This is the only part of persistence that is used anymore, for marking
	// tutorials read, and for counting things for achievements. Revisit those
	// later (should achievements be per-profile or across all slots?).
	WBEvent			m_VariableMap;
};

#endif // ROSAPERSISTENCE_H
