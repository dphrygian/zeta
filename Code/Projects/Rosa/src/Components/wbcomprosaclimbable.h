#ifndef WBCOMPROSACLIMBABLE_H
#define WBCOMPROSACLIMBABLE_H

#include "wbrosacomponent.h"
#include "plane.h"

class WBCompRosaClimbable : public WBRosaComponent
{
public:
	WBCompRosaClimbable();
	virtual ~WBCompRosaClimbable();

	DEFINE_WBCOMP( RosaClimbable, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );
	virtual void	AddContextToEvent( WBEvent& Event ) const;

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			InitializeSnapPlane();

	bool			m_UseSnapPlane;	// Config
	Plane			m_SnapPlane;	// Serialized
};

#endif // WBCOMPROSACLIMBABLE_H
