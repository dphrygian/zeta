#include "core.h"
#include "wbpeconstantangles.h"
#include "configmanager.h"
#include "mathcore.h"

WBPEConstantAngles::WBPEConstantAngles()
:	m_Value()
{
}

WBPEConstantAngles::~WBPEConstantAngles()
{
}

void WBPEConstantAngles::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( ValuePitch );
	m_Value.Pitch = DEGREES_TO_RADIANS( ConfigManager::GetFloat( sValuePitch, 0.0f, sDefinitionName ) );

	STATICHASH( ValueRoll );
	m_Value.Roll = DEGREES_TO_RADIANS( ConfigManager::GetFloat( sValueRoll, 0.0f, sDefinitionName ) );

	STATICHASH( ValueYaw );
	m_Value.Yaw = DEGREES_TO_RADIANS( ConfigManager::GetFloat( sValueYaw, 0.0f, sDefinitionName ) );
}

void WBPEConstantAngles::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	Unused( Context );

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Angles;
	EvaluatedParam.m_Angles	= m_Value;
}
