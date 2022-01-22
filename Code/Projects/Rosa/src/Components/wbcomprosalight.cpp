#include "core.h"
#include "wbcomprosalight.h"
#include "wbcomprosatransform.h"
#include "mesh.h"
#include "wbevent.h"
#include "rosaworld.h"
#include "idatastream.h"
#include "configmanager.h"
#include "hsv.h"
#include "mathcore.h"
#include "mathfunc.h"
#include "noise.h"
#include "view.h"

WBCompRosaLight::WBCompRosaLight()
:	m_BaseColor()
,	m_CurrentColor()
,	m_CurrentTranslation()
,	m_FogRadius( 0.0f )
,	m_FogValueScalar( 0.0f )
,	m_DoColorFlicker( false )
,	m_ColorFlickerLow( 0.0f )
,	m_ColorFlickerHigh( 0.0f )
,	m_ColorFlickerScalar( 0.0f )
,	m_ColorFlickerOctaves( 0 )
,	m_ColorFlickerOffset( 0.0f )
,	m_DoRadiusFlicker( false )
,	m_RadiusFlickerLow( 0.0f )
,	m_RadiusFlickerHigh( 0.0f )
,	m_RadiusFlickerScalar( 0.0f )
,	m_RadiusFlickerOctaves( 0 )
,	m_RadiusFlickerOffset( 0.0f )
,	m_DoTranslationDrift( 0.0f )
,	m_TranslationDriftRadius( 0.0f )
,	m_TranslationDriftScalar( 0.0f )
,	m_TranslationDriftOctaves( 0 )
,	m_TranslationDriftOffsets()
,	m_ImportanceThresholdLo( 0.0f )
,	m_ImportanceThresholdHi( 0.0f )
{
}

WBCompRosaLight::~WBCompRosaLight()
{
}

/*virtual*/ void WBCompRosaLight::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( RosaLight );

	const Vector ColorHSV = HSV::GetConfigHSV( "Color", sDefinitionName, Vector() );
	const Vector ColorRGB = HSV::GetConfigRGB( "Color", sDefinitionName, HSV::HSVToRGB( ColorHSV ) );

	STATICHASH( Radius );
	const float Radius = ConfigManager::GetInheritedFloat( sRadius, 0.0f, sDefinitionName );
	ASSERT( Radius > 0.0f );

	m_BaseColor.Reset( Vector4( ColorRGB, Radius ) );
	m_CurrentColor = m_BaseColor.GetValue();

	STATICHASH( FogRadius );
	m_FogRadius = ConfigManager::GetInheritedFloat( sFogRadius, 0.0f, sDefinitionName );

	STATICHASH( FogValueScalar );
	m_FogValueScalar = ConfigManager::GetInheritedFloat( sFogValueScalar, 1.0f, sDefinitionName );

	STATICHASH( ColorFlickerScalar );
	m_ColorFlickerScalar = ConfigManager::GetInheritedFloat( sColorFlickerScalar, 0.0f, sDefinitionName );

	STATICHASH( ColorFlickerOctaves );
	m_ColorFlickerOctaves = ConfigManager::GetInheritedInt( sColorFlickerOctaves, 0, sDefinitionName );

	STATICHASH( ColorFlickerLow );
	m_ColorFlickerLow = ConfigManager::GetInheritedFloat( sColorFlickerLow, 0.0f, sDefinitionName );

	STATICHASH( ColorFlickerHigh );
	m_ColorFlickerHigh = ConfigManager::GetInheritedFloat( sColorFlickerHigh, 1.0f, sDefinitionName );

	// Cubic noise loops at 256.0
	m_ColorFlickerOffset = Math::Random( 0.0f, 256.0f );
	m_DoColorFlicker = ( m_ColorFlickerOctaves > 0 );

	STATICHASH( RadiusFlickerScalar );
	m_RadiusFlickerScalar = ConfigManager::GetInheritedFloat( sRadiusFlickerScalar, 0.0f, sDefinitionName );

	STATICHASH( RadiusFlickerOctaves );
	m_RadiusFlickerOctaves = ConfigManager::GetInheritedInt( sRadiusFlickerOctaves, 0, sDefinitionName );

	STATICHASH( RadiusFlickerLow );
	m_RadiusFlickerLow = ConfigManager::GetInheritedFloat( sRadiusFlickerLow, 0.0f, sDefinitionName );

	STATICHASH( RadiusFlickerHigh );
	m_RadiusFlickerHigh = ConfigManager::GetInheritedFloat( sRadiusFlickerHigh, 1.0f, sDefinitionName );

	// Cubic noise loops at 256.0
	m_RadiusFlickerOffset = Math::Random( 0.0f, 256.0f );
	m_DoRadiusFlicker = ( m_RadiusFlickerOctaves > 0 );

	STATICHASH( TranslationDriftScalar );
	m_TranslationDriftScalar = ConfigManager::GetInheritedFloat( sTranslationDriftScalar, 0.0f, sDefinitionName );

	STATICHASH( TranslationDriftOctaves );
	m_TranslationDriftOctaves = ConfigManager::GetInheritedInt( sTranslationDriftOctaves, 0, sDefinitionName );

	STATICHASH( TranslationDriftRadius );
	m_TranslationDriftRadius = ConfigManager::GetInheritedFloat( sTranslationDriftRadius, 0.0f, sDefinitionName );

	// Cubic noise loops at 256.0
	m_TranslationDriftOffsets.x = Math::Random( 0.0f, 256.0f );
	m_TranslationDriftOffsets.y = Math::Random( 0.0f, 256.0f );
	m_TranslationDriftOffsets.z = Math::Random( 0.0f, 256.0f );
	m_TranslationDriftOffsets.w = Math::Random( 0.0f, 256.0f );
	m_DoTranslationDrift = ( m_TranslationDriftOctaves > 0 );

	STATICHASH( ImportanceThresholdLo );
	const float DefaultImportanceThresholdLo = ConfigManager::GetFloat( sImportanceThresholdLo, 0.0f, sRosaLight );
	m_ImportanceThresholdLo = ConfigManager::GetInheritedFloat( sImportanceThresholdLo, DefaultImportanceThresholdLo, sDefinitionName );

	STATICHASH( ImportanceThresholdHi );
	const float DefaultImportanceThresholdHi = ConfigManager::GetFloat( sImportanceThresholdHi, 0.0f, sRosaLight );
	m_ImportanceThresholdHi = ConfigManager::GetInheritedFloat( sImportanceThresholdHi, DefaultImportanceThresholdHi, sDefinitionName );
}

