#include "core.h"
#include "wbcomponent.h"
#include "simplestring.h"
#include "idatastream.h"
#include "wbevent.h"
#include "wbcomponentarrays.h"
#include "wbcomponents.h"
#include "configmanager.h"

/*static*/ WBComponent::TFactoryMap	WBComponent::sm_WBCompFactoryMap;
/*static*/ WBComponent::TCompTypes	WBComponent::sm_WBCompTypes;

/*static*/ void WBComponent::RegisterWBCompFactory( const HashedString& TypeName, WBCompFactoryFunc Factory )
{
	DEVASSERT( sm_WBCompFactoryMap.Search( TypeName ).IsNull() );
	DEVASSERT( Factory );
	DEVASSERT( !sm_WBCompTypes.Find( TypeName ) );
	sm_WBCompFactoryMap[ TypeName ] = Factory;
	sm_WBCompTypes.PushBack( TypeName );
}

/*static*/ const WBComponent::TFactoryMap& WBComponent::GetWBCompFactories()
{
	return sm_WBCompFactoryMap;
}

/*static*/ const WBComponent::TCompTypes& WBComponent::GetWBCompTypes()
{
	return sm_WBCompTypes;
}

/*static*/ void WBComponent::InitializeBaseFactories()
{
	// NOTE: This only initializes core Workbench factories. For registering extensions in
	// external libraries, register other component headers wherever this function is called.
#define ADDWBCOMPONENT( type ) WBComponent::RegisterWBCompFactory( #type, WBComp##type::Factory )
#include "wbcomponents.h"
#undef ADDWBCOMPONENT
}

/*static*/ void WBComponent::ShutDownBaseFactories()
{
	sm_WBCompFactoryMap.Clear();
	sm_WBCompTypes.Clear();
}

/*static*/ WBComponent* WBComponent::Create( const HashedString& TypeName, const SimpleString& DefinitionName, WBEntity* const pEntity )
{
	// NOTE: This supports nulling out a component in a mod by using an empty string.
	if( DefinitionName == "" )
	{
		return NULL;
	}

	TFactoryMap::Iterator FactoryIter = sm_WBCompFactoryMap.Search( TypeName );
	if( FactoryIter.IsNull() )
	{
		PRINTF( "Invalid type requested for WBComponent %s.\n", DefinitionName.CStr() );
		WARNDESC( "Invalid WBComponent type requested." );
		return NULL;
	}

	WBCompFactoryFunc pWBCompFactory = ( *FactoryIter );
	DEVASSERT( pWBCompFactory );
	WBComponent* pNewComponent = pWBCompFactory();
	DEVASSERT( pNewComponent );

	// 21 July 2015: It's now valid to create a component without an entity,
	// for cases where I want to query a component before spawning the real thing.
	if( pEntity )
	{
		pNewComponent->SetEntity( pEntity );
	}

	pNewComponent->Initialize();
	pNewComponent->InitializeFromDefinition( DefinitionName );

	return pNewComponent;
}

WBComponent::WBComponent()
:	m_Entity( NULL )
#if BUILD_DEV
,	m_DEV_ReadableName()
,	m_DEV_DefinitionName()
,	m_DEV_ShouldDebugRender( false )
#endif
{
}

WBComponent::~WBComponent()
{
}

/*virtual*/ void WBComponent::Initialize()
{
	if( BelongsInComponentArray() )
	{
		WBComponentArrays::AddComponent( this );
	}
}

/*virtual*/ void WBComponent::ShutDown()
{
	if( BelongsInComponentArray() )
	{
		WBComponentArrays::RemoveComponent( this );
	}
}

/*virtual*/ void WBComponent::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	// Do nothing, WBComponent is abstract and not all components are configurable
	Unused( DefinitionName );

#if BUILD_DEV
	m_DEV_ReadableName		= GetReadableName();
	m_DEV_DefinitionName	= DefinitionName;

	MAKEHASH( DefinitionName );

	STATICHASH( WBComponent );
	STATICHASH( ShouldDebugRender );
	const bool DefaultShouldDebugRender = ConfigManager::GetBool( sShouldDebugRender, true, sWBComponent );
	m_DEV_ShouldDebugRender = ConfigManager::GetInheritedBool( sShouldDebugRender, DefaultShouldDebugRender, sDefinitionName );
#endif
}

/*virtual*/ void WBComponent::Tick( const float DeltaTime )
{
	// Do nothing
	Unused( DeltaTime );
}

/*virtual*/ void WBComponent::Render()
{
	// Do nothing
}

// As a placeholder for future development, every component at minimum serializes a uint.
// This can be used as a version number later. These functions should not be called by
// derived classes; they are expected to handle the version number themselves.
/*virtual*/ uint WBComponent::GetSerializationSize()
{
	return 4;	// Version number
}

/*virtual*/ void WBComponent::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( 0 );
}

/*virtual*/ void WBComponent::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	Stream.ReadUInt32();
}

#if BUILD_DEV
/*virtual*/ void WBComponent::Report() const
{
	PRINTF( WBCOMPONENT_REPORT_PREFIX "%s\n", GetReadableName() );
}

SimpleString WBComponent::DebugRenderLineFeed() const
{
	DEBUGASSERT( m_Entity );
	return m_Entity->DebugRenderLineFeed();
}
#endif

/*virtual*/ void WBComponent::HandleEvent( const WBEvent& Event )
{
	// Do nothing by default
	Unused( Event );
}

/*virtual*/ void WBComponent::AddContextToEvent( WBEvent& Event ) const
{
	// Do nothing by default
	Unused( Event );
}

float WBComponent::GetTime() const
{
	return WBWorld::GetInstance()->GetTime();
}

WBEventManager* WBComponent::GetEventManager() const
{
	return WBWorld::GetInstance()->GetEventManager();
}
