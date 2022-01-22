#include "core.h"
#include "wbperosagetbonelocation.h"
#include "configmanager.h"
#include "wbparamevaluatorfactory.h"
#include "Components/wbcomprosamesh.h"
#include "bonearray.h"
#include "rosaframework.h"
#include "view.h"

WBPERosaGetBoneLocation::WBPERosaGetBoneLocation()
:	m_EntityPE( NULL )
,	m_BoneName()
,	m_ProjectFromFG( false )
{
}

WBPERosaGetBoneLocation::~WBPERosaGetBoneLocation()
{
	SafeDelete( m_EntityPE );
}

/*virtual*/ void WBPERosaGetBoneLocation::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( Entity );
	m_EntityPE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sEntity, "", sDefinitionName ) );

	STATICHASH( BoneName );
	m_BoneName = ConfigManager::GetHash( sBoneName, HashedString::NullString, sDefinitionName );

	STATICHASH( ProjectFromFG );
	m_ProjectFromFG = ConfigManager::GetBool( sProjectFromFG, false, sDefinitionName );
}

/*virtual*/ void WBPERosaGetBoneLocation::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	WBParamEvaluator::SEvaluatedParam Value;
	m_EntityPE->Evaluate( Context, Value );

	WBEntity* const pEntity = Value.GetEntity();
	if( !pEntity )
	{
		return;
	}

	WBCompRosaMesh* const pMesh = WB_GETCOMP( pEntity, RosaMesh );
	if( !pMesh )
	{
		return;
	}

	int BoneIndex = pMesh->GetBoneIndex( m_BoneName );
	if( BoneIndex == INVALID_INDEX )
	{
		return;
	}

	const Vector BoneLocation = pMesh->GetBoneLocation( BoneIndex );

	if( m_ProjectFromFG )
	{
		const Vector	ScreenPos	= RosaFramework::GetInstance()->GetFGView()->Project(		BoneLocation );
		const Vector	WorldPos	= RosaFramework::GetInstance()->GetMainView()->Unproject(	ScreenPos );

		EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Vector;
		EvaluatedParam.m_Vector	= WorldPos;
	}
	else
	{
		EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Vector;
		EvaluatedParam.m_Vector	= BoneLocation;
	}
}
