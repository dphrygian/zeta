#ifndef WBCOMPROSAANCHOR_H
#define WBCOMPROSAANCHOR_H

#include "wbrosacomponent.h"
#include "vector.h"

class WBCompRosaAnchor : public WBRosaComponent
{
public:
	WBCompRosaAnchor();
	virtual ~WBCompRosaAnchor();

	DEFINE_WBCOMP( RosaAnchor, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );
	virtual void	AddContextToEvent( WBEvent& Event ) const;

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	bool			IsAnchored() const { return m_IsAnchored; }

	// HACKHACK: Only works when called immediately after spawning.
	void			SetAnchorDirection( const Vector& AnchorDirection ) { m_AnchorDirection = AnchorDirection; }

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			SetAnchor();
	void			CheckAnchor();
	void			Unanchor();

	bool	m_IsAnchored;		// Serialized
	Vector	m_AnchorPoint;		// Serialized
	Vector	m_AnchorDirection;	// Config
};

#endif // WBCOMPROSAANCHOR_H
