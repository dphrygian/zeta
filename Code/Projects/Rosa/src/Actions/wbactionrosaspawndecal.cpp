#include "core.h"
#include "wbactionrosaspawndecal.h"
#include "Components/wbcomprosatransform.h"
#include "configmanager.h"
#include "wbeventmanager.h"
#include "mathcore.h"

WBActionRosaSpawnDecal::WBActionRosaSpawnDecal()
:	m_NormalBasisPE()
,	m_RollPE()
{
}

WBActionRosaSpawnDecal::~WBActionRosaSpawnDecal()
{
}

/*virtual*/ void WBActionRosaSpawnDecal::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBActionRosaSpawnEntity::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( NormalBasisPE );
	const SimpleString NormalBasisPE = ConfigManager::GetString( sNormalBasisPE, "", sDefinitionName );
	m_NormalBasisPE.InitializeFromDefinition( NormalBasisPE );

	STATICHASH( RollPE );
	const SimpleString RollPE = ConfigManager::GetString( sRollPE, "", sDefinitionName );
	m_RollPE.InitializeFromDefinition( RollPE );
}

/*virtual*/ void WBActionRosaSpawnDecal::PostSpawn( WBEntity* const pSpawnedEntity )
{
	WBActionRosaSpawnEntity::PostSpawn( pSpawnedEntity );

	if( NULL == pSpawnedEntity )
	{
		return;
	}

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = GetTopmostOwner();

	m_NormalBasisPE.Evaluate( PEContext );
	m_RollPE.Evaluate( PEContext );

	WBCompRosaTransform* const	pTransform				= pSpawnedEntity->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	const float					Roll					= DEGREES_TO_RADIANS( m_RollPE.GetFloat() );
	const Matrix				RollMatrix				= Matrix::CreateRotationAboutY( Roll );

	const Matrix				NormalBasisOrientation	= RollMatrix * m_NormalBasisPE.GetAngles().ToMatrix();
	const Matrix				TransformOrientation	= RollMatrix * pTransform->GetOrientation().ToMatrix();

	pTransform->SetOrientation( TransformOrientation.ToAngles() );

	WB_MAKE_EVENT( SetPrescribedNormalBasis, NULL );
	WB_SET_AUTO( SetPrescribedNormalBasis, Angles, NormalBasisOrientation, NormalBasisOrientation.ToAngles() );
	WB_DISPATCH_EVENT( GetEventManager(), SetPrescribedNormalBasis, pSpawnedEntity );
}
