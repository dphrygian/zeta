#ifndef WBCOMPROSAREPULSOR_H
#define WBCOMPROSAREPULSOR_H

#include "wbrosacomponent.h"

class WBCompRosaRepulsor : public WBRosaComponent
{
public:
	WBCompRosaRepulsor();
	virtual ~WBCompRosaRepulsor();

	DEFINE_WBCOMP( RosaRepulsor, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }
	virtual bool	BelongsInComponentArray() { return true; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	bool			IsActive() const	{ return m_Active; }
	bool			IsDirected() const	{ return m_Directed; }
	float			GetRadius() const	{ return m_Radius; }

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	bool	m_Active;	// Config/serialized
	bool	m_Directed;	// Config
	float	m_Radius;	// Config
};

#endif // WBCOMPROSAREPULSOR_H
