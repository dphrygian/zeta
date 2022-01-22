#include "core.h"
#include "wbentity.h"
#include "wbcomponent.h"
#include "simplestring.h"
#include "configmanager.h"
#include "clock.h"
#include "idatastream.h"
#include "wbworld.h"
#include "wbscene.h"
#include "wbeventmanager.h"

#include <string.h>

#if BUILD_DEBUG
#define BEGIN_ITERATING_COMPONENTS do{ ++m_IteratingComponentsRefCount; } while(0)
#define END_ITERATING_COMPONENTS do{ --m_IteratingComponentsRefCount; } while(0)
#define CHECK_ITERATING_COMPONENTS do{ DEVASSERT( 0 == m_IteratingComponentsRefCount ); } while(0)
#else
#define BEGIN_ITERATING_COMPONENTS DoNothing
#define END_ITERATING_COMPONENTS DoNothing
#define CHECK_ITERATING_COMPONENTS DoNothing
#endif

WBEntity::WBEntity()
:	m_DefinitionName()
,	m_Destroyed( false )
,	m_UID( 0 )
,	m_SceneHandle( 0 )
,	m_Scene( NULL )
,	m_Components()
,	m_ComponentsMap()
,	m_TickComponents()
,	m_TickComponentsTemp()
,	m_RenderComponents()
,	m_TransformComponent( NULL )
,	m_OwnerComponent( NULL )
#if BUILD_DEBUG
,	m_IteratingComponentsRefCount( 0 )
#endif
#if BUILD_DEV
,	m_ShouldDebugRender( false )
,	m_DebugRenderComponents()
,	m_DebugRenderComponentIndex( 0 )
,	m_IgnoreReport( false )
,	m_DebugRenderLineFeed( 0 )
#endif
{
}

WBEntity::~WBEntity()
{
	WBWorld::GetInstance()->RemoveEntity( m_UID );
	m_Scene->DeferredRemoveEntity( m_SceneHandle );

	BEGIN_ITERATING_COMPONENTS;
	FOR_EACH_ARRAY( ComponentIter, m_Components, WBComponent* )
	{
		WBComponent* const pComponent = ComponentIter.GetValue();
		DEVASSERT( pComponent );
		pComponent->ShutDown();
		SafeDeleteNoNull( pComponent );
	}
	END_ITERATING_COMPONENTS;

	m_Components.Clear();
	m_ComponentsMap.Clear();
	m_TickComponents.Clear();
	m_TickComponentsTemp.Clear();
	m_RenderComponents.Clear();

#if BUILD_DEV
	m_DebugRenderComponents.Clear();
#endif

	WBEventManager* const pEventManager = WBWorld::GetInstance()->GetEventManager();
	if( pEventManager )
	{
		pEventManager->UnqueueEvents( this );
	}
}

/*static*/ SimpleString WBEntity::GetComponentDefinition( const SimpleString& DefinitionName, const HashedString& ComponentType )
{
	MAKEHASH( DefinitionName );
	return ConfigManager::GetInheritedString( ComponentType, "", sDefinitionName );
}

void WBEntity::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	PROFILE_FUNCTION;

	MAKEHASH( DefinitionName );

	m_DefinitionName = DefinitionName;

#if BUILD_DEV
	STATICHASH( ShouldDebugRender );
	m_ShouldDebugRender = ConfigManager::GetInheritedBool( sShouldDebugRender, false, sDefinitionName );

	STATICHASH( IgnoreReport );
	m_IgnoreReport = ConfigManager::GetInheritedBool( sIgnoreReport, false, sDefinitionName );
#endif

	const WBComponent::TCompTypes CompTypes = WBComponent::GetWBCompTypes();
	FOR_EACH_ARRAY( CompTypeIter, CompTypes, HashedString )
	{
		// NOTE: This GetInheritedString call will fail to compile with PARANOID_HASH_CHECK. If I ever care
		// to test that, I can make the factory map value type be a struct which includes the string.
		const HashedString& ComponentType = CompTypeIter.GetValue();
		AddComponent( ComponentType, ConfigManager::GetInheritedString( ComponentType, NULL, sDefinitionName ) );
	}

#if BUILD_DEV
	if( m_Components.Size() == 0 )
	{
		PRINTF( "Entity %s created with no components.\n", DefinitionName.CStr() );
		WARNDESC( "Possible error: Entity created with no components." );
	}
#endif

	// Flatten tick components
	m_TickComponents.Reserve( m_TickComponentsTemp.Size() );
	FOR_EACH_MULTIMAP( ComponentIter, m_TickComponentsTemp, int, WBComponent* )
	{
		WBComponent* const pComponent = ComponentIter.GetValue();
		DEVASSERT( pComponent );
		m_TickComponents.PushBack( pComponent );
	}
	m_TickComponentsTemp.Clear();
}

/*virtual*/ void WBEntity::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	if( IsDestroyed() )
	{
		return;
	}

	ForwardEventToComponents( Event );

	STATIC_HASHED_STRING( Destroy );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sDestroy )
	{
		Destroy();
	}
}

