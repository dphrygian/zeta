#ifndef WBCOMPROSARESPAWNER_H
#define WBCOMPROSARESPAWNER_H

#include "wbrosacomponent.h"
#include "vector.h"
#include "angles.h"

class WBCompRosaRespawner : public WBRosaComponent
{
public:
	WBCompRosaRespawner();
	virtual ~WBCompRosaRespawner();

	DEFINE_WBCOMP( RosaRespawner, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void	TryRespawn();
	void	Respawn();

	bool	CanRespawn();
	bool	IsOriginNearPlayer();
	bool	CanOriginBeSeenByPlayer();

	bool	m_OriginSet;			// Serialized
	Vector	m_OriginLocation;		// Serialized
	Angles	m_OriginOrientation;	// Serialized

	float	m_RetryRespawnTime;				// Config
	float	m_RespawnMinPlayerDistanceSq;	// Config
};

#endif // WBCOMPROSARESPAWNER_H
