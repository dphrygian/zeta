#ifndef WBCOMPROSALINKEDENTITIES_H
#define WBCOMPROSALINKEDENTITIES_H

// Dummy component that holds linked entities for any other component that might need it. (fka loop metadata)

#include "wbrosacomponent.h"
#include "rosaworldgen.h"

class WBCompRosaLinkedEntities : public WBRosaComponent
{
public:
	WBCompRosaLinkedEntities();
	virtual ~WBCompRosaLinkedEntities();

	DEFINE_WBCOMP( RosaLinkedEntities, WBRosaComponent );

	// Added for trailer cam lookup
	virtual bool	BelongsInComponentArray() { return true; }
	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	const Array<WBEntityRef>&	GetLinkedEntities() const { return m_LinkedEntities; }

private:
	Array<WBEntityRef>	m_LinkedEntities;
};

#endif // WBCOMPROSALINKEDENTITIES_H