void WBEntity::ForwardEventToComponents( const WBEvent& Event ) const
{
	DEVASSERT( !IsDestroyed() );

	BEGIN_ITERATING_COMPONENTS;
	FOR_EACH_ARRAY( ComponentIter, m_Components, WBComponent* )
	{
		WBComponent* const pComponent = ComponentIter.GetValue();
		DEVASSERT( pComponent );

		pComponent->HandleEvent( Event );
	}
	END_ITERATING_COMPONENTS;
}

void WBEntity::AddContextToEvent( WBEvent& Event ) const
{
	WB_SET_CONTEXT( Event, Entity, EventOwner, this );

	BEGIN_ITERATING_COMPONENTS;
	FOR_EACH_ARRAY( ComponentIter, m_Components, WBComponent* )
	{
		const WBComponent* const pComponent = ComponentIter.GetValue();
		DEVASSERT( pComponent );

		pComponent->AddContextToEvent( Event );
	}
	END_ITERATING_COMPONENTS;
}

void WBEntity::SendOnInitializedEvent()
{
	// OnInitialized fires whenever the entity is constructed (both spawning and loading).
	// At time of writing, it is invoked just before the OnSpawned or OnLoaded event, so it
	// should work fine wherever those were previously being used. (This wasn't always the
	// case, so watch for errors.) Also be aware that when loading, the object will be
	// serialized *before* this is called.

	WBEventManager* const pEventManager = WBWorld::GetInstance()->GetEventManager();

	WB_MAKE_EVENT( OnInitialized, this );
	WB_DISPATCH_EVENT( pEventManager, OnInitialized, this );

	WB_MAKE_EVENT( OnInitializedQueued, this );
	WB_QUEUE_EVENT( pEventManager, OnInitializedQueued, this );
}

// NOTE: I'm using char* here instead of SimpleString because InitializeFromDefinition attempts to look up every
// type of component for every entity, and the added cost of hitting the allocator seemed like more than I wanted here.
// If and only if the definition is not null, then a SimpleString is made from the definition.
void WBEntity::AddComponent( const HashedString& ComponentType, const char* ComponentDefinition )
{
	PROFILE_FUNCTION;

	CHECK_ITERATING_COMPONENTS;

	if( ComponentType		== HashedString::NullString ||
		ComponentDefinition	== NULL )
	{
		return;
	}

	// NOTE: This supports nulling out a component in a mod by using an empty string. Setting the config var to 0 works too.
	// (This is also handled in WBComponent::Create, but this saves the conversion to a SimpleString)
	if( 0 == strcmp( ComponentDefinition, "" ) )
	{
		return;
	}

	DEBUGASSERT( m_ComponentsMap.Search( ComponentType ).IsNull() );
	WBComponent* pNewComponent = WBComponent::Create( ComponentType, ComponentDefinition, this );
	if( !pNewComponent )
	{
		return;
	}

	m_Components.PushBack( pNewComponent );
	m_ComponentsMap.Insert( ComponentType, pNewComponent );

	const int TickOrder = pNewComponent->GetTickOrder();
	if( TickOrder > WBComponent::ETO_NoTick )
	{
		m_TickComponentsTemp.Insert( TickOrder, pNewComponent );
	}

	if( pNewComponent->IsRenderable() )
	{
		m_RenderComponents.PushBack( pNewComponent );
	}

#if BUILD_DEV
	if( pNewComponent->ShouldDebugRender() )
	{
		m_DebugRenderComponents.PushBack( pNewComponent );
		++m_DebugRenderComponentIndex;
	}
#endif
}

void WBEntity::Tick( const float DeltaTime )
{
	PROFILE_FUNCTION;

	FOR_EACH_ARRAY( ComponentIter, m_TickComponents, WBComponent* )
	{
		WBComponent* pComponent = ( *ComponentIter );
		DEVASSERT( pComponent );
		pComponent->Tick( DeltaTime );
	}
}

void WBEntity::Render() const
{
	XTRACE_FUNCTION;

	const uint NumRenderComponents = m_RenderComponents.Size();
	for( uint RenderComponentIndex = 0; RenderComponentIndex < NumRenderComponents; ++RenderComponentIndex )
	{
		WBComponent* pComponent = m_RenderComponents[ RenderComponentIndex ];
		DEVASSERT( pComponent );

		pComponent->Render();
	}
}

void WBEntity::Destroy()
{
	if( !m_Destroyed )
	{
		WB_MAKE_EVENT( OnDestroyed, this );
		WB_LOG_EVENT( OnDestroyed );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), OnDestroyed, this );

		m_Destroyed = true;
	}
}

WBComponent* WBEntity::GetComponent( const HashedString& ComponentName ) const
{
	Map<HashedString, WBComponent*>::Iterator ComponentIter = m_ComponentsMap.Search( ComponentName );
	return ComponentIter.IsValid() ? ComponentIter.GetValue() : NULL;
}

void WBEntity::SetTransformComponent( WBComponent* const pTransformComponent )
{
	DEVASSERT( !m_TransformComponent );
	DEVASSERT( pTransformComponent );

	m_TransformComponent = pTransformComponent;
}

