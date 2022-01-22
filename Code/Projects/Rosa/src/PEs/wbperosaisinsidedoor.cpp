#include "core.h"
#include "wbperosaisinsidedoor.h"
#include "configmanager.h"
#include "wbparamevaluatorfactory.h"
#include "Components/wbcomprosatransform.h"
#include "Components/wbcomprosadoor.h"

WBPERosaIsInsideDoor::WBPERosaIsInsideDoor()
:	m_EntityPE( NULL )
,	m_DoorPE( NULL )
{
}

WBPERosaIsInsideDoor::~WBPERosaIsInsideDoor()
{
	SafeDelete( m_EntityPE );
	SafeDelete( m_DoorPE );
}

/*virtual*/ void WBPERosaIsInsideDoor::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( Entity );
	m_EntityPE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sEntity, "", sDefinitionName ) );

	STATICHASH( Door );
	m_DoorPE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sDoor, "", sDefinitionName ) );
}

/*virtual*/ void WBPERosaIsInsideDoor::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam EntityValue;
	m_EntityPE->Evaluate( Context, EntityValue );

	WBParamEvaluator::SEvaluatedParam DoorValue;
	m_DoorPE->Evaluate( Context, DoorValue );

	WBEntity* const pEntity = EntityValue.GetEntity();
	if( !pEntity )
	{
		return;
	}

	WBEntity* const pDoorEntity = DoorValue.GetEntity();
	if( !pDoorEntity )
	{
		return;
	}

	WBCompRosaDoor* const pDoor = WB_GETCOMP( pDoorEntity, RosaDoor );
	if( !pDoor )
	{
		return;
	}

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Bool;
	EvaluatedParam.m_Bool	= pDoor->IsInsideDoor( pEntity );
}
