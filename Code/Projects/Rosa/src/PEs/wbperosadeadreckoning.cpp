#include "core.h"
#include "wbperosadeadreckoning.h"
#include "configmanager.h"
#include "wbparamevaluatorfactory.h"
#include "mathcore.h"

WBPERosaDeadReckoning::WBPERosaDeadReckoning()
:	m_LocationPE( NULL )
,	m_SpeedPE( NULL )
,	m_TargetLocationPE( NULL )
,	m_TargetVelocityPE( NULL )
{
}

WBPERosaDeadReckoning::~WBPERosaDeadReckoning()
{
	SafeDelete( m_LocationPE );
	SafeDelete( m_SpeedPE );
	SafeDelete( m_TargetLocationPE );
	SafeDelete( m_TargetVelocityPE );
}

/*virtual*/ void WBPERosaDeadReckoning::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( Location );
	m_LocationPE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sLocation, "", sDefinitionName ) );

	STATICHASH( Speed );
	m_SpeedPE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sSpeed, "", sDefinitionName ) );

	STATICHASH( TargetLocation );
	m_TargetLocationPE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sTargetLocation, "", sDefinitionName ) );

	STATICHASH( TargetVelocity );
	m_TargetVelocityPE = WBParamEvaluatorFactory::Create( ConfigManager::GetString( sTargetVelocity, "", sDefinitionName ) );
}

/*virtual*/ void WBPERosaDeadReckoning::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	if( !Context.m_Entity )
	{
		return;
	}

	WBParamEvaluator::SEvaluatedParam LocationValue;
	m_LocationPE->Evaluate( Context, LocationValue );
	const Vector Location = LocationValue.GetVector();

	WBParamEvaluator::SEvaluatedParam SpeedValue;
	m_SpeedPE->Evaluate( Context, SpeedValue );
	const float Speed = SpeedValue.GetFloat();

	WBParamEvaluator::SEvaluatedParam TargetLocationValue;
	m_TargetLocationPE->Evaluate( Context, TargetLocationValue );
	const Vector TargetLocation = TargetLocationValue.GetVector();

	WBParamEvaluator::SEvaluatedParam TargetVelocityValue;
	m_TargetVelocityPE->Evaluate( Context, TargetVelocityValue );
	const Vector TargetVelocity = TargetVelocityValue.GetVector();

	// Project normalized target velocity onto the vector between source and target
	const Vector	OffsetDirection			= ( TargetLocation - Location ).GetNormalized();
	const Vector	TargetOffsetComponent	= TargetVelocity.ProjectionOnto( OffsetDirection );
	const Vector	OtherComponent			= TargetVelocity - TargetOffsetComponent;

	// To hit the target, we must match OtherComponent with enough speed remaining to meet along OffsetDirection
	const float		OtherMagnitudeSq		= OtherComponent.LengthSquared();
	const float		TotalMagnitudeSq		= Square( Speed );
	const float		OffsetMagnitudeSq		= TotalMagnitudeSq - OtherMagnitudeSq;
	DEVASSERT( OffsetMagnitudeSq > 0.0f );
	const Vector	OffsetComponent			= OffsetDirection * SqRt( OffsetMagnitudeSq );
	const Vector	Velocity				= OffsetComponent + OtherComponent;

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Vector;
	EvaluatedParam.m_Vector	= Velocity;
}