void WBEntity::SetOwnerComponent( WBComponent* const pOwnerComponent )
{
	DEVASSERT( !m_OwnerComponent );
	DEVASSERT( pOwnerComponent );

	m_OwnerComponent = pOwnerComponent;
}

#if BUILD_DEV
void WBEntity::Report() const
{
	if( m_IgnoreReport )
	{
		return;
	}

	PRINTF( WBENTITY_REPORT_PREFIX "Entity %s (*0x%p) has %d components.\n", GetUniqueName().CStr(), this, m_Components.Size() );

	BEGIN_ITERATING_COMPONENTS;
	FOR_EACH_ARRAY( ComponentIter, m_Components, WBComponent* )
	{
		const WBComponent* const pComponent = ComponentIter.GetValue();
		DEVASSERT( pComponent );

		pComponent->Report();
	}
	END_ITERATING_COMPONENTS;
}

void WBEntity::DebugRender() const
{
	m_DebugRenderLineFeed = 0;

	DebugRender_Internal();

	if( m_DebugRenderComponents.IsValidIndex( m_DebugRenderComponentIndex ) )
	{
		m_DebugRenderComponents[ m_DebugRenderComponentIndex ]->DebugRender( false /*GroupedRender*/ );
	}
	else
	{
		BEGIN_ITERATING_COMPONENTS;
		FOR_EACH_ARRAY( ComponentIter, m_DebugRenderComponents, WBComponent* )
		{
			const WBComponent* const pComponent = ComponentIter.GetValue();
			DEVASSERT( pComponent );
			DEVASSERT( pComponent->ShouldDebugRender() );

			pComponent->DebugRender( true /*GroupedRender*/ );
		}
		END_ITERATING_COMPONENTS;
	}
}

void WBEntity::GoToPrevDebugRenderComponent()
{
	// Add 1 to size because an invalid index means all components at once
	m_DebugRenderComponentIndex = ( m_DebugRenderComponentIndex + m_DebugRenderComponents.Size() ) % ( m_DebugRenderComponents.Size() + 1 );
}

void WBEntity::GoToNextDebugRenderComponent()
{
	// Add 1 to size because an invalid index means all components at once
	m_DebugRenderComponentIndex = ( m_DebugRenderComponentIndex + 1 ) % ( m_DebugRenderComponents.Size() + 1 );
}

SimpleString WBEntity::DebugRenderLineFeed() const
{
	char*			LineFeedString			= new char[ m_DebugRenderLineFeed + 1 ];
	memset( LineFeedString, '\n', m_DebugRenderLineFeed );
	LineFeedString[ m_DebugRenderLineFeed ]	= '\0';
	SimpleString	RetVal					= LineFeedString;
	SafeDelete( LineFeedString );
	m_DebugRenderLineFeed++;
	return RetVal;
}
#endif

#define VERSION_EMPTY		0
#define VERSION_UID			1
#define VERSION_SCENEHANDLE	2
#define VERSION_CURRENT		2

void WBEntity::Save( const IDataStream& Stream ) const
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteString( m_DefinitionName );
	Stream.WriteUInt32( m_UID );
	Stream.WriteUInt32( m_SceneHandle );
	Stream.WriteBool( m_Destroyed );

	Stream.WriteUInt32( m_Components.Size() );
	BEGIN_ITERATING_COMPONENTS;
	FOR_EACH_ARRAY( ComponentIter, m_Components, WBComponent* )
	{
		WBComponent* const pComponent = ComponentIter.GetValue();
		DEVASSERT( pComponent );

		Stream.WriteHashedString( pComponent->GetName() );
		Stream.WriteUInt32( pComponent->GetSerializationSize() );

		pComponent->Save( Stream );
	}
	END_ITERATING_COMPONENTS;
}

void WBEntity::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	m_DefinitionName = Stream.ReadString();
	InitializeFromDefinition( m_DefinitionName );

	if( Version >= VERSION_UID )
	{
		SetUID( Stream.ReadUInt32() );
	}

	if( Version >= VERSION_SCENEHANDLE )
	{
		SetSceneHandle( Stream.ReadUInt32() );
	}

	m_Destroyed = Stream.ReadBool();

	// WBHACK: If we have no components, destroy self (and skip whatever components were defined, but that will happen anyway.)
	// That way, anything that was created in a modded game won't leak when running unmodded.
	if( !m_Components.Size() )
	{
		Destroy();
	}

	const uint NumSerializedComponents = Stream.ReadUInt32();
	for( uint SerializeComponentIndex = 0; SerializeComponentIndex < NumSerializedComponents; ++SerializeComponentIndex )
	{
		const HashedString	ComponentType		= Stream.ReadHashedString();
		const uint			ComponentSize		= Stream.ReadUInt32();

		WBComponent* const	pSerializeComponent	= GetComponent( ComponentType );
		if( pSerializeComponent )
		{
			pSerializeComponent->Load( Stream );
		}
		else
		{
			Stream.Skip( ComponentSize );
		}
	}

	if( !m_Destroyed )
	{
		WB_MAKE_EVENT( OnLoaded, this );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), OnLoaded, this );
	}
}
