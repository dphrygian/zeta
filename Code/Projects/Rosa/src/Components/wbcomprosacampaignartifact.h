#ifndef WBCOMPROSACAMPAIGNARTIFACT_H
#define WBCOMPROSACAMPAIGNARTIFACT_H

#include "wbrosacomponent.h"

class WBCompRosaCampaignArtifact : public WBRosaComponent
{
public:
	WBCompRosaCampaignArtifact();
	virtual ~WBCompRosaCampaignArtifact();

	DEFINE_WBCOMP( RosaCampaignArtifact, WBRosaComponent );

	virtual bool	BelongsInComponentArray() { return true; }

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );
	virtual void	AddContextToEvent( WBEvent& Event ) const;

	const HashedString&	GetTag() const		{ return m_Tag; }
	uint				GetCapacity() const	{ return m_Capacity; }

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	HashedString	m_Tag;		// Config
	uint			m_Capacity;	// Config
};

#endif // WBCOMPROSACAMPAIGNARTIFACT_H
