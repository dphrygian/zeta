#ifndef WBCOMPROSAROPE_H
#define WBCOMPROSAROPE_H

#include "wbrosacomponent.h"
#include "vector.h"
#include "simplestring.h"

class WBCompRosaRope : public WBRosaComponent
{
public:
	WBCompRosaRope();
	virtual ~WBCompRosaRope();

	DEFINE_WBCOMP( RosaRope, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			DropRope();

	Vector			Quantize( const Vector& V ) const;

	float			m_CollisionFatten;	// Config
	float			m_MeshFatten;		// Config

	float			m_EndpointSpacing;	// Config
	float			m_AnchorDepth;		// Config
	float			m_HookLength;		// Config
	float			m_DangleHeight;		// Config
	SimpleString	m_HookEntity;		// Config

	Vector			m_Anchor;			// Serialized

	bool			m_Dropped;			// Serialized
};

#endif // WBCOMPROSAROPE_H