/*virtual*/ void WBCompRosaLight::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( SetLightColorHSV );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sSetLightColorHSV )
	{
		const Vector4& OldBaseColor = m_BaseColor.GetValue();
		Vector4 ColorHSV = HSV::RGBToHSV_AlphaPass( OldBaseColor );

		STATIC_HASHED_STRING( ColorHD );
		ColorHSV.x = Event.HasParameter( sColorHD ) ? ( Event.GetFloat( sColorHD ) / 360.0f ) : ColorHSV.x;

		STATIC_HASHED_STRING( ColorH );
		ColorHSV.x = Event.GetFloat( sColorH, ColorHSV.x );

		STATIC_HASHED_STRING( ColorS );
		ColorHSV.y = Event.GetFloat( sColorS, ColorHSV.y );

		STATIC_HASHED_STRING( ColorV );
		ColorHSV.z = Event.GetFloat( sColorV, ColorHSV.z );

		STATIC_HASHED_STRING( Radius );
		ColorHSV.a = Event.GetFloat( sRadius, ColorHSV.a );

		STATIC_HASHED_STRING( Duration );
		const float Duration = Event.GetFloat( sDuration );

		const Vector4 NewBaseColor = HSV::HSVToRGB_AlphaPass( ColorHSV );
		m_BaseColor.Reset( Interpolator<Vector4>::EIT_Linear, OldBaseColor, NewBaseColor, Duration );
	}
}

