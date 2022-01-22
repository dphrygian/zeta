#ifndef WBWORLD_H
#define WBWORLD_H

#include "map.h"

class WBEventManager;
class WBEntity;
class WBScene;
class Clock;

class WBWorld
{
public:
	static void		CreateInstance();
	static bool		HasInstance();
	static WBWorld*	GetInstance();
	static void		DeleteInstance();

	WBEventManager*	GetEventManager() { return m_EventManager; }

	void			Initialize();
	void			ShutDown();

	void			Tick( const float DeltaTime );

	// In case game ticks and renders at different rates. We always need to submit a renderable even when we don't tick.
	void			Render() const;

	WBEntity*		GetEntity( const uint EntityUID ) const;
	WBEntity*		CreateEntity( const SimpleString& Archetype, WBScene* Scene = NULL );
	uint			AddEntity( WBEntity* const Entity );
	void			AddEntity( WBEntity* const Entity, const uint EntityUID );
	void			RemoveEntity( const uint EntityUID );

	WBScene*		GetDefaultScene() const { return m_DefaultScene; }
	WBScene*		GetScene( const uint SceneUID ) const;	// SRF: Not currently used
	WBScene*		CreateScene();
	uint			AddScene( WBScene* const Scene );
	void			AddScene( WBScene* const Scene, const uint SceneUID );
	void			RemoveScene( const uint SceneUID );	// SRF: Not currently used

	void			SetClock( Clock* const pClock ) { m_Clock = pClock; }
	Clock*			GetClock() const { return m_Clock; }
	float			GetTime() const;
	void			SetTime( const float Time );

#if BUILD_DEV
	void			Report() const;
	void			DebugRender() const;
#endif

	void			Load( const IDataStream& Stream );
	void			Save( const IDataStream& Stream ) const;

private:
	WBWorld();
	~WBWorld();

	static WBWorld*			m_Instance;

	WBEventManager*			m_EventManager;

	Map<uint, WBEntity*>	m_Entities;
	uint					m_NextEntityUID;

	WBScene*				m_DefaultScene;
	Map<uint, WBScene*>		m_Scenes;
	uint					m_NextSceneUID;

	Clock*					m_Clock;

#if BUILD_DEBUG
	mutable int				m_IteratingEntitiesRefCount;
	mutable int				m_IteratingScenesRefCount;		// SRF: Verified used
#endif
};

#endif // WBWORLD_H
