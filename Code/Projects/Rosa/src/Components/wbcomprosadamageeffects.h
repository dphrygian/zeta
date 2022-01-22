#ifndef WBCOMPROSADAMAGEEFFECTS_H
#define WBCOMPROSADAMAGEEFFECTS_H

#include "wbrosacomponent.h"
#include "wbentityref.h"
#include "vector.h"
#include "map.h"

class Mesh;
class ITexture;

class WBCompRosaDamageEffects : public WBRosaComponent
{
public:
	WBCompRosaDamageEffects();
	virtual ~WBCompRosaDamageEffects();

	DEFINE_WBCOMP( RosaDamageEffects, WBRosaComponent );

	virtual void	Tick( const float DeltaTime );
	virtual int		GetTickOrder() { return ETO_TickDefault; }	// Should tick after transform

	virtual void	HandleEvent( const WBEvent& Event );

	virtual bool	IsRenderable() { return true; }
	virtual void	Render();

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	struct SDamageRecord
	{
		SDamageRecord()
		:	m_Location()
		,	m_Mesh( NULL )
		,	m_ExpireTime( 0.0f )
		{
		}

		Vector		m_Location;
		Mesh*		m_Mesh;
		float		m_ExpireTime;
	};

	void			OnDamaged( WBEntity* const pDamager );
	Mesh*			CreateMesh() const;
	void			UpdateDamageTransform( SDamageRecord& DamageRecord );
	void			UpdateDamageOverlay( const bool ForceUpdate );

	typedef Map<WBEntityRef, SDamageRecord>	TDamageMap;
	TDamageMap				m_DamageRecords;	// Transient
	float					m_Duration;			// Config; how long directional indicators last
	float					m_InvFadeDuration;	// Config; how long directional indicators fade
	SimpleString			m_Material;			// Config
	ITexture*				m_Texture;			// Config
	float					m_Radius;			// Config; radius of directional indicator ring relative to screen height
	float					m_Size;				// Config; extents of directional indicators relative to screen height

	HashedString			m_OverlayScreenName;	// Config
	HashedString			m_OverlayWidgetName;	// Config
	float					m_OverlayDuration;		// Config
	float					m_InvOverlayDuration;	// Config
	float					m_OverlayExpireTime;	// Transient
	float					m_LastOverlayAlpha;		// Transient

	bool					m_ConfigEnabled;		// Transient, mirrors the ShowHUD config option so it doesn't have to be queried every frame
};

#endif // WBCOMPROSADAMAGEEFFECTS_H
