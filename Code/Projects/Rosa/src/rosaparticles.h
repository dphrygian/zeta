#ifndef ROSAPARTICLES_H
#define ROSAPARTICLES_H

#include "vector.h"
#include "vector4.h"
#include "vector2.h"
#include "angles.h"
#include "list.h"
#include "array.h"
#include "matrix.h"
#include "3d.h"
#include "map.h"
#include "simplestring.h"

class RosaMesh;
class ITexture;
class IVertexBuffer;

class RosaParticles
{
public:
	RosaParticles();
	~RosaParticles();

	void			InitializeFromDefinition( const SimpleString& DefinitionName );

	void			Tick( const float DeltaTime );
	void			Render();

	Vector			GetLocation() const							{ return m_SystemLocation; }
	void			SetLocation( const Vector& Location );
	Vector			GetBeamEndLocation() const					{ return m_SystemBeamEndLocation; }
	void			SetBeamEndLocation( const Vector& BeamEndLocation );
	void			SetOrientation( const Angles& Orientation )	{ m_SystemOrientation = Orientation; }

	void			SetIsBeam( const bool IsBeam )				{ m_IsBeam = IsBeam; }
	bool			IsBeam() const								{ return m_IsBeam; }

	const RosaMesh*	GetDynamicMesh() const { return m_DynamicMesh; }

	void			Expire();
	bool			IsExpired() const;
	bool			IsFinished() const;

	static void		FlushConfigCache();

private:
	struct SParticle
	{
		SParticle();

		float	m_SpawnTime;		// Timestamp, used to determine fade phases
		float	m_ExpireTime;		// Timestamp, used to determine lifetime and fade phases
		int		m_TickLifetime;		// ROSANOTE: It's fine if this counts down, subtracting deltas for ticks isn't a problem
		float	m_Size;
		float	m_SizeVelocity;
		Vector4	m_Color;
		Vector	m_Location;
		Vector	m_Velocity;
		float	m_Roll;
		float	m_RollVelocity;
	};

	struct SParticleMeshParams
	{
		SParticleMeshParams()
		:	m_ViewLocation()
		,	m_ViewAxis()
		,	m_AngleMatrix()
		,	m_TracerX()
		{
		}

		Vector	m_ViewLocation;
		Vector	m_ViewAxis;
		Matrix	m_AngleMatrix;
		Vector	m_TracerX;
	};

	void			CreateMesh();
	void			InitVertexBuffers( IVertexBuffer* pVertexBuffer );
	void			UpdateMesh();
	void			AddParticleToMesh( const uint Index, const SParticle& Particle, const SParticleMeshParams& MeshParams );
	void			AddTracerToMesh( const uint Index, const SParticle& Particle, const SParticleMeshParams& MeshParams );
	void			TickParticles( const float DeltaTime );
	bool			TickParticle( SParticle& Particle, const float DeltaTime );	// Returns false if this particle has expired
	uint			GetNumParticles() const;
	void			SpawnParticles( uint NumParticlesToSpawn );
	Vector			GetParticleSpawnLocationOffset() const;

	static void		DeviceResetCallback( void* pVoid, IVertexBuffer* pBuffer );

	void			GetCachedConfig( const SimpleString& DefinitionName );

	float			GetTime() const;
	bool			SystemIsFinite() const;

	// Configurable parameters
	struct SParticleSystemParams
	{
		SParticleSystemParams()
		:	m_SystemLifetime( 0.0f )
		,	m_SpawnRate( 0.0f )
		,	m_InvSpawnRate( 0.0f )
		,	m_TickSpawnRate( 0 )
		,	m_MaxParticles( 0 )
		,	m_ParticlesPerMeter( 0.0f )
		,	m_AlbedoMapName()
		,	m_NormalMapName()
		,	m_SpecMapName()
		,	m_IsTracer( false )
		,	m_ImmediateSpawnMax( false )
		,	m_AlwaysDraw( false )
		,	m_ForegroundDraw( false )
		,	m_Collision( false )
		,	m_MaterialOverride()
		,	m_SpawnOffsetOS()
		,	m_SpawnOffsetWS()
		,	m_SpawnExtents()
		,	m_InitialVelocityOSMin()
		,	m_InitialVelocityOSMax()
		,	m_InitialVelocityWSMin()
		,	m_InitialVelocityWSMax()
		,	m_InitialVelocityWindMin( 0.0f )
		,	m_InitialVelocityWindMax( 0.0f )
		,	m_BeamVelocityMin( 0.0f )
		,	m_BeamVelocityMax( 0.0f )
		,	m_TracerLength( 0.0f )
		,	m_Elasticity( 0.0f )
		,	m_InitialRollMin( 0.0f )
		,	m_InitialRollMax( 0.0f )
		,	m_RollVelocityMin( 0.0f )
		,	m_RollVelocityMax( 0.0f )
		,	m_LifetimeMin( 0.0f )
		,	m_LifetimeMax( 0.0f )
		,	m_TickLifetime( 0 )
		,	m_FadeInTime( 0.0f )
		,	m_FadeOutTime( 0.0f )
		,	m_InvFadeInTime( 0.0f )
		,	m_InvFadeOutTime( 0.0f )
		,	m_AccelerationWS()
		,	m_ViewBoundRadius( 0.0f )
		,	m_InitialSizeMin( 0.0f )
		,	m_InitialSizeMax( 0.0f )
		,	m_SizeVelocityMin( 0.0f )
		,	m_SizeVelocityMax( 0.0f )
		,	m_ColorMin()
		,	m_ColorMax()
		,	m_LinkedRGB( false )
		,	m_InitialTrace( false )
		,	m_InitialTraceOffsetOS()
		,	m_UpdateMeshTickRate( 0.0f )
		{
		}

