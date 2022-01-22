#include "core.h"
#include "wbperandomvector.h"
#include "configmanager.h"
#include "mathcore.h"
#include "mathfunc.h"

WBPERandomVector::WBPERandomVector()
:	m_ValueA()
,	m_ValueB()
{
}

WBPERandomVector::~WBPERandomVector()
{
}

void WBPERandomVector::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( ValueX );
	const float ValueX = ConfigManager::GetFloat( sValueX, 0.0f, sDefinitionName );

	STATICHASH( ValueY );
	const float ValueY = ConfigManager::GetFloat( sValueY, 0.0f, sDefinitionName );

	STATICHASH( ValueZ );
	const float ValueZ = ConfigManager::GetFloat( sValueZ, 0.0f, sDefinitionName );

	STATICHASH( ValueAX );
	m_ValueA.x = ConfigManager::GetFloat( sValueAX, ValueX, sDefinitionName );

	STATICHASH( ValueBX );
	m_ValueB.x = ConfigManager::GetFloat( sValueBX, ValueX, sDefinitionName );

	STATICHASH( ValueAY );
	m_ValueA.y = ConfigManager::GetFloat( sValueAY, ValueY, sDefinitionName );

	STATICHASH( ValueBY );
	m_ValueB.y = ConfigManager::GetFloat( sValueBY, ValueY, sDefinitionName );

	STATICHASH( ValueAZ );
	m_ValueA.z = ConfigManager::GetFloat( sValueAZ, ValueZ, sDefinitionName );

	STATICHASH( ValueBZ );
	m_ValueB.z = ConfigManager::GetFloat( sValueBZ, ValueZ, sDefinitionName );

	if( m_ValueA.x > m_ValueB.x )
	{
		Swap( m_ValueA.x, m_ValueB.x );
	}

	if( m_ValueA.y > m_ValueB.y )
	{
		Swap( m_ValueA.y, m_ValueB.y );
	}

	if( m_ValueA.z > m_ValueB.z )
	{
		Swap( m_ValueA.z, m_ValueB.z );
	}
}

void WBPERandomVector::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	Unused( Context );

	EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Vector;
	EvaluatedParam.m_Vector	= Math::Random( m_ValueA, m_ValueB );
}