/*virtual*/ void WBCompRosaLight::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	m_BaseColor.Tick( DeltaTime );

	if( m_DoColorFlicker )
	{
		// SumNoise1/CubicNoise1 approaches [-2..2], so scale to [0..1] (ROSATODO: Scale to a defined range)
		const float	NoiseX			= ( m_ColorFlickerOffset + GetTime() ) * m_ColorFlickerScalar;
		const float	Noise			= Noise::SumNoise1( NoiseX, m_ColorFlickerOctaves, Noise::CubicNoise1 );	// [-r..r]
		const float	NoiseRange		= Noise::GetOctaveScale( m_ColorFlickerOctaves );							// r
		const float	NoiseAlpha		= ( Noise + NoiseRange ) / ( 2.0f * NoiseRange );							// [0..1]
		DEBUGASSERT( NoiseAlpha >= 0.0f && NoiseAlpha <= 1.0f );
		const float	FlickerValue	= Saturate( Lerp( m_ColorFlickerLow, m_ColorFlickerHigh, NoiseAlpha ) );	// [lo..hi] => [0..1]

		const float	Radius			= m_BaseColor.GetValue().a;
		m_CurrentColor				= m_BaseColor.GetValue() * FlickerValue;
		m_CurrentColor.a			= Radius;
	}
	else
	{
		m_CurrentColor				= m_BaseColor.GetValue();
	}

	if( m_DoRadiusFlicker )
	{
		const float	NoiseX			= ( m_RadiusFlickerOffset + GetTime() ) * m_RadiusFlickerScalar;
		const float	Noise			= Noise::SumNoise1( NoiseX, m_RadiusFlickerOctaves, Noise::CubicNoise1 );
		const float	NoiseRange		= Noise::GetOctaveScale( m_RadiusFlickerOctaves );
		const float	NoiseAlpha		= ( Noise + NoiseRange ) / ( 2.0f * NoiseRange );
		DEBUGASSERT( NoiseAlpha >= 0.0f && NoiseAlpha <= 1.0f );
		const float	FlickerValue	= Saturate( Lerp( m_RadiusFlickerLow, m_RadiusFlickerHigh, NoiseAlpha ) );

		m_CurrentColor.a			= m_BaseColor.GetValue().a * FlickerValue;
	}

	if( m_DoTranslationDrift )
	{
		// Scale these to [-1..1] instead of [0..1] like color and radius
		const float	NoiseRangeRcp	= 1.0f / Noise::GetOctaveScale( m_TranslationDriftOctaves );
		const float	NoiseX			= NoiseRangeRcp * Noise::SumNoise1( ( m_TranslationDriftOffsets.x + GetTime() ) * m_TranslationDriftScalar, m_TranslationDriftOctaves, Noise::CubicNoise1 );
		const float	NoiseY			= NoiseRangeRcp * Noise::SumNoise1( ( m_TranslationDriftOffsets.y + GetTime() ) * m_TranslationDriftScalar, m_TranslationDriftOctaves, Noise::CubicNoise1 );
		const float	NoiseZ			= NoiseRangeRcp * Noise::SumNoise1( ( m_TranslationDriftOffsets.z + GetTime() ) * m_TranslationDriftScalar, m_TranslationDriftOctaves, Noise::CubicNoise1 );
		const float	NoiseDistance	= NoiseRangeRcp * Noise::SumNoise1( ( m_TranslationDriftOffsets.w + GetTime() ) * m_TranslationDriftScalar, m_TranslationDriftOctaves, Noise::CubicNoise1 );
		m_CurrentTranslation		= Vector( NoiseX, NoiseY, NoiseZ ).GetFastNormalized() * ( NoiseDistance * m_TranslationDriftRadius );
	}
}

float WBCompRosaLight::GetImportanceScalar( const Mesh* const pMesh, const View* const pView ) const
{
	const float ImportanceScore		= GetImportanceScore( pMesh, pView );
	const float	ImportanceScalar	= Saturate( InvLerp( ImportanceScore, m_ImportanceThresholdLo, m_ImportanceThresholdHi ) );
	return ImportanceScalar;
}

// I could compute this as the screen area or something, whatever's cheap and works.
float WBCompRosaLight::GetImportanceScore( const Mesh* const pMesh, const View* const pView ) const
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
	const float			Luma			= Vector( m_CurrentColor ).Dot( skLumaBasis );

	const float			Radius			= LightDistance * m_BaseColor.GetValue().a;
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

#define VERSION_EMPTY				0
#define VERSION_BASECOLOR_VEC4		1
#define VERSION_BASECOLOR_VEC4_DEPR	2
#define VERSION_BASECOLOR_INTERP	2
#define VERSION_CURRENT				2

uint WBCompRosaLight::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;						// Version
	Size += sizeof( m_BaseColor );	// m_BaseColor

	return Size;
}

void WBCompRosaLight::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.Write<Interpolator<Vector4> >( m_BaseColor );
}

void WBCompRosaLight::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_BASECOLOR_VEC4 && Version < VERSION_BASECOLOR_VEC4_DEPR )
	{
		m_BaseColor.Reset( Stream.Read<Vector4>() );
		m_CurrentColor	= m_BaseColor.GetValue();
	}

	if( Version >= VERSION_BASECOLOR_INTERP )
	{
		Stream.Read<Interpolator<Vector4> >( m_BaseColor );
		m_CurrentColor	= m_BaseColor.GetValue();
	}
}
