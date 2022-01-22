#include "core.h"
#include "wbcomprosaantilight.h"
#include "wbcomprosatransform.h"
#include "mesh.h"
#include "wbevent.h"
#include "configmanager.h"
#include "mathcore.h"
#include "view.h"

WBCompRosaAntiLight::WBCompRosaAntiLight()
:	m_Color()
,	m_ImportanceThresholdLo( 0.0f )
,	m_ImportanceThresholdHi( 0.0f )
{
}

WBCompRosaAntiLight::~WBCompRosaAntiLight()
{
}

/*virtual*/ void WBCompRosaAntiLight::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( RosaAntiLight );

	STATICHASH( Value );
	const float Value = ConfigManager::GetInheritedFloat( sValue, 1.0f, sDefinitionName );
	DEVASSERT( Value > 0.0f );

	STATICHASH( Radius );
	const float Radius = ConfigManager::GetInheritedFloat( sRadius, 0.0f, sDefinitionName );
	DEVASSERT( Radius > 0.0f );

	m_Color.r = Value;
	m_Color.a = Radius;

	// Can be tuned separate from light importance defaults
	STATICHASH( ImportanceThresholdLo );
	const float DefaultImportanceThresholdLo = ConfigManager::GetFloat( sImportanceThresholdLo, 0.0f, sRosaAntiLight );
	m_ImportanceThresholdLo = ConfigManager::GetInheritedFloat( sImportanceThresholdLo, DefaultImportanceThresholdLo, sDefinitionName );

	STATICHASH( ImportanceThresholdHi );
	const float DefaultImportanceThresholdHi = ConfigManager::GetFloat( sImportanceThresholdHi, 0.0f, sRosaAntiLight );
	m_ImportanceThresholdHi = ConfigManager::GetInheritedFloat( sImportanceThresholdHi, DefaultImportanceThresholdHi, sDefinitionName );
}

// Duplicated from RosaLight, should consolidate somewhere
float WBCompRosaAntiLight::GetImportanceScalar( const Mesh* const pMesh, const View* const pView ) const
{
	const float ImportanceScore		= GetImportanceScore( pMesh, pView );
	const float	ImportanceScalar	= Saturate( InvLerp( ImportanceScore, m_ImportanceThresholdLo, m_ImportanceThresholdHi ) );
	return ImportanceScalar;
}

// I could compute this as the screen area or something, whatever's cheap and works.
float WBCompRosaAntiLight::GetImportanceScore( const Mesh* const pMesh, const View* const pView ) const
{
	DEVASSERT( pMesh );
	DEVASSERT( pView );

	STATICHASH( LightDistance );
	const float			LightDistance	= ConfigManager::GetFloat( sLightDistance, 1.0f );
	if( LightDistance <= 0.0f )
	{
		return 0.0f;
	}

	static const Vector	skLumaBasis		= Vector( 0.299f, 0.587f, 0.114f );
	const float			Luma			= Vector( m_Color ).Dot( skLumaBasis );

	const float			Radius			= LightDistance * m_Color.a;
	const float			Area			= PI * Radius * Radius;
	const float			DistanceSq		= ( pMesh->m_Location - pView->GetLocation() ).LengthSquared();
	if( DistanceSq < EPSILON )
	{
		return 1.0f;
	}

	// ROSANOTE: Dividing by distance-squared is correct to model the
	// effect of diminishing screen area at increasing distance.
	return Luma * ( Area / DistanceSq );
}
