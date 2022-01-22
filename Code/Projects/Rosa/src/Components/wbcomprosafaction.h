#ifndef WBCOMPROSAFACTION_H
#define WBCOMPROSAFACTION_H

#include "wbrosacomponent.h"
#include "rosafactions.h"

class WBCompRosaFaction : public WBRosaComponent
{
public:
	WBCompRosaFaction();
	virtual ~WBCompRosaFaction();

	DEFINE_WBCOMP( RosaFaction, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );
	virtual void	AddContextToEvent( WBEvent& Event ) const;

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

#if BUILD_DEV
	virtual void	Report() const;
#endif

	RosaFactions::EFactionCon			GetCon( const WBEntity* const pEntityB );
	RosaFactions::EFactionCon			GetCon( const HashedString& FactionB );
	static RosaFactions::EFactionCon	GetCon( const WBEntity* const pEntityA, const WBEntity* const pEntityB );

	HashedString	GetFaction() const { return m_Faction; }

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			PushPersistence() const;
	void			PullPersistence();

	HashedString	m_Faction;		// Serialized
	bool			m_Immutable;	// Config
};

#endif // WBCOMPROSAFACTION_H
