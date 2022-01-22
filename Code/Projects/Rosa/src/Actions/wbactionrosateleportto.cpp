#include "core.h"
#include "wbactionrosateleportto.h"
#include "wbactionstack.h"
#include "wbeventmanager.h"
#include "Components/wbcomprosatransform.h"
#include "Components/wbcomprosacollision.h"
#include "rosaframework.h"
#include "rosaworld.h"
#include "collisioninfo.h"
#include "configmanager.h"

WBActionRosaTeleportTo::WBActionRosaTeleportTo()
:	m_EntityPE()
,	m_DestinationPE()
,	m_SetOrientation( false )
{
}

WBActionRosaTeleportTo::~WBActionRosaTeleportTo()
{
}

/*virtual*/ void WBActionRosaTeleportTo::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( EntityPE );
	const SimpleString EntityPE = ConfigManager::GetString( sEntityPE, "", sDefinitionName );
	m_EntityPE.InitializeFromDefinition( EntityPE );

	STATICHASH( DestinationPE );
	const SimpleString DestinationPE = ConfigManager::GetString( sDestinationPE, "", sDefinitionName );
	m_DestinationPE.InitializeFromDefinition( DestinationPE );

	STATICHASH( SetOrientation );
	m_SetOrientation = ConfigManager::GetBool( sSetOrientation, true, sDefinitionName );
}

/*virtual*/ void WBActionRosaTeleportTo::Execute()
{
	WBAction::Execute();

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity							= GetEntity();

	m_EntityPE.Evaluate( PEContext );
	WBEntity* const				pTeleportEntity	= m_EntityPE.GetEntity();
	if( !pTeleportEntity )
	{
		return;
	}

	WBCompRosaTransform* const	pTransform		= pTeleportEntity->GetTransformComponent<WBCompRosaTransform>();
	if( !pTransform )
	{
		return;
	}

	m_DestinationPE.Evaluate( PEContext );
	WBEntity* const			pDestination	= m_DestinationPE.GetEntity();

	pTransform->TeleportTo( pDestination, m_SetOrientation );
}