		float			m_SystemLifetime;
		float			m_SpawnRate;
		float			m_InvSpawnRate;
		uint			m_TickSpawnRate;
		uint			m_MaxParticles;
		float			m_ParticlesPerMeter;
		SimpleString	m_AlbedoMapName;
		SimpleString	m_NormalMapName;
		SimpleString	m_SpecMapName;
		bool			m_IsTracer;
		bool			m_ImmediateSpawnMax;
		bool			m_AlwaysDraw;
		bool			m_ForegroundDraw;
		bool			m_Collision;
		SimpleString	m_MaterialOverride;
		Vector			m_SpawnOffsetOS;
		Vector			m_SpawnOffsetWS;
		Vector			m_SpawnExtents;
		Vector			m_InitialVelocityOSMin;
		Vector			m_InitialVelocityOSMax;
		Vector			m_InitialVelocityWSMin;
		Vector			m_InitialVelocityWSMax;
		float			m_InitialVelocityWindMin;
		float			m_InitialVelocityWindMax;
		float			m_BeamVelocityMin;
		float			m_BeamVelocityMax;
		float			m_TracerLength;
		float			m_Elasticity;
		float			m_InitialRollMin;
		float			m_InitialRollMax;
		float			m_RollVelocityMin;
		float			m_RollVelocityMax;
		float			m_LifetimeMin;
		float			m_LifetimeMax;
		int				m_TickLifetime;
		float			m_FadeInTime;
		float			m_FadeOutTime;
		float			m_InvFadeInTime;
		float			m_InvFadeOutTime;
		Vector			m_AccelerationWS;
		float			m_ViewBoundRadius;
		float			m_InitialSizeMin;
		float			m_InitialSizeMax;
		float			m_SizeVelocityMin;
		float			m_SizeVelocityMax;
		Vector4			m_ColorMin;
		Vector4			m_ColorMax;
		bool			m_LinkedRGB;
		bool			m_InitialTrace;
		Vector			m_InitialTraceOffsetOS;
		float			m_UpdateMeshTickRate;	// To match stylized animation
	};

	SParticleSystemParams	m_Params;

	Vector					m_SystemLocation;			// Emitter base location
	Vector					m_SystemBeamEndLocation;	// Optional, spawn particles on segment from m_SystemLocation to here
	Angles					m_SystemOrientation;

	float					m_SpawnAccumulator;
	float					m_ExpireTime;
	float					m_FinishTime;
	bool					m_RenderedLastFrame;
	bool					m_FirstTick;
	bool					m_IsBeam;

	bool					m_ShouldUpdateMesh;			// Transient
	float					m_NextUpdateMeshTime;		// Transient

	RosaMesh*				m_DynamicMesh;
	ITexture*				m_AlbedoMap;
	ITexture*				m_NormalMap;
	ITexture*				m_SpecMap;

	Array<SParticle>		m_ParticlesArray;

	// Persistently allocated local vertex buffers for pushing each frame
	Array<Vector>			m_VB_Positions;
	Array<Vector4>			m_VB_FloatColors;
	Array<Vector2>			m_VB_UVs;
	Array<Vector>			m_VB_Normals;
	Array<Vector4>			m_VB_Tangents;
	Array<index_t>			m_VB_Indices;

	typedef Map<HashedString, RosaParticles::SParticleSystemParams> TParamCache;
	static TParamCache		sm_ParamCache;
};

#endif // ROSAPARTICLES_H
