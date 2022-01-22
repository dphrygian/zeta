#ifndef WBCOMPROSAPARTICLES_H
#define WBCOMPROSAPARTICLES_H

#include "wbrosacomponent.h"
#include "array.h"
#include "simplestring.h"
#include "vector.h"
#include "angles.h"
#include "bonearray.h"

class RosaParticles;

class WBCompRosaParticles : public WBRosaComponent
{
public:
	WBCompRosaParticles();
	virtual ~WBCompRosaParticles();

	DEFINE_WBCOMP( RosaParticles, WBRosaComponent );

	virtual void	Tick( const float DeltaTime );
	virtual int		GetTickOrder() { return ETO_TickDefault; }	// Should tick after transform

	virtual bool	IsRenderable() { return true; }
	virtual void	Render();

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	bool			HasAttachBone() const;

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			AddParticleSystem( const SimpleString& DefinitionName, const bool Serialize, const bool Attached, const int AttachBoneIndex, const Vector* const pLocation, const Vector* const pBeamEndLocation, const Angles* const pOrientation );
	void			AddParticleSystem( const SimpleString& DefinitionName, const bool Serialize, const bool Attached, const int AttachBoneIndex, const Vector& Location, const bool IsBeam, const Vector& BeamEndLocation, const Angles& Orientation );
	void			StopParticleSystem( const HashedString& DefinitionNameHash );
	void			StopParticleSystems();
	void			ExpireParticleSystem( const HashedString& DefinitionNameHash );
	void			ExpireParticleSystems();

	bool			IsWithinCullDistance() const;
	bool			IntersectsAnyVisibleSector() const;

	struct SParticleSystem
	{
		SParticleSystem()
		:	m_System( NULL )
		,	m_Attached( false )
		,	m_AttachBoneIndex( INVALID_INDEX )
		,	m_Serialize( false )
		,	m_DefinitionName()
		,	m_Location()
		,	m_IsBeam( false )
		,	m_BeamEndLocation()
		,	m_Orientation()
		,	m_DefinitionNameHash()
		{
		}

		RosaParticles*		m_System;
		bool				m_Attached;
		int					m_AttachBoneIndex;

		// Initialization parameters, so we can serialize active systems
		bool				m_Serialize;
		SimpleString		m_DefinitionName;
		Vector				m_Location;
		bool				m_IsBeam;
		Vector				m_BeamEndLocation;
		Angles				m_Orientation;

		// For quick removal
		HashedString		m_DefinitionNameHash;
	};

	Array<SParticleSystem>	m_ParticleSystems;
	bool					m_Hidden;

	float					m_CullDistanceSq;	// Config: If non-zero, mesh is only drawn within this distance from camera
};

#endif // WBCOMPROSAPARTICLES_H
