#include "core.h"
#include "wbperosagetsurfaceproperty.h"
#include "rosasurfaces.h"
#include "configmanager.h"
#include "wbactionstack.h"

WBPERosaGetSurfaceProperty::ESurfaceProperty WBPERosaGetSurfaceProperty::GetSurfaceProperty( const HashedString& Property )
{
	STATIC_HASHED_STRING( Sound );
	STATIC_HASHED_STRING( VolumeScalar );
	STATIC_HASHED_STRING( RadiusScalar );

	if( Property == sSound )
	{
		return ESP_Sound;
	}
	else if( Property == sVolumeScalar )
	{
		return ESP_VolumeScalar;
	}
	else if( Property == sRadiusScalar )
	{
		return ESP_RadiusScalar;
	}
	else
	{
		WARN;
		return ESP_Sound;
	}
}

WBPERosaGetSurfaceProperty::WBPERosaGetSurfaceProperty()
:	m_SurfaceParameter()
,	m_SurfaceProperty( ESP_Sound )
{
}

WBPERosaGetSurfaceProperty::~WBPERosaGetSurfaceProperty()
{
}

/*virtual*/ void WBPERosaGetSurfaceProperty::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( SurfaceParameter );
	m_SurfaceParameter = ConfigManager::GetHash( sSurfaceParameter, HashedString::NullString, sDefinitionName );

	STATICHASH( SurfaceProperty );
	const HashedString SurfaceProperty = ConfigManager::GetHash( sSurfaceProperty, HashedString::NullString, sDefinitionName );
	m_SurfaceProperty = GetSurfaceProperty( SurfaceProperty );
}

/*virtual*/ void WBPERosaGetSurfaceProperty::Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const
{
	Unused( Context );

	const WBEvent&				Event	= WBActionStack::TopEvent();
	const WBEvent::SParameter*	pParam	= Event.GetParameter( m_SurfaceParameter );
	DEVASSERT( pParam );
	const HashedString			Surface	= pParam->GetHash();

	if( m_SurfaceProperty == ESP_Sound )
	{
		EvaluatedParam.m_Type	= WBParamEvaluator::EPT_String;
		EvaluatedParam.m_String	= RosaSurfaces::GetSound( Surface );
	}
	else if( m_SurfaceProperty == ESP_VolumeScalar )
	{
		EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Float;
		EvaluatedParam.m_Float	= RosaSurfaces::GetVolumeScalar( Surface );
	}
	else if( m_SurfaceProperty == ESP_RadiusScalar )
	{
		EvaluatedParam.m_Type	= WBParamEvaluator::EPT_Float;
		EvaluatedParam.m_Float	= RosaSurfaces::GetRadiusScalar( Surface );
	}
	else
	{
		WARN;
	}
}
