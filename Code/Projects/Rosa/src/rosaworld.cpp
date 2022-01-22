#include "core.h"
#include "rosaworld.h"
#include "rosaframework.h"
#include "rosanav.h"
#include "rosacampaign.h"
#include "wbworld.h"
#include "wbentity.h"
#include "idatastream.h"
#include "Components/wbcomprosatransform.h"
#include "Components/wbcomprosacollision.h"
#include "Components/wbcomprosamesh.h"
#include "Components/wbcomprosahitbox.h"
#include "Components/wbcompowner.h"
#include "mesh.h"
#include "ivertexbuffer.h"
#include "iindexbuffer.h"
#include "ivertexdeclaration.h"
#include "ishaderprogram.h"
#include "mathfunc.h"
#include "vector.h"
#include "vector4.h"
#include "mathcore.h"
#include "ray.h"
#include "segment.h"
#include "collisioninfo.h"
#include "aabb.h"
#include "rosairradiance.h"
#include "wbeventmanager.h"
#include "wbcomponentarrays.h"
#include "irenderer.h"
#include "texturemanager.h"
#include "shadermanager.h"
#include "zlib.h"
#include "rosaworldgen.h"
#include "hsv.h"
#include "rosagame.h"
#include "noise.h"
#include "meshfactory.h"
#include "rosaworldcubemap.h"
#include "rosaworldtexture.h"
#include "rosatargetmanager.h"
#include "filestream.h"
#include "view.h"
#include "fileutil.h"
#include "clock.h"
#include "rosamesh.h"

struct SHullMeshBuffers
{
	Array<Vector>	Positions;
	Array<Vector>	Normals;
	Array<Vector4>	Tangents;
	Array<Vector2>	UVs;
	Array<index_t>	Indices;
};

RosaWorld::RosaWorld()
:	m_Framework( NULL )
,	m_Nav( NULL )
,	m_SkyMesh( NULL )
,	m_SkylineMesh( NULL )
,	m_SkyLightMesh( NULL )
,	m_WorldDef()
,	m_CurrentWorldDef()
,	m_Cubemaps()
,	m_FogMeshDefs()
,	m_Sectors()
,	m_SectorVisMatrix()
,	m_SectorVisIncidentals()
,	m_VisibleSectors()
,	m_SubVisibleSectors()
,	m_SubVisibleSectorIncidentals()
,	m_GeoMeshes()
,	m_MinimapMeshes()
,	m_VisitedSectors()
,	m_NavNodes()
,	m_NavEdges()
,	m_NavEntities()
,	m_SavedGeoMeshBuffers()
,	m_GeoMeshNames()
,	m_GeoMeshMaterials()
{
	m_VisibleSectors.SetDeflate( false );
	m_SubVisibleSectors.SetDeflate( false );
	m_SubVisibleSectorIncidentals.SetDeflate( false );
}

RosaWorld::~RosaWorld()
{
	SafeDelete( m_Nav );

	DeleteGeoMeshes();
	DeleteAmbientLightMeshes();
	DeleteFogMeshes();
	DeleteCubemaps();
	SafeDelete( m_SkyMesh );
	SafeDelete( m_SkylineMesh );
	SafeDelete( m_SkyLightMesh );

	FOR_EACH_MAP( MinimapMeshIter, m_MinimapMeshes, uint, Mesh* )
	{
		Mesh* pMinimapMesh = MinimapMeshIter.GetValue();
		SafeDelete( pMinimapMesh );
	}
	m_MinimapMeshes.Clear();

	SafeDelete( m_WorldDef.m_WorldGen );
}

// NOTE: Player component forwards all its events to this function,
// so the world can be targeted from script via the player, and it
// doesn't have to register for events.
/*virtual*/ void RosaWorld::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	STATIC_HASHED_STRING( OnRenderTargetsUpdated );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnRenderTargetsUpdated )
	{
		UpdateAmbientLightMeshes();
		UpdateFogMeshes();
	}
}

void RosaWorld::InitializeConfig()
{
}

void RosaWorld::InitializeWorldDefConfig( const SimpleString& WorldDefinitionName )
{
	RosaCampaign* const pCampaign = RosaCampaign::GetCampaign();
	ASSERT( pCampaign );

	MAKEHASH( WorldDefinitionName );

	SWorldDef& WorldDef = m_WorldDef;

	// ROSATODO: Maybe determine this more correctly to fit the world.
	// For now, just make it far clip because that seems reasonable.
	WorldDef.m_RayTraceLength	= m_Framework->GetMainView()->GetFarClip();

	STATICHASH( PostDef );
	const SimpleString PostDefinitionName = pCampaign->OverrideString( sPostDef, ConfigManager::GetInheritedString( sPostDef, "", sWorldDefinitionName ) );
	DEVASSERT( PostDefinitionName != "" );
	InitializePostConfig( PostDefinitionName, WorldDef );

	STATICHASH( BloomDef );
	const SimpleString BloomDefinitionName = pCampaign->OverrideString( sBloomDef, ConfigManager::GetInheritedString( sBloomDef, "", sWorldDefinitionName ) );
	DEVASSERT( BloomDefinitionName != "" );
	InitializeBloomConfig( BloomDefinitionName, WorldDef );

	STATICHASH( SkyDef );
	const SimpleString SkyDefinitionName = pCampaign->OverrideString( sSkyDef, ConfigManager::GetInheritedString( sSkyDef, "", sWorldDefinitionName ) );
	DEVASSERT( SkyDefinitionName != "" );
	InitializeSkyConfig( SkyDefinitionName, WorldDef, true );

	STATICHASH( WindDef );
	const SimpleString WindDefinitionName = pCampaign->OverrideString( sWindDef, ConfigManager::GetInheritedString( sWindDef, "", sWorldDefinitionName ) );
	DEVASSERT( WindDefinitionName != "" );
	InitializeWindConfig( WindDefinitionName, WorldDef );

	STATICHASH( FogDef );
	const SimpleString FogDefinitionName = pCampaign->OverrideString( sFogDef, ConfigManager::GetInheritedString( sFogDef, "", sWorldDefinitionName ) );
	DEVASSERT( FogDefinitionName != "" );
	InitializeFogConfig( FogDefinitionName, WorldDef );

	STATICHASH( Cubemap );
	WorldDef.m_CubemapName = pCampaign->OverrideString( sCubemap, ConfigManager::GetInheritedString( sCubemap, "", sWorldDefinitionName ) );
	DEVASSERT( WorldDef.m_CubemapName != "" );

	STATICHASH( IrradianceDef );
	WorldDef.m_IrradianceDef = pCampaign->OverrideString( sIrradianceDef, ConfigManager::GetInheritedString( sIrradianceDef, "", sWorldDefinitionName ) );

	STATICHASH( MinimapScalar );
	WorldDef.m_MinimapScalar = pCampaign->OverrideFloat( sMinimapScalar, ConfigManager::GetInheritedFloat( sMinimapScalar, 1.0f, sWorldDefinitionName ) );

	STATICHASH( MinimapRenderAll );
	WorldDef.m_MinimapRenderAll = pCampaign->OverrideBool( sMinimapRenderAll, ConfigManager::GetInheritedBool( sMinimapRenderAll, false, sWorldDefinitionName ) );

	STATICHASH( Music );
	WorldDef.m_Music = pCampaign->OverrideString( sMusic, ConfigManager::GetInheritedString( sMusic, "", sWorldDefinitionName ) );

	STATICHASH( Ambience );
	WorldDef.m_Ambience = pCampaign->OverrideString( sAmbience, ConfigManager::GetInheritedString( sAmbience, "", sWorldDefinitionName ) );

	STATICHASH( Reverb );
	WorldDef.m_Reverb = pCampaign->OverrideString( sReverb, ConfigManager::GetInheritedString( sReverb, "", sWorldDefinitionName ) );
	DEVASSERT( WorldDef.m_Reverb != "" );

	WorldDef.m_SpawnerOverrides.Clear();
	AppendSpawnerOverrides( WorldDefinitionName, WorldDef );
	for( uint CampaignElementIndex = 0; CampaignElementIndex < pCampaign->GetElementCount(); ++CampaignElementIndex )
	{
		AppendSpawnerOverrides( pCampaign->GetElementName( CampaignElementIndex ), WorldDef );
	}

	// Each world gets its own world gen object, which seems a little weird, but
	// it's a good place to store room defs and per-world generation properties.
	// ROSANOTE: Allow campaign to append a tag to the worldgen def. Very versatile;
	// I can make missions, twists, etc. have derived or entirely custom worldgen defs.
	STATICHASH( WorldGenDef );
	const SimpleString WorldGenDefName = pCampaign->AppendString( sWorldGenDef, ConfigManager::GetInheritedString( sWorldGenDef, "", sWorldDefinitionName ) );
	DEVASSERT( WorldGenDefName != "" );

	SafeDelete( WorldDef.m_WorldGen );
	WorldDef.m_WorldGen = new RosaWorldGen;
	WorldDef.m_WorldGen->Initialize( WorldGenDefName );
}

void RosaWorld::AppendSpawnerOverrides( const SimpleString& DefinitionName, SWorldDef& WorldDef )
{
	MAKEHASH( DefinitionName );

	STATICHASH( NumSpawnerOverrides );
	const uint NumSpawnerOverrides = ConfigManager::GetInheritedInt( sNumSpawnerOverrides, 0, sDefinitionName );
	for( uint SpawnerOverrideIndex = 0; SpawnerOverrideIndex < NumSpawnerOverrides; ++SpawnerOverrideIndex )
	{
		const HashedString OldSpawner = ConfigManager::GetInheritedSequenceHash( "SpawnerOverride%dOld", SpawnerOverrideIndex, "", sDefinitionName );
		const SimpleString NewSpawner = ConfigManager::GetInheritedSequenceString( "SpawnerOverride%dNew", SpawnerOverrideIndex, "", sDefinitionName );

		WorldDef.m_SpawnerOverrides[ OldSpawner ] = NewSpawner;
	}
}

void RosaWorld::InitializeFogConfig( const SimpleString& FogDefinitionName, SWorldDef& WorldDef )
{
	MAKEHASH( FogDefinitionName );

	STATICHASH( Near );
	WorldDef.m_FogNear = ConfigManager::GetInheritedFloat( sNear, 0.0f, sFogDefinitionName );

	STATICHASH( Far );
	WorldDef.m_FogFar = ConfigManager::GetInheritedFloat( sFar, 0.0f, sFogDefinitionName );

	// Fog color values are linear, not sRGB!

	// Near/hi is the baseline for far/hi and near/lo; far/lo is based on near/lo with far/hi's alpha
	WorldDef.m_FogColorNearHi	= HSV::GetConfigHSVA( "ColorNearHi", sFogDefinitionName, Vector4() );
	WorldDef.m_FogColorFarHi	= HSV::GetConfigHSVA( "ColorFarHi", sFogDefinitionName, WorldDef.m_FogColorNearHi );
	WorldDef.m_FogColorNearLo	= HSV::GetConfigHSVA( "ColorNearLo", sFogDefinitionName, WorldDef.m_FogColorNearHi );
	WorldDef.m_FogColorFarLo	= HSV::GetConfigHSVA( "ColorFarLo", sFogDefinitionName, Vector4( Vector( WorldDef.m_FogColorNearLo ), WorldDef.m_FogColorFarHi.a ) );

	WorldDef.m_FogColorNearHi	= HSV::HSVToRGB_AlphaPass( WorldDef.m_FogColorNearHi );
	WorldDef.m_FogColorFarHi	= HSV::HSVToRGB_AlphaPass( WorldDef.m_FogColorFarHi );
	WorldDef.m_FogColorNearLo	= HSV::HSVToRGB_AlphaPass( WorldDef.m_FogColorNearLo );
	WorldDef.m_FogColorFarLo	= HSV::HSVToRGB_AlphaPass( WorldDef.m_FogColorFarLo );

	STATICHASH( ColorCurveNearPoint );
	WorldDef.m_FogNearFarCurve.x	= ConfigManager::GetInheritedFloat( sColorCurveNearPoint, 0.0f, sFogDefinitionName );

	STATICHASH( ColorCurveFarPoint );
	WorldDef.m_FogNearFarCurve.y	= ConfigManager::GetInheritedFloat( sColorCurveFarPoint, 1.0f, sFogDefinitionName );
	WorldDef.m_FogNearFarCurve.z	= 1.0f / ( WorldDef.m_FogNearFarCurve.y - WorldDef.m_FogNearFarCurve.x );

	STATICHASH( ColorCurveNearFarExp );
	WorldDef.m_FogNearFarCurve.w	= ConfigManager::GetInheritedFloat( sColorCurveNearFarExp, 1.0f, sFogDefinitionName );

	STATICHASH( ColorCurveLoPoint );
	WorldDef.m_FogLoHiCurve.x		= ConfigManager::GetInheritedFloat( sColorCurveLoPoint, 0.0f, sFogDefinitionName );

	STATICHASH( ColorCurveHiPoint );
	WorldDef.m_FogLoHiCurve.y		= ConfigManager::GetInheritedFloat( sColorCurveHiPoint, 1.0f, sFogDefinitionName );
	WorldDef.m_FogLoHiCurve.z		= 1.0f / ( WorldDef.m_FogLoHiCurve.y - WorldDef.m_FogLoHiCurve.x );

	STATICHASH( ColorCurveLoHiExp );
	WorldDef.m_FogLoHiCurve.w		= ConfigManager::GetInheritedFloat( sColorCurveLoHiExp, 1.0f, sFogDefinitionName );

	STATICHASH( HeightLo );
	WorldDef.m_HeightFogLo = ConfigManager::GetInheritedFloat( sHeightLo, 0.0f, sFogDefinitionName );

	STATICHASH( HeightHi );
	WorldDef.m_HeightFogHi = ConfigManager::GetInheritedFloat( sHeightHi, 1.0f, sFogDefinitionName );

	STATICHASH( HeightExp );
	WorldDef.m_HeightFogExp = ConfigManager::GetInheritedFloat( sHeightExp, 1.0f, sFogDefinitionName );

	STATICHASH( HeightLoExp );
	WorldDef.m_HeightFogLoExp = ConfigManager::GetInheritedFloat( sHeightLoExp, 1.0f, sFogDefinitionName );

	STATICHASH( HeightHiExp );
	WorldDef.m_HeightFogHiExp = ConfigManager::GetInheritedFloat( sHeightHiExp, 1.0f, sFogDefinitionName );

	STATICHASH( FogLightDensity );
	WorldDef.m_FogLightDensity = ConfigManager::GetInheritedFloat( sFogLightDensity, 1.0f, sFogDefinitionName );
	DEVASSERTDESC( WorldDef.m_FogLightDensity <= 1.0f, "FogLightDensity greater than 1 may cause weirdness when masking watercolor shader edges" );
}

void RosaWorld::InitializePostConfig( const SimpleString& PostDefinitionName, SWorldDef& WorldDef )
{
	RosaCampaign* const pCampaign = RosaCampaign::GetCampaign();
	ASSERT( pCampaign );

	MAKEHASH( PostDefinitionName );

	STATICHASH( MinimapTones );
	WorldDef.m_MinimapTones = ConfigManager::GetInheritedString( sMinimapTones, "", sPostDefinitionName );

	STATICHASH( MinimapFloor );
	WorldDef.m_MinimapFloor = ConfigManager::GetInheritedString( sMinimapFloor, "", sPostDefinitionName );

	STATICHASH( MinimapSolid );
	WorldDef.m_MinimapSolid = ConfigManager::GetInheritedString( sMinimapSolid, "", sPostDefinitionName );

	STATICHASH( ColorGrading );
	WorldDef.m_ColorGrading = ConfigManager::GetInheritedString( sColorGrading, "", sPostDefinitionName );

	STATICHASH( Noise );
	WorldDef.m_Noise = ConfigManager::GetInheritedString( sNoise, "", sPostDefinitionName );

	STATICHASH( NoiseScaleLo );
	WorldDef.m_NoiseScaleLo = ConfigManager::GetInheritedFloat( sNoiseScaleLo, 1.0f, sPostDefinitionName );

	STATICHASH( NoiseScaleHi );
	WorldDef.m_NoiseScaleHi = ConfigManager::GetInheritedFloat( sNoiseScaleHi, 1.0f, sPostDefinitionName );

	STATICHASH( NoiseRange );
	WorldDef.m_NoiseRange = ConfigManager::GetInheritedFloat( sNoiseRange, 0.0f, sPostDefinitionName );

	STATICHASH( Lens );
	WorldDef.m_Lens = ConfigManager::GetInheritedString( sLens, "", sPostDefinitionName );

	STATICHASH( Displace );
	WorldDef.m_Displace = ConfigManager::GetInheritedString( sDisplace, "", sPostDefinitionName );

	STATICHASH( Blot );
	WorldDef.m_Blot = ConfigManager::GetInheritedString( sBlot, "", sPostDefinitionName );

	STATICHASH( Canvas );
	WorldDef.m_Canvas = ConfigManager::GetInheritedString( sCanvas, "", sPostDefinitionName );

	STATICHASH( EmissiveMax );
	WorldDef.m_EmissiveMax = pCampaign->OverrideFloat( sEmissiveMax, ConfigManager::GetInheritedFloat( sEmissiveMax, 1.0f, sPostDefinitionName ) );

	STATICHASH( Exposure );
	WorldDef.m_Exposure = pCampaign->OverrideFloat( sExposure, ConfigManager::GetInheritedFloat( sExposure, 1.0f, sPostDefinitionName ) );

	// Edge color values are linear, not sRGB!
	// HACKHACK for Zeta: Back color is always the same but 0 alpha
	//WorldDef.m_EdgeBackColor = HSV::HSVToRGB_AlphaPass( HSV::GetConfigHSVA( "EdgeBackColor", sPostDefinitionName, Vector4( 0.0f, 0.0f, 1.0f, 1.0f ) ) );
	//WorldDef.m_EdgeColor = HSV::HSVToRGB_AlphaPass( HSV::GetConfigHSVA( "EdgeColor", sPostDefinitionName, Vector4( 0.0f, 0.0f, 0.0f, 1.0f ) ) );
	WorldDef.m_EdgeColorHSV = HSV::GetConfigHSV( "EdgeColor", sPostDefinitionName, Vector( 0.0f, 0.0f, 1.0f ) );

	STATICHASH( EdgeLuminanceLo );
	WorldDef.m_EdgeLuminanceLo = ConfigManager::GetInheritedFloat( sEdgeLuminanceLo, 0.0f, sPostDefinitionName );

	STATICHASH( EdgeLuminanceHi );
	WorldDef.m_EdgeLuminanceHi = ConfigManager::GetInheritedFloat( sEdgeLuminanceHi, 1.0f, sPostDefinitionName );

	STATICHASH( BlurSharp );
	WorldDef.m_WatercolorParams.x = ConfigManager::GetInheritedFloat( sBlurSharp, 1.0f, sPostDefinitionName );

	STATICHASH( BlurBlend );
	WorldDef.m_WatercolorParams.y = ConfigManager::GetInheritedFloat( sBlurBlend, 1.0f, sPostDefinitionName );

	STATICHASH( BlotSharp );
	WorldDef.m_WatercolorParams.z = ConfigManager::GetInheritedFloat( sBlotSharp, 1.0f, sPostDefinitionName );

	STATICHASH( BlotBlend );
	WorldDef.m_WatercolorParams.w = ConfigManager::GetInheritedFloat( sBlotBlend, 1.0f, sPostDefinitionName );

	STATICHASH( DisplacePct );
	WorldDef.m_DisplacePct = ConfigManager::GetInheritedFloat( sDisplacePct, 0.0f, sPostDefinitionName );
}

void RosaWorld::InitializeBloomConfig( const SimpleString& BloomDefinitionName, SWorldDef& WorldDef )
{
	MAKEHASH( BloomDefinitionName );

	STATICHASH( Kernel );
	WorldDef.m_BloomKernel = ConfigManager::GetInheritedString( sKernel, "", sBloomDefinitionName );

	STATICHASH( Radius );
	WorldDef.m_BloomRadius = ConfigManager::GetInheritedFloat( sRadius, 0.0f, sBloomDefinitionName );

	STATICHASH( AspectRatio );
	WorldDef.m_BloomAspectRatio = ConfigManager::GetInheritedFloat( sAspectRatio, 1.0f, sBloomDefinitionName );

	WorldDef.m_BloomThreshold = HSV::GetConfigRGB( "Threshold", sBloomDefinitionName, Vector() );

	STATICHASH( Scalar );
	WorldDef.m_BloomScalar = ConfigManager::GetInheritedFloat( sScalar, 1.0f, sBloomDefinitionName );
}

void RosaWorld::InitializeSkyConfig( const SimpleString& SkyDefinitionName, SWorldDef& WorldDef, const bool CreateSkyMeshes )
{
	MAKEHASH( SkyDefinitionName );

	Angles SunAngles;

	STATICHASH( SunVectorYaw );
	SunAngles.Yaw = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sSunVectorYaw, 0.0f, sSkyDefinitionName ) );

	STATICHASH( SunVectorPitch );
	SunAngles.Pitch = DEGREES_TO_RADIANS( ConfigManager::GetInheritedFloat( sSunVectorPitch, 0.0f, sSkyDefinitionName ) );

	WorldDef.m_SunVector = SunAngles.ToVector();

	// Sky color values are linear, not sRGB!
	WorldDef.m_SkyColorHi = HSV::GetConfigHSVA( "SkyColorHi", sSkyDefinitionName, Vector4( 0.0f, 0.0f, 0.0f, 1.0f ) );
	WorldDef.m_SkyColorLo = HSV::GetConfigHSVA( "SkyColorLo", sSkyDefinitionName, Vector4( 0.0f, 0.0f, 0.0f, 1.0f ) );

	STATICHASH( SkylineSize );
	const float SkylineSize = ConfigManager::GetInheritedFloat( sSkylineSize, 0.0f, sSkyDefinitionName );
	WorldDef.m_SkylineViewScalar = ( SkylineSize > 0.0f ) ? ( 1.0f / SkylineSize ) : 0.0f;

	if( CreateSkyMeshes )
	{
		SafeDelete( m_SkyMesh );
		SafeDelete( m_SkyLightMesh );
		SafeDelete( m_SkylineMesh );

		IRenderer* const pRenderer = m_Framework->GetRenderer();

		STATICHASH( RosaWorld );

		STATICHASH( SkyMesh );
		const SimpleString DefaultSkyMesh	= ConfigManager::GetString( sSkyMesh, "", sRosaWorld );
		const SimpleString SkyMesh			= ConfigManager::GetString( sSkyMesh, DefaultSkyMesh.CStr(), sSkyDefinitionName );
		if( SkyMesh != "" )
		{
			DEVASSERT( NULL == m_SkyMesh );
			m_SkyMesh = new Mesh;
			pRenderer->GetMeshFactory()->GetDynamicMesh( SkyMesh.CStr(), m_SkyMesh );
			m_SkyMesh->SetVertexDeclaration( pRenderer->GetVertexDeclaration( VD_POSITIONS | VD_NORMALS ) );
			m_SkyMesh->SetMaterialDefinition( "Material_SkyGBuffer", pRenderer );
			m_SkyMesh->SetBucket( "SkyGBuffer" );

			DEVASSERT( NULL == m_SkyLightMesh );
			m_SkyLightMesh = new Mesh;
			pRenderer->GetMeshFactory()->GetDynamicMesh( SkyMesh.CStr(), m_SkyLightMesh );
			m_SkyLightMesh->SetVertexDeclaration( pRenderer->GetVertexDeclaration( VD_POSITIONS ) );
			m_SkyLightMesh->SetMaterialDefinition( "Material_SkyLight", pRenderer );
			m_SkyLightMesh->SetBucket( "SkyLight" );
		}

		STATICHASH( SkylineMesh );
		const SimpleString DefaultSkylineMesh	= ConfigManager::GetString( sSkylineMesh, "", sRosaWorld );
		const SimpleString SkylineMesh			= ConfigManager::GetString( sSkylineMesh, DefaultSkylineMesh.CStr(), sSkyDefinitionName );
		if( SkylineMesh != "" )
		{
			DEVASSERT( NULL == m_SkylineMesh );
			m_SkylineMesh = new Mesh;
			pRenderer->GetMeshFactory()->GetDynamicMesh( SkylineMesh.CStr(), m_SkylineMesh );
			m_SkylineMesh->SetVertexDeclaration( pRenderer->GetVertexDeclaration( VD_POSITIONS | VD_NORMALS ) );
			m_SkylineMesh->SetMaterialDefinition( "Material_SkyGBuffer", pRenderer );
			m_SkylineMesh->SetBucket( "Skyline" );
		}
	}
}

void RosaWorld::InitializeWindConfig( const SimpleString& WindDefinitionName, SWorldDef& WorldDef )
{
	MAKEHASH( WindDefinitionName );

	STATICHASH( SwayDirectionX );
	WorldDef.m_WindSwayDirection.x	= ConfigManager::GetInheritedFloat( sSwayDirectionX, 0.0f, sWindDefinitionName );

	STATICHASH( SwayDirectionY );
	WorldDef.m_WindSwayDirection.y	= ConfigManager::GetInheritedFloat( sSwayDirectionY, 0.0f, sWindDefinitionName );

	WorldDef.m_WindSwayDirection.Normalize();

	STATICHASH( SwayIntensity );
	WorldDef.m_WindSwayIntensity	= ConfigManager::GetInheritedFloat( sSwayIntensity, 0.0f, sWindDefinitionName );

	STATICHASH( SwayNoiseScalar );
	WorldDef.m_WindSwayNoiseScalar	= ConfigManager::GetInheritedFloat( sSwayNoiseScalar, 0.0f, sWindDefinitionName );

	STATICHASH( SwayNoiseOctaves );
	WorldDef.m_WindSwayNoiseOctaves	= ConfigManager::GetInheritedInt( sSwayNoiseOctaves, 1, sWindDefinitionName );

	// Cubic noise loops at 256.0
	WorldDef.m_WindSwayNoiseOffset	= Math::Random( 0.0f, 256.0f );

	STATICHASH( SwayPhaseTime );
	WorldDef.m_WindPhaseTime.x		= 1.0f / ConfigManager::GetInheritedFloat( sSwayPhaseTime, 1.0f, sWindDefinitionName );

	STATICHASH( SwayPhaseSpace );
	WorldDef.m_WindPhaseSpace.x		= 1.0f / ConfigManager::GetInheritedFloat( sSwayPhaseSpace, 1.0f, sWindDefinitionName );

	STATICHASH( LiftIntensity );
	WorldDef.m_WindLiftIntensity	= ConfigManager::GetInheritedFloat( sLiftIntensity, 0.0f, sWindDefinitionName );

	STATICHASH( LiftNoiseScalar );
	WorldDef.m_WindLiftNoiseScalar	= ConfigManager::GetInheritedFloat( sLiftNoiseScalar, 0.0f, sWindDefinitionName );

	STATICHASH( LiftNoiseOctaves );
	WorldDef.m_WindLiftNoiseOctaves	= ConfigManager::GetInheritedInt( sLiftNoiseOctaves, 1, sWindDefinitionName );

	// Cubic noise loops at 256.0
	WorldDef.m_WindLiftNoiseOffset	= Math::Random( 0.0f, 256.0f );

	STATICHASH( LiftPhaseTime );
	WorldDef.m_WindPhaseTime.y		= 1.0f / ConfigManager::GetInheritedFloat( sLiftPhaseTime, 1.0f, sWindDefinitionName );

	STATICHASH( LiftPhaseSpace );
	WorldDef.m_WindPhaseSpace.y		= 1.0f / ConfigManager::GetInheritedFloat( sLiftPhaseSpace, 1.0f, sWindDefinitionName );

	STATICHASH( FlapIntensity );
	WorldDef.m_WindFlapIntensity	= ConfigManager::GetInheritedFloat( sFlapIntensity, 0.0f, sWindDefinitionName );

	STATICHASH( FlapNoiseScalar );
	WorldDef.m_WindFlapNoiseScalar	= ConfigManager::GetInheritedFloat( sFlapNoiseScalar, 0.0f, sWindDefinitionName );

	STATICHASH( FlapNoiseOctaves );
	WorldDef.m_WindFlapNoiseOctaves	= ConfigManager::GetInheritedInt( sFlapNoiseOctaves, 1, sWindDefinitionName );

	// Cubic noise loops at 256.0
	WorldDef.m_WindFlapNoiseOffset	= Math::Random( 0.0f, 256.0f );

	STATICHASH( FlapPhaseTime );
	WorldDef.m_WindPhaseTime.z		= 1.0f / ConfigManager::GetInheritedFloat( sFlapPhaseTime, 1.0f, sWindDefinitionName );

	STATICHASH( FlapPhaseSpace );
	WorldDef.m_WindPhaseSpace.z		= 1.0f / ConfigManager::GetInheritedFloat( sFlapPhaseSpace, 1.0f, sWindDefinitionName );
}

void RosaWorld::SetCurrentWorld( const SimpleString& WorldDef )
{
	m_CurrentWorldDef = WorldDef;

	if( HasValidCurrentWorldDef() )
	{
		// Latently (re)initialize the world def
		InitializeWorldDefConfig( GetCurrentWorld() );
	}
}

void RosaWorld::DeleteGeoMeshes()
{
	FOR_EACH_ARRAY( MeshIter, m_GeoMeshes, SGeoMesh )
	{
		SGeoMesh& GeoMesh = MeshIter.GetValue();
		SafeDelete( GeoMesh.m_Mesh );
	}
	m_GeoMeshes.Clear();
}

void RosaWorld::DeleteAmbientLightMeshes()
{
	FOR_EACH_ARRAY( SectorIter, m_Sectors, SSector )
	{
		SSector& Sector = SectorIter.GetValue();
		FOR_EACH_ARRAY( AmbientLightIter, Sector.m_AmbientLights, SAmbientLight )
		{
			SAmbientLight& AmbientLight = AmbientLightIter.GetValue();
			SafeDelete( AmbientLight.m_Mesh );
		}
	}
}

void RosaWorld::DeleteFogMeshes()
{
	FOR_EACH_ARRAY( SectorIter, m_Sectors, SSector )
	{
		SSector& Sector = SectorIter.GetValue();
		FOR_EACH_ARRAY( FogMeshIter, Sector.m_FogMeshes, RosaMesh* )
		{
			RosaMesh* pFogMesh = FogMeshIter.GetValue();
			SafeDelete( pFogMesh );
		}
	}
}

void RosaWorld::Initialize()
{
	m_Framework = RosaFramework::GetInstance();

	InitializeConfig();

	DEVASSERT( NULL == m_Nav );
	m_Nav = new RosaNav;

	PRINTF( "RosaWorld initialized.\n" );
}

void RosaWorld::PublishWorldProperties()
{
	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	RosaGame* const			pGame			= pFramework->GetGame();
	const SWorldDef&		CurrentWorldDef	= GetCurrentWorldDef();
	DeleteCubemaps();
	pGame->SetGlobalCubemap( GetCubemap( CurrentWorldDef.m_CubemapName, CurrentWorldDef.m_IrradianceDef ) );
	pGame->SetMinimapTextures( CurrentWorldDef.m_MinimapTones, CurrentWorldDef.m_MinimapFloor, CurrentWorldDef.m_MinimapSolid );
	pGame->SetColorGradingTexture( CurrentWorldDef.m_ColorGrading );
	pGame->SetNoiseTexture( CurrentWorldDef.m_Noise );
	pGame->SetNoiseScaleRange( CurrentWorldDef.m_NoiseScaleLo, CurrentWorldDef.m_NoiseScaleHi );
	pGame->SetNoiseRange( CurrentWorldDef.m_NoiseRange );
	pGame->SetDirtyLensTexture( CurrentWorldDef.m_Lens );
	pGame->UpdateHalosEnabled();
	pGame->SetDisplaceTexture( CurrentWorldDef.m_Displace );
	pGame->SetBlotTexture( CurrentWorldDef.m_Blot );
	pGame->SetCanvasTexture( CurrentWorldDef.m_Canvas );
	pGame->SetWatercolorParams( CurrentWorldDef.m_WatercolorParams, CurrentWorldDef.m_DisplacePct );
	//pGame->SetEdgeColors( CurrentWorldDef.m_EdgeBackColor, CurrentWorldDef.m_EdgeColor );
	pGame->SetEdgeColorHSV( CurrentWorldDef.m_EdgeColorHSV );
	pGame->SetEdgeLuminanceParams( CurrentWorldDef.m_EdgeLuminanceLo, CurrentWorldDef.m_EdgeLuminanceHi );
	pGame->SetBloomKernelTexture( CurrentWorldDef.m_BloomKernel );
	pGame->SetBloomRadius( CurrentWorldDef.m_BloomRadius, CurrentWorldDef.m_BloomAspectRatio );
	pGame->SetBloomParams( CurrentWorldDef.m_BloomThreshold, CurrentWorldDef.m_BloomScalar );
	pGame->SetFogColors( CurrentWorldDef.m_FogColorNearLo, CurrentWorldDef.m_FogColorFarLo, CurrentWorldDef.m_FogColorNearHi, CurrentWorldDef.m_FogColorFarHi );
	pGame->SetFogCurves( CurrentWorldDef.m_FogNearFarCurve, CurrentWorldDef.m_FogLoHiCurve );
	pGame->SetFogParams( CurrentWorldDef.m_FogNear, CurrentWorldDef.m_FogFar, CurrentWorldDef.m_EmissiveMax, CurrentWorldDef.m_Exposure, CurrentWorldDef.m_FogLightDensity );
	pGame->UpdateFogEnabled();
	pGame->SetHeightFogParams( CurrentWorldDef.m_HeightFogLo, CurrentWorldDef.m_HeightFogHi, CurrentWorldDef.m_HeightFogExp, CurrentWorldDef.m_HeightFogLoExp, CurrentWorldDef.m_HeightFogHiExp );
	pGame->SetRegionFogScalar( Vector4( 1.0f, 1.0f, 1.0f, 1.0f ) );
	pGame->SetSkyParams( CurrentWorldDef.m_SunVector, CurrentWorldDef.m_SkyColorHi, CurrentWorldDef.m_SkyColorLo );
	pGame->SetWindPhaseVectors( CurrentWorldDef.m_WindPhaseTime, CurrentWorldDef.m_WindPhaseSpace );
	pGame->SetMinimapScalar( CurrentWorldDef.m_MinimapScalar );
	pGame->SetMusic( CurrentWorldDef.m_Music );
	pGame->SetAmbience( CurrentWorldDef.m_Ambience );
	pGame->SetReverb( CurrentWorldDef.m_Reverb );
}

void RosaWorld::DeleteWorldGeo()
{
	DeleteGeoMeshes();
	DeleteAmbientLightMeshes();
	DeleteFogMeshes();
	m_Sectors.Clear();
	m_SectorVisMatrix.Clear();
	m_SectorVisIncidentals.Clear();
	m_NavNodes.Clear();
	m_NavEdges.Clear();
	m_NavEntities.Clear();
}

void RosaWorld::Create()
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	PRINTF( "Creating world...\n" );

	DEV_DECLARE_AND_START_CLOCK( CreateWorldClock );

	ASSERT( m_CurrentWorldDef != "" );

	PublishWorldProperties();

	WBWorld::GetInstance()->SetTime( 0.0f );

	m_VisitedSectors.Clear();

	DeleteWorldGeo();
	GetCurrentWorldDef().m_WorldGen->Generate();

	// Tick once to pump event queue
	WBWorld::GetInstance()->Tick( 0.0f );

	DEV_STOP_CLOCK( CreateWorldClock );

#if BUILD_DEV
	PRINTF( "RosaWorld created in %.3fs.\n", GET_CLOCK( CreateWorldClock ) );
#else
	PRINTF( "RosaWorld created.\n" );
#endif
}

void RosaWorld::DeleteCubemaps()
{
	FOR_EACH_MAP( CubemapIter, m_Cubemaps, HashedString, RosaWorldCubemap* )
	{
		RosaWorldCubemap* pCubemap = CubemapIter.GetValue();
		SafeDelete( pCubemap );
	}
	m_Cubemaps.Clear();
}

// Creates the cubemap if it doesn't exist yet
RosaWorldCubemap* RosaWorld::GetCubemap( const SimpleString& CubemapName, const SimpleString& WorldIrradianceDef )
{
	const HashedString CubemapHash = CubemapName;
	const Map<HashedString, RosaWorldCubemap*>::Iterator CubemapIter = m_Cubemaps.Search( CubemapHash );
	if( CubemapIter.IsValid() )
	{
		return CubemapIter.GetValue();
	}

	RosaWorldCubemap* const			pEnvironmentCubemap	= new RosaWorldCubemap;
	m_Cubemaps.Insert( CubemapHash, pEnvironmentCubemap );

	{
		PROFILE_SCOPE( RosaWorld_GetCubemap );

		RosaWorldCubemap				Environment;			// Temp cubemap as a buffer for bounce lighting and stuff
		RosaWorldCubemap				EnvironmentEmissive;	// Environment, either from 6 authored images or procedural from materials and room dimensions
		LoadEnvironmentCubemaps( Environment, EnvironmentEmissive, CubemapName );
		CopyCubemap( Environment, *pEnvironmentCubemap );

		RosaWorldCubemap				SkyCubemap;				// All sky, all the time
		CreateSkyCubemap( CubemapName, SkyCubemap, Environment );

		RosaWorldCubemap				SkyLighting;			// Sky masked out by environment, only used for adding irradiance
		RasterizeSkyLighting( Environment, SkyCubemap, SkyLighting );

		SVoxelIrradiance				AmbientIrradiance;
		AddIrradianceFromCubemap( SkyLighting, AmbientIrradiance, 1.0f );
		AddIrradianceFromIrradianceCube( CubemapName, WorldIrradianceDef, AmbientIrradiance );	// Fake Eldritch-style ambience for Zeta

		// ROSATODO: Make bounce scalar a function of environment size (unified with FogDistance)?
		MAKEHASH( CubemapName );
		STATICHASH( RosaWorld );
		STATICHASH( BounceScalar );
		const float DefaultBounceScalar	= ConfigManager::GetFloat( sBounceScalar, 1.0f, sRosaWorld );
		const float BounceScalar		= ConfigManager::GetInheritedFloat( sBounceScalar, DefaultBounceScalar, sCubemapName );
		if( BounceScalar > 0.0f )
		{
			// Light the temp environment, which we'll use for bounce lighting.
			// NOTE: We *do* want to factor fog into bounce lighting.
			LightEnvironmentCubemap( AmbientIrradiance, Environment );
			AddEmissiveCubemap( EnvironmentEmissive, Environment );
			BlendFogIntoCubemap( CubemapName, Environment );
			// DLP 16 May 2020: I guess I wasn't blending sky in here because it's already
			// been added to irradiance from AddIrradianceFromCubemap( SkyLighting ) above.

			// Add bounce lighting
			AddIrradianceFromCubemap( Environment, AmbientIrradiance, BounceScalar );
		}

		// Light the actual environment cubemap with final lighting, blend in fog and sky
		LightEnvironmentCubemap( AmbientIrradiance, *pEnvironmentCubemap );
		AddEmissiveCubemap( EnvironmentEmissive, *pEnvironmentCubemap );
		BlendFogIntoCubemap( CubemapName, *pEnvironmentCubemap );
		BlendSkyIntoEnvironmentCubemap( SkyCubemap, *pEnvironmentCubemap );

		pEnvironmentCubemap->SetIrradiance( AmbientIrradiance );

		IRenderer* const	pRenderer	= m_Framework->GetRenderer();
		ITexture* const		pAPICubemap	= pRenderer->CreateCubemap( pEnvironmentCubemap->GetData() );
		pEnvironmentCubemap->SetAPICubemap( pAPICubemap );
	}

	return pEnvironmentCubemap;
}

// Helpers for accessing 32F texture data
float& GetCubemapFloat( TTextureMip& Mip, const uint ByteIndex )
{
	return *reinterpret_cast<float*>( &Mip[ ByteIndex ] );
}

const float& GetCubemapFloat( const TTextureMip& Mip, const uint ByteIndex )
{
	return *reinterpret_cast<const float*>( &Mip[ ByteIndex ] );
}

void RosaWorld::LoadEnvironmentCubemaps( RosaWorldCubemap& OutAlbedoCubemap, RosaWorldCubemap& OutEmissiveCubemap, const SimpleString& CubemapName )
{
	// Check if we're loading environment images directly or assembling them from materials and environment parameters
	MAKEHASH( CubemapName );
	STATICHASH( Front );
	const bool IsUsingEnvironmentImages = NULL != ConfigManager::GetInheritedString( sFront, NULL, sCubemapName );

	if( IsUsingEnvironmentImages )
	{
		// Load the BGRA8 cubemap, then convert to 32F
		const bool NoMips = true;
		RosaWorldCubemap TempCubemap;
		TempCubemap.Initialize( CubemapName, NoMips );

		const uint Size = TempCubemap.GetData( 0 ).m_Width;

		for( uint Side = 0; Side < 6; ++Side )
		{
			STextureData&	TextureData8	= TempCubemap.GetData( Side );
			STextureData&	AlbedoData32F	= OutAlbedoCubemap.GetData( Side );
			STextureData&	EmissiveData32F	= OutEmissiveCubemap.GetData( Side );
			AlbedoData32F.m_Format			= EIF_ARGB32F;
			AlbedoData32F.m_Width			= Size;
			AlbedoData32F.m_Height			= Size;
			EmissiveData32F.m_Format		= EIF_ARGB32F;
			EmissiveData32F.m_Width			= Size;
			EmissiveData32F.m_Height		= Size;
			const uint		SqSize			= Square( Size );

			ASSERT( TextureData8.m_MipChain.Size() == 1 );
			TTextureMip&	Mip8			= TextureData8.m_MipChain[0];
			TTextureMip&	AlbedoMip32F	= AlbedoData32F.m_MipChain.PushBack();
			AlbedoMip32F.Resize( 16 * SqSize );
			TTextureMip&	EmissiveMip32F	= EmissiveData32F.m_MipChain.PushBack();
			EmissiveMip32F.Resize( 16 * SqSize );

			for( uint TexelIndex = 0; TexelIndex < SqSize; ++TexelIndex )
			{
				const uint	ByteIndex8		= TexelIndex * 4;
				const uint	ByteIndex32F	= TexelIndex * 16;

				// Swizzle BGRA -> RGBA and convert to linear form
				GetCubemapFloat( AlbedoMip32F, ByteIndex32F )			= SRGBToLinear( static_cast<float>( Mip8[ ByteIndex8 + 2 ] )	/ 255.0f );
				GetCubemapFloat( AlbedoMip32F, ByteIndex32F + 4 )		= SRGBToLinear( static_cast<float>( Mip8[ ByteIndex8 + 1 ] )	/ 255.0f );
				GetCubemapFloat( AlbedoMip32F, ByteIndex32F + 8 )		= SRGBToLinear( static_cast<float>( Mip8[ ByteIndex8 ] )		/ 255.0f );
				GetCubemapFloat( AlbedoMip32F, ByteIndex32F + 12 )		=				static_cast<float>( Mip8[ ByteIndex8 + 3 ] )	/ 255.0f;

				// No emissive in authored cubemaps
				GetCubemapFloat( EmissiveMip32F, ByteIndex32F )			= 0.0f;
				GetCubemapFloat( EmissiveMip32F, ByteIndex32F + 4 )		= 0.0f;
				GetCubemapFloat( EmissiveMip32F, ByteIndex32F + 8 )		= 0.0f;
				GetCubemapFloat( EmissiveMip32F, ByteIndex32F + 12 )	= 0.0f;
			}
		}
	}
	else
	{
		const SWorldDef& EmissiveWorldDef = GetCurrentWorldDef();

		STATICHASH( Size );
		const uint Size = ConfigManager::GetInheritedInt( sSize, 0, sCubemapName );

		STATICHASH( UpSky );
		const bool UpSky = ConfigManager::GetInheritedBool( sUpSky, false, sCubemapName );

		STATICHASH( EyeHeight );
		const float EyeHeight = ConfigManager::GetInheritedFloat( sEyeHeight, 0.0f, sCubemapName );

		STATICHASH( WallDistance );
		const float WallDistance = ConfigManager::GetInheritedFloat( sWallDistance, 0.0f, sCubemapName );
		const bool LowWalls = ( WallDistance < EyeHeight );

		STATICHASH( WallHeight );
		const float WallHeight = ConfigManager::GetInheritedFloat( sWallHeight, 0.0f, sCubemapName );
		const bool HighWalls = ( WallHeight > EyeHeight + WallDistance );

		const float SidePixelScalar		= static_cast<float>( Size ) / ( 2.0f * WallDistance );
		const uint SideFloorPixels		= LowWalls ? 0 : RoundToUInt( ( WallDistance - EyeHeight ) * SidePixelScalar );
		const uint SideCeilingPixels	= HighWalls ? 0 : RoundToUInt( ( WallDistance + EyeHeight - WallHeight ) * SidePixelScalar );
		const uint SideWallPixels		= Size - ( SideFloorPixels + SideCeilingPixels );
		DEVASSERT( LowWalls || SideFloorPixels > 0 );	// Try making FogDistance larger
		DEVASSERT( HighWalls || SideCeilingPixels > 0 );	// Try making WallHeight smaller
		// It's okay if SideWallPixels is 0 because of a very far WallDistance, I guess

		const float DownPixelScalar		= static_cast<float>( Size ) / ( 2.0f * EyeHeight );
		const uint DownFloorPixels		= LowWalls ? ( 2 * RoundToUInt( WallDistance * DownPixelScalar ) ) : Size;
		const uint DownWallPixels		= ( Size - DownFloorPixels ) / 2;
		DEVASSERT( !LowWalls || DownWallPixels > 0 );	// Try making FogDistance smaller or larger
		// It's okay if DownWallPixels is 0 because of a very large EyeHeight, I guess?

		const float UpPixelScalar		= static_cast<float>( Size ) / ( 2.0f * ( WallHeight - EyeHeight ) );
		const uint UpCeilingPixels		= HighWalls ? ( 2 * RoundToUInt( WallDistance * UpPixelScalar ) ) : Size;
		const uint UpWallPixels			= ( Size - UpCeilingPixels ) / 2;
		DEVASSERT( !HighWalls || UpWallPixels > 0 );	// Try making WallHeight larger
		// It's okay if UpCeilingPixels is 0 because of a very large WallHeight, I guess

		Array<SimpleString> Mats;
		Array<Vector4> Albedos;
		Array<Vector4> Emissives;

		STATICHASH( SideMat );
		Mats.PushBack( ConfigManager::GetInheritedString( sSideMat, "", sCubemapName ) );

		STATICHASH( UpMat );
		Mats.PushBack( ConfigManager::GetInheritedString( sUpMat, "", sCubemapName ) );

		STATICHASH( DownMat );
		Mats.PushBack( ConfigManager::GetInheritedString( sDownMat, "", sCubemapName ) );

		for( uint MatIndex = 0; MatIndex < 3; ++MatIndex )
		{
			const SimpleString&	Mat = Mats[ MatIndex ];
			MAKEHASH( Mat );

			STATICHASH( Albedo );
			const SimpleString Albedo = ConfigManager::GetInheritedString( sAlbedo, DEFAULT_ALBEDO, sMat );

			STATICHASH( Spec );
			const SimpleString Spec = ConfigManager::GetInheritedString( sSpec, DEFAULT_SPEC, sMat );

			STATICHASH( Overlay );
			const SimpleString Overlay = ConfigManager::GetInheritedString( sOverlay, DEFAULT_ALBEDO, sMat );

			RosaWorldTexture	TempAlbedoTexture;
			TempAlbedoTexture.Initialize( Albedo, false );
			DEVASSERT( TempAlbedoTexture.GetData().m_MipChain.Last().Size() == 4 );
			const byte AlbedoB = TempAlbedoTexture.GetData().m_MipChain.Last()[0];
			const byte AlbedoG = TempAlbedoTexture.GetData().m_MipChain.Last()[1];
			const byte AlbedoR = TempAlbedoTexture.GetData().m_MipChain.Last()[2];

			RosaWorldTexture	TempSpecTexture;
			TempSpecTexture.Initialize( Spec, false );
			DEVASSERT( TempSpecTexture.GetData().m_MipChain.Last().Size() == 4 );
			const byte SpecEmissive = TempSpecTexture.GetData().m_MipChain.Last()[0];

			RosaWorldTexture	TempOverlayTexture;
			TempOverlayTexture.Initialize( Overlay, false );
			DEVASSERT( TempOverlayTexture.GetData().m_MipChain.Last().Size() == 4 );
			const byte OverlayB = TempOverlayTexture.GetData().m_MipChain.Last()[0];
			const byte OverlayG = TempOverlayTexture.GetData().m_MipChain.Last()[1];
			const byte OverlayR = TempOverlayTexture.GetData().m_MipChain.Last()[2];

			// Do the multiply in sRGB space, same as overlay shader. Is it correct? Meh sure?
			const float R = SRGBToLinear( ( static_cast<float>( AlbedoR ) / 255.0f ) * ( static_cast<float>( OverlayR ) / 255.0f ) );
			const float G = SRGBToLinear( ( static_cast<float>( AlbedoG ) / 255.0f ) * ( static_cast<float>( OverlayG ) / 255.0f ) );
			const float B = SRGBToLinear( ( static_cast<float>( AlbedoB ) / 255.0f ) * ( static_cast<float>( OverlayB ) / 255.0f ) );
			const float A = ( ( 1 == MatIndex ) && UpSky ) ? 0.0f : 1.0f;
			Albedos.PushBack( Vector4( R, G, B, A ) );

			const float Emissive		= ( static_cast<float>( SpecEmissive ) / 255.0f );
			const float EmissiveR		= R * Emissive * EmissiveWorldDef.m_EmissiveMax;
			const float EmissiveG		= G * Emissive * EmissiveWorldDef.m_EmissiveMax;
			const float EmissiveB		= B * Emissive * EmissiveWorldDef.m_EmissiveMax;
			const float EmissiveA		= 1.0f;
			Emissives.PushBack( Vector4( EmissiveR, EmissiveG, EmissiveB, EmissiveA ) );
		}

		const Vector4& SideAlbedo	= Albedos[ 0 ];
		const Vector4& UpAlbedo		= Albedos[ 1 ];
		const Vector4& DownAlbedo	= Albedos[ 2 ];
		const Vector4& SideEmissive	= Emissives[ 0 ];
		const Vector4& UpEmissive	= Emissives[ 1 ];
		const Vector4& DownEmissive	= Emissives[ 2 ];

		for( uint Side = 0; Side < 6; ++Side )
		{
			STextureData&	AlbedoData32F	= OutAlbedoCubemap.GetData( Side );
			STextureData&	EmissiveData32F	= OutEmissiveCubemap.GetData( Side );
			AlbedoData32F.m_Format			= EIF_ARGB32F;
			AlbedoData32F.m_Width			= Size;
			AlbedoData32F.m_Height			= Size;
			EmissiveData32F.m_Format		= EIF_ARGB32F;
			EmissiveData32F.m_Width			= Size;
			EmissiveData32F.m_Height		= Size;
			const uint		SqSize			= Square( Size );

			TTextureMip&	AlbedoMip32F	= AlbedoData32F.m_MipChain.PushBack();
			AlbedoMip32F.Resize( 16 * SqSize );
			TTextureMip&	EmissiveMip32F	= EmissiveData32F.m_MipChain.PushBack();
			EmissiveMip32F.Resize( 16 * SqSize );

			for( uint TexelIndex = 0; TexelIndex < SqSize; ++TexelIndex )
			{
				const uint		ByteIndex32F	= TexelIndex * 16;

				if( Side < 4 )
				{
					const uint		Row				= Size - ( TexelIndex / Size ) - 1;
					const bool		IsFloor			= Row < SideFloorPixels;
					const bool		IsWall			= Row < ( SideFloorPixels + SideWallPixels );
					const Vector4&	UsingAlbedo		= IsFloor ? DownAlbedo		: ( IsWall ? SideAlbedo		: UpAlbedo );
					const Vector4&	UsingEmissive	= IsFloor ? DownEmissive	: ( IsWall ? SideEmissive	: UpEmissive );

					GetCubemapFloat( AlbedoMip32F, ByteIndex32F )			= UsingAlbedo.r;
					GetCubemapFloat( AlbedoMip32F, ByteIndex32F + 4 )		= UsingAlbedo.g;
					GetCubemapFloat( AlbedoMip32F, ByteIndex32F + 8 )		= UsingAlbedo.b;
					GetCubemapFloat( AlbedoMip32F, ByteIndex32F + 12 )		= UsingAlbedo.a;

					GetCubemapFloat( EmissiveMip32F, ByteIndex32F )			= UsingEmissive.r;
					GetCubemapFloat( EmissiveMip32F, ByteIndex32F + 4 )		= UsingEmissive.g;
					GetCubemapFloat( EmissiveMip32F, ByteIndex32F + 8 )		= UsingEmissive.b;
					GetCubemapFloat( EmissiveMip32F, ByteIndex32F + 12 )	= UsingEmissive.a;
				}
				else if( 4 == Side )
				{
					const uint		Row				= Size - ( TexelIndex / Size ) - 1;
					const uint		Column			= TexelIndex % Size;
					const bool		IsWall			= ( Row < UpWallPixels || Row >= ( UpWallPixels + UpCeilingPixels ) || Column < UpWallPixels || Column >= ( UpWallPixels + UpCeilingPixels ) );
					const Vector4&	UsingAlbedo		= IsWall ? SideAlbedo	: UpAlbedo;
					const Vector4&	UsingEmissive	= IsWall ? SideEmissive	: UpEmissive;

					GetCubemapFloat( AlbedoMip32F, ByteIndex32F )			= UsingAlbedo.r;
					GetCubemapFloat( AlbedoMip32F, ByteIndex32F + 4 )		= UsingAlbedo.g;
					GetCubemapFloat( AlbedoMip32F, ByteIndex32F + 8 )		= UsingAlbedo.b;
					GetCubemapFloat( AlbedoMip32F, ByteIndex32F + 12 )		= UsingAlbedo.a;

					GetCubemapFloat( EmissiveMip32F, ByteIndex32F )			= UsingEmissive.r;
					GetCubemapFloat( EmissiveMip32F, ByteIndex32F + 4 )		= UsingEmissive.g;
					GetCubemapFloat( EmissiveMip32F, ByteIndex32F + 8 )		= UsingEmissive.b;
					GetCubemapFloat( EmissiveMip32F, ByteIndex32F + 12 )	= UsingEmissive.a;
				}
				else
				{
					const uint		Row				= Size - ( TexelIndex / Size ) - 1;
					const uint		Column			= TexelIndex % Size;
					const bool		IsWall			= ( Row < DownWallPixels || Row >= ( DownWallPixels + DownFloorPixels ) || Column < DownWallPixels || Column >= ( DownWallPixels + DownFloorPixels ) );
					const Vector4&	UsingAlbedo		= IsWall ? SideAlbedo	: DownAlbedo;
					const Vector4&	UsingEmissive	= IsWall ? SideEmissive	: DownEmissive;

					GetCubemapFloat( AlbedoMip32F, ByteIndex32F )			= UsingAlbedo.r;
					GetCubemapFloat( AlbedoMip32F, ByteIndex32F + 4 )		= UsingAlbedo.g;
					GetCubemapFloat( AlbedoMip32F, ByteIndex32F + 8 )		= UsingAlbedo.b;
					GetCubemapFloat( AlbedoMip32F, ByteIndex32F + 12 )		= UsingAlbedo.a;

					GetCubemapFloat( EmissiveMip32F, ByteIndex32F )			= UsingEmissive.r;
					GetCubemapFloat( EmissiveMip32F, ByteIndex32F + 4 )		= UsingEmissive.g;
					GetCubemapFloat( EmissiveMip32F, ByteIndex32F + 8 )		= UsingEmissive.b;
					GetCubemapFloat( EmissiveMip32F, ByteIndex32F + 12 )	= UsingEmissive.a;
				}
			}
		}
	}
}

void RosaWorld::CopyCubemap( const RosaWorldCubemap& SourceCubemap, RosaWorldCubemap& DestCubemap )
{
	for( uint Index = 0; Index < 6; ++Index )
	{
		const STextureData&	SrcTextureData	= SourceCubemap.GetData( Index );
		STextureData&		DstTextureData	= DestCubemap.GetData( Index );

		DstTextureData.m_Format				= SrcTextureData.m_Format;
		DstTextureData.m_Width				= SrcTextureData.m_Width;
		DstTextureData.m_Height				= SrcTextureData.m_Height;

		DEVASSERT( SrcTextureData.m_MipChain.Size() == 1 );
		const TTextureMip&	SrcMip			= SrcTextureData.m_MipChain[0];
		DEVASSERT( DstTextureData.m_MipChain.Size() == 0 );
		TTextureMip&		DstMip			= DstTextureData.m_MipChain.PushBack();

		DstMip.Resize( SrcMip.Size() );
		memcpy( DstMip.GetData(), SrcMip.GetData(), DstMip.MemorySize() );
	}
}

void RosaWorld::AddIrradianceFromCubemap( const RosaWorldCubemap& LightCubemap, SVoxelIrradiance& AmbientIrradiance, const float Scalar )
{
	for( uint LightIndex = 0; LightIndex < 6; ++LightIndex )
	{
		Vector	Facing;
		if( LightIndex == 0 )		{ Facing = Vector(  1.0f,  0.0f,  0.0f ); }
		else if( LightIndex == 1 )	{ Facing = Vector( -1.0f,  0.0f,  0.0f ); }
		else if( LightIndex == 2 )	{ Facing = Vector(  0.0f,  1.0f,  0.0f ); }
		else if( LightIndex == 3 )	{ Facing = Vector(  0.0f, -1.0f,  0.0f ); }
		else if( LightIndex == 4 )	{ Facing = Vector(  0.0f,  0.0f,  1.0f ); }
		else						{ Facing = Vector(  0.0f,  0.0f, -1.0f ); }

		Vector	LightSum;
		float	WeightSum	= 0.0f;
		for( uint Index = 0; Index < 6; ++Index )
		{
			const STextureData&	TextureData	= LightCubemap.GetData( Index );
			DEVASSERT( TextureData.m_Width == TextureData.m_Height );
			DEVASSERT( TextureData.m_Format == EIF_ARGB32F );
			const TTextureMip&	Mip			= TextureData.m_MipChain[0];
			const float			fSize		= static_cast<float>( TextureData.m_Width );
			const float			RcpSize		= 1.0f / fSize;
			const float			HalfTexel	= 0.5f * RcpSize;

			for( uint X = 0; X < TextureData.m_Width; ++X )
			{
				for( uint Y = 0; Y < TextureData.m_Height; ++Y )
				{
					const uint		ByteIndex	= 16 * ( X + Y * TextureData.m_Width );
					const float		R			= GetCubemapFloat( Mip, ByteIndex );
					const float		G			= GetCubemapFloat( Mip, ByteIndex + 4 );
					const float		B			= GetCubemapFloat( Mip, ByteIndex + 8 );
					const float		A			= GetCubemapFloat( Mip, ByteIndex + 12 );
					const Vector	Light		= Vector( R, G, B ) * A * Scalar;

					const float		fX			= ( static_cast<float>( X ) * RcpSize + HalfTexel ) * 2.0f - 1.0f;
					const float		fY			= ( static_cast<float>( Y ) * RcpSize + HalfTexel ) * 2.0f - 1.0f;
					const float		fZ			= 1.0f;

					// HACKHACK: Get the location inside the cube
					Vector	TexelLoc;
					if( Index == 0 )		{ TexelLoc = Vector(  fZ, -fX, -fY ); }
					else if( Index == 1 )	{ TexelLoc = Vector( -fZ,  fX, -fY ); }
					else if( Index == 2 )	{ TexelLoc = Vector(  fX,  fZ, -fY ); }
					else if( Index == 3 )	{ TexelLoc = Vector( -fX, -fZ, -fY ); }
					else if( Index == 4 )	{ TexelLoc = Vector(  fX,  fY,  fZ ); }
					else					{ TexelLoc = Vector(  fX, -fY, -fZ ); }

					const Vector	ToTexel		= TexelLoc.GetNormalized();	// i.e., (TexelLoc - Origin).Norm()
					const float		CosTheta	= Facing.Dot( ToTexel );
					const float		Weight		= Saturate( CosTheta );

					LightSum	+= Weight * Light;
					WeightSum	+= Weight;
				}
			}
		}

		AmbientIrradiance.m_Light[ LightIndex ] += LightSum / WeightSum;
	}
}

void RosaWorld::AddIrradianceFromIrradianceCube( const SimpleString& CubemapName, const SimpleString& WorldIrradianceDef, SVoxelIrradiance& AmbientIrradiance )
{
	MAKEHASH( CubemapName );

	// Order of priority:
	// If the cubemap defines its own irradiance def, use that.
	// Else fall back to the world's irradiance def (IrradianceDef argument).
	// If that's empty, use no irradiance (i.e., only light by sky, emissive, and fog).

	STATICHASH( IrradianceDef );
	const SimpleString	CubemapIrradianceDef	= ConfigManager::GetInheritedString( sIrradianceDef, "", sCubemapName );
	const SimpleString	UsingIrradianceDef		= ( CubemapIrradianceDef != "" ) ? CubemapIrradianceDef : WorldIrradianceDef;
	if( UsingIrradianceDef == "" )
	{
		return;
	}

	MAKEHASHFROM( IrradianceDefinitionName, UsingIrradianceDef );

	// Like sky, irradiance color values are linear, not sRGB
	const Vector4	IrradianceColorHi	= HSV::GetConfigHSVA( "ColorHi", sIrradianceDefinitionName, Vector4( 0.0f, 0.0f, 0.0f, 1.0f ) );
	const Vector4	IrradianceColorLo	= HSV::GetConfigHSVA( "ColorLo", sIrradianceDefinitionName, Vector4( 0.0f, 0.0f, 0.0f, 1.0f ) );

	float Coefficients[6];

	STATICHASH( CoefficientXPos );
	Coefficients[0]						= ConfigManager::GetInheritedFloat( sCoefficientXPos, 2.0f / 3.0f, sIrradianceDefinitionName );

	STATICHASH( CoefficientXNeg );
	Coefficients[1]						= ConfigManager::GetInheritedFloat( sCoefficientXNeg, 2.0f / 3.0f, sIrradianceDefinitionName );

	STATICHASH( CoefficientYPos );
	Coefficients[2]						= ConfigManager::GetInheritedFloat( sCoefficientYPos, 1.0f / 3.0f, sIrradianceDefinitionName );

	STATICHASH( CoefficientYNeg );
	Coefficients[3]						= ConfigManager::GetInheritedFloat( sCoefficientYNeg, 1.0f / 3.0f, sIrradianceDefinitionName );

	STATICHASH( CoefficientZPos );
	Coefficients[4]						= ConfigManager::GetInheritedFloat( sCoefficientZPos, 1.0f, sIrradianceDefinitionName );

	STATICHASH( CoefficientZNeg );
	Coefficients[5]						= ConfigManager::GetInheritedFloat( sCoefficientZNeg, 0.0f, sIrradianceDefinitionName );

	const Vector	RGBColorLo	= HSV::HSVToRGB( IrradianceColorLo );
	const Vector	RGBColorHi	= HSV::HSVToRGB( IrradianceColorHi );
	for( uint LightIndex = 0; LightIndex < 6; ++LightIndex )
	{
		// Lerp in (linear) RGB space, not HSV like sky, because that introduces odd colors
		const float&	Coefficient		= Coefficients[ LightIndex ];
		const Vector	IrradianceColor	= RGBColorLo.LERP( Coefficient, RGBColorHi );

		AmbientIrradiance.m_Light[ LightIndex ] += IrradianceColor;
	}
}

void RosaWorld::RasterizeSkyLighting( const RosaWorldCubemap& EnvironmentCubemap, const RosaWorldCubemap& SkyCubemap, RosaWorldCubemap& SkyLightingCubemap )
{
	static const Vector skBlack = Vector( 0.0f, 0.0f, 0.0f );

	for( uint Index = 0; Index < 6; ++Index )
	{
		const STextureData&	SkyTextureData	= SkyCubemap.GetData( Index );
		const STextureData&	EnvTextureData	= EnvironmentCubemap.GetData( Index );
		STextureData&		OutTextureData	= SkyLightingCubemap.GetData( Index );

		OutTextureData.m_Format				= SkyTextureData.m_Format;
		OutTextureData.m_Width				= SkyTextureData.m_Width;
		OutTextureData.m_Height				= SkyTextureData.m_Height;

		DEVASSERT( SkyTextureData.m_MipChain.Size() == 1 );
		const TTextureMip&	SkyMip			= SkyTextureData.m_MipChain[0];
		DEVASSERT( EnvTextureData.m_MipChain.Size() == 1 );
		const TTextureMip&	EnvMip			= EnvTextureData.m_MipChain[0];
		DEVASSERT( OutTextureData.m_MipChain.Size() == 0 );
		TTextureMip&		OutMip			= OutTextureData.m_MipChain.PushBack();

		DEVASSERT( SkyMip.Size() == EnvMip.Size() );
		OutMip.Resize( SkyMip.Size() );

		const uint			NumTexels		= Square( SkyTextureData.m_Width );
		for( uint TexelIndex = 0; TexelIndex < NumTexels; ++TexelIndex )
		{
			const uint		ByteIndex	= TexelIndex * 16;

			const float		SkyR		= GetCubemapFloat( SkyMip, ByteIndex );
			const float		SkyG		= GetCubemapFloat( SkyMip, ByteIndex + 4 );
			const float		SkyB		= GetCubemapFloat( SkyMip, ByteIndex + 8 );

			const float		EnvA		= GetCubemapFloat( EnvMip, ByteIndex + 12 );

			const Vector	SkyColor	= Vector( SkyR, SkyG, SkyB );
			const Vector	OutColor	= SkyColor.LERP( EnvA, skBlack );

			GetCubemapFloat( OutMip, ByteIndex )		= OutColor.r;
			GetCubemapFloat( OutMip, ByteIndex + 4 )	= OutColor.g;
			GetCubemapFloat( OutMip, ByteIndex + 8 )	= OutColor.b;
			GetCubemapFloat( OutMip, ByteIndex + 12 )	= 1.0f;
		}
	}
}

// Equivalent to GetCubeLight in shader code
Vector4 RosaWorld::GetIrradiance( const SVoxelIrradiance& AmbientIrradiance, const Vector& Direction )
{
	const Vector	DirectionSq	= Direction * Direction;
	const uint		IndexX		= ( Direction.x >= 0.0f ) ? 0 : 1;
	const uint		IndexY		= ( Direction.y >= 0.0f ) ? 2 : 3;
	const uint		IndexZ		= ( Direction.z >= 0.0f ) ? 4 : 5;
	return
		DirectionSq.x * AmbientIrradiance.m_Light[ IndexX ] +
		DirectionSq.y * AmbientIrradiance.m_Light[ IndexY ] +
		DirectionSq.z * AmbientIrradiance.m_Light[ IndexZ ];
}

void RosaWorld::AddEmissiveCubemap( const RosaWorldCubemap& EmissiveCubemap, RosaWorldCubemap& EnvironmentCubemap )
{
	DEVASSERT( EnvironmentCubemap.GetData( 0 ).m_Width == EmissiveCubemap.GetData( 0 ).m_Width );

	for( uint Index = 0; Index < 6; ++Index )
	{
		const STextureData&	EmTextureData			= EmissiveCubemap.GetData( Index );
		STextureData&		EnvTextureData			= EnvironmentCubemap.GetData( Index );
		const uint			NumTexels				= Square( EnvTextureData.m_Width );
		const TTextureMip&	EmMip					= EmTextureData.m_MipChain[0];
		TTextureMip&		EnvMip					= EnvTextureData.m_MipChain[0];

		for( uint TexelIndex = 0; TexelIndex < NumTexels; ++TexelIndex )
		{
			const uint		ByteIndex					= TexelIndex * 16;

			GetCubemapFloat( EnvMip, ByteIndex )		+= GetCubemapFloat( EmMip, ByteIndex );
			GetCubemapFloat( EnvMip, ByteIndex + 4 )	+= GetCubemapFloat( EmMip, ByteIndex + 4 );
			GetCubemapFloat( EnvMip, ByteIndex + 8 )	+= GetCubemapFloat( EmMip, ByteIndex + 8 );
		}
	}
}

// NOTE: Mirrored in shader SampleFog()
Vector4 RosaWorld::SampleFog( const float NearFar, const float LoHi )
{
	DEBUGASSERT( NearFar >= 0.0f && NearFar <= 1.0f );
	DEBUGASSERT( LoHi >= 0.0f && LoHi <= 1.0f );

	const SWorldDef& FogWorldDef = GetCurrentWorldDef();

	const float		NearFarAlpha	= Pow( Saturate( ( NearFar -	FogWorldDef.m_FogNearFarCurve.x ) *	FogWorldDef.m_FogNearFarCurve.z ),	FogWorldDef.m_FogNearFarCurve.w );
	const float		LoHiAlpha		= Pow( Saturate( ( LoHi -		FogWorldDef.m_FogLoHiCurve.x ) *	FogWorldDef.m_FogLoHiCurve.z ),		FogWorldDef.m_FogLoHiCurve.w );
	const Vector4	FogLo			= FogWorldDef.m_FogColorNearLo.LERP( NearFarAlpha, FogWorldDef.m_FogColorFarLo );
	const Vector4	FogHi			= FogWorldDef.m_FogColorNearHi.LERP( NearFarAlpha, FogWorldDef.m_FogColorFarHi );
	const Vector4	Fog				= FogLo.LERP( LoHiAlpha, FogHi );

	return Fog;
}

// Equivalent to volumetric fog shaders
Vector4 RosaWorld::SampleFogMeshValues( const float Distance, const Vector4& FogMeshColor, const Vector4& FogMeshParams )
{
	const float		FogFalloff	= Pow( Saturate( Distance * FogMeshParams.x ), FogMeshParams.y );
	const Vector4	Fog			= Vector4( FogMeshColor.r, FogMeshColor.g, FogMeshColor.b, FogFalloff * FogMeshColor.a );

	return Fog;
}

void RosaWorld::BlendFogIntoCubemap( const SimpleString& CubemapName, RosaWorldCubemap& EnvironmentCubemap )
{
	MAKEHASH( CubemapName );

	const SWorldDef&	FogWorldDef	= GetCurrentWorldDef();

	// This is a scalar on fog blend in cubemap, and is a legacy Vamp thing from before WallDistance was implemented.
	// I don't think I need it anymore, but it doesn't hurt to have it here.
	STATICHASH( FogFactor );
	const float			FogFactor		= ConfigManager::GetInheritedFloat( sFogFactor, 1.0f, sCubemapName );

	// This is distance in meters, modeling how far the reflected environment is supposed to be from the viewer.
	// This is the max distance at the cubemap horizon, blending to zero at up/down (a hack assuming close ceilings/floors)
	// (Also, note that this has nothing to do with the sky light, which always gets fully fogged, except in the interior ambient model.)
	STATICHASH( WallDistance );
	const float			WallDistance	= ConfigManager::GetInheritedFloat( sWallDistance, FogWorldDef.m_FogFar, sCubemapName );

	STATICHASH( EyeHeight );
	const float EyeHeight = ConfigManager::GetInheritedFloat( sEyeHeight, 0.0f, sCubemapName );
	const bool UsingEnvironmentSize = ( EyeHeight > 0.0f );

	STATICHASH( WallHeight );
	const float WallHeight = ConfigManager::GetInheritedFloat( sWallHeight, 0.0f, sCubemapName );
	const float HalfWallHeight = 0.5f * WallHeight;

	const AABB			EnvironmentBox		= AABB::CreateFromCenterAndExtents( Vector( 0.0f, 0.0f, HalfWallHeight ), Vector( WallDistance, WallDistance, HalfWallHeight ) );
	const Vector		EyePosition			= Vector( 0.0f, 0.0f, EyeHeight );
	const float			RcpHeightFogRange	= 1.0f / ( FogWorldDef.m_HeightFogHi - FogWorldDef.m_HeightFogLo );

	const Vector4		RegionFogScalar		= GetRegionFogScalar( CubemapName );

	Vector4 FogMeshColor;
	Vector4 FogMeshParams;
	const bool			GotFogMeshValues	= GetFogMeshValuesForCubemap( CubemapName, FogMeshColor, FogMeshParams );

	const uint			Size		= EnvironmentCubemap.GetData( 0 ).m_Width;
	const float			fSize		= static_cast<float>( Size );
	const float			RcpSize		= 1.0f / ( fSize - 1.0f );	// Subtract 1 so [0..7] scales into [0..1] instead of [0..7/8]

	for( uint Index = 0; Index < 6; ++Index )
	{
		STextureData&		EnvTextureData			= EnvironmentCubemap.GetData( Index );
		const uint			NumTexels				= Square( EnvTextureData.m_Width );
		TTextureMip&		EnvMip					= EnvTextureData.m_MipChain[0];

		for( uint TexelIndex = 0; TexelIndex < NumTexels; ++TexelIndex )
		{
			const uint		ByteIndex		= TexelIndex * 16;

			const uint		X				= TexelIndex % Size;
			const uint		Y				= TexelIndex / Size;

			const float		fX				= ( static_cast<float>( X ) * RcpSize ) * 2.0f - 1.0f;
			const float		fY				= ( static_cast<float>( Y ) * RcpSize ) * 2.0f - 1.0f;
			const float		fZ				= 1.0f;

			Vector			CubeLocation;
			if( Index == 0 )		{ CubeLocation = Vector(  fZ, -fX, -fY ); }
			else if( Index == 1 )	{ CubeLocation = Vector( -fZ,  fX, -fY ); }
			else if( Index == 2 )	{ CubeLocation = Vector(  fX,  fZ, -fY ); }
			else if( Index == 3 )	{ CubeLocation = Vector( -fX, -fZ, -fY ); }
			else if( Index == 4 )	{ CubeLocation = Vector(  fX,  fY,  fZ ); }
			else					{ CubeLocation = Vector(  fX, -fY, -fZ ); }

			// HACKHACK: Avoid undesirable peaks of fog in corners of environments
			const Vector	YZDirection	= Vector( 0.0f, Max( Abs( CubeLocation.x ), Abs( CubeLocation.y ) ), CubeLocation.z ).GetNormalized();
			const Vector	Direction	= CubeLocation.GetNormalized();

			float FogT			= 0.0f;
			float FogHeightT	= 0.0f;
			float FogDistance	= 0.0f;
			if( UsingEnvironmentSize )
			{
				const Vector	FarPosition	= YZDirection * ( 3.0f * WallDistance );	// Actual furthest possible distance should have a limit of 2*WallDistance due to WallHeight limits
				const Segment	BoxSegment	= Segment( FarPosition, EyePosition );
				CollisionInfo	BoxInfo;
				if( BoxSegment.Intersects( EnvironmentBox, &BoxInfo ) )
				{
					FogDistance				= ( BoxInfo.m_Out_Intersection - EyePosition ).Length();
					FogT					= Min( FogDistance / FogWorldDef.m_FogFar, 1.0f );
					const float	FogHeight	= 0.5f + BoxInfo.m_Out_Intersection.z;	// HACKHACK, add 1/2m to account for usual ground height
					FogHeightT				= Clamp( FogHeight, FogWorldDef.m_HeightFogLo, FogWorldDef.m_HeightFogHi ) * RcpHeightFogRange;
				}
				else
				{
					WARN;
				}
			}
			else
			{
				const float		FogDistanceMax	= Min( WallDistance / FogWorldDef.m_FogFar, 1.0f );
				const float		FogHemiFactor	= Min( 1.0f + Direction.z, 1.0f ); //1.0f - Abs( Direction.z );	// Zero distance straight down, max at and above horizon (changed from Vamp, where fog was zero straight up)
				FogT							= FogDistanceMax * FogHemiFactor;

				// Height fog exp blended from straight down to straight up; it's not going to perfectly match any environment, so why not
				FogHeightT						= 0.5f * ( 1.0f + Direction.z );
			}

			const float		FogExp			= Lerp( FogWorldDef.m_HeightFogLoExp, FogWorldDef.m_HeightFogHiExp, FogHeightT );
			const float		FogNearFar		= Pow( FogT, FogExp );
			const float		FogLoHi			= 0.5f * ( 1.0f + Direction.Dot( FogWorldDef.m_SunVector ) );
			Vector4			FogColor		= RegionFogScalar * SampleFog( FogNearFar, FogLoHi );
			if( GotFogMeshValues )
			{
				DEVASSERTDESC( UsingEnvironmentSize, "Old Vamp-style cubemap fog system doesn't support fog mesh stuff. FogDistance will be zero." );

				// Blend local fog mesh value over the global fog, to match the renderer.
				const Vector4	SampledFogMeshColor	= SampleFogMeshValues( FogDistance, FogMeshColor, FogMeshParams );
				FogColor							= FogColor.LERP( SampledFogMeshColor.a, SampledFogMeshColor );
			}

			const float		EnvR			= GetCubemapFloat( EnvMip, ByteIndex );
			const float		EnvG			= GetCubemapFloat( EnvMip, ByteIndex + 4 );
			const float		EnvB			= GetCubemapFloat( EnvMip, ByteIndex + 8 );
			const Vector	EnvColor		= Vector( EnvR, EnvG, EnvB );

			const Vector	OutColor		= EnvColor.LERP( FogFactor * FogColor.a, Vector( FogColor ) );

			GetCubemapFloat( EnvMip, ByteIndex )		= OutColor.r;
			GetCubemapFloat( EnvMip, ByteIndex + 4 )	= OutColor.g;
			GetCubemapFloat( EnvMip, ByteIndex + 8 )	= OutColor.b;
			// ROSANOTE: Intentionally leaving alpha alone here, it'll be used in the next step
		}
	}
}

void RosaWorld::LightEnvironmentCubemap( const SVoxelIrradiance& AmbientIrradiance, RosaWorldCubemap& EnvironmentCubemap )
{
	const uint			Size		= EnvironmentCubemap.GetData( 0 ).m_Width;
	const float			fSize		= static_cast<float>( Size );
	const float			RcpSize		= 1.0f / ( fSize - 1.0f );	// Subtract 1 so [0..7] scales into [0..1] instead of [0..7/8]

	for( uint Index = 0; Index < 6; ++Index )
	{
		STextureData&		EnvTextureData			= EnvironmentCubemap.GetData( Index );
		const uint			NumTexels				= Square( EnvTextureData.m_Width );
		TTextureMip&		EnvMip					= EnvTextureData.m_MipChain[0];

		for( uint TexelIndex = 0; TexelIndex < NumTexels; ++TexelIndex )
		{
			const uint		ByteIndex		= TexelIndex * 16;

			const uint		X				= TexelIndex % Size;
			const uint		Y				= TexelIndex / Size;

			const float		fX				= ( static_cast<float>( X ) * RcpSize ) * 2.0f - 1.0f;
			const float		fY				= ( static_cast<float>( Y ) * RcpSize ) * 2.0f - 1.0f;
			const float		fZ				= 1.0f;

			Vector			CubeLocation;
			if( Index == 0 )		{ CubeLocation = Vector(  fZ, -fX, -fY ); }
			else if( Index == 1 )	{ CubeLocation = Vector( -fZ,  fX, -fY ); }
			else if( Index == 2 )	{ CubeLocation = Vector(  fX,  fZ, -fY ); }
			else if( Index == 3 )	{ CubeLocation = Vector( -fX, -fZ, -fY ); }
			else if( Index == 4 )	{ CubeLocation = Vector(  fX,  fY,  fZ ); }
			else					{ CubeLocation = Vector(  fX, -fY, -fZ ); }

			const Vector	Direction		= CubeLocation.GetNormalized();

			const float		EnvR			= GetCubemapFloat( EnvMip, ByteIndex );
			const float		EnvG			= GetCubemapFloat( EnvMip, ByteIndex + 4 );
			const float		EnvB			= GetCubemapFloat( EnvMip, ByteIndex + 8 );
			const Vector	EnvColor		= Vector( EnvR, EnvG, EnvB );

			const Vector	OutColor		= EnvColor * GetIrradiance( AmbientIrradiance, -Direction );

			GetCubemapFloat( EnvMip, ByteIndex )		= OutColor.r;
			GetCubemapFloat( EnvMip, ByteIndex + 4 )	= OutColor.g;
			GetCubemapFloat( EnvMip, ByteIndex + 8 )	= OutColor.b;
		}
	}
}

// FOGTODO: Deprecate? May not be needed now that I have fog meshes.
Vector4 RosaWorld::GetRegionFogScalar( const SimpleString& CubemapName )
{
	MAKEHASH( CubemapName );

	const Vector4 RegionFogScalarHSVA	= HSV::GetConfigHSVA( "RegionFogScalar", sCubemapName, Vector4( 0.0f, 0.0f, 1.0f, 1.0f ) );
	const Vector4 RegionFogScalar		= HSV::HSVToRGB_AlphaPass( RegionFogScalarHSVA );

	return RegionFogScalar;
}

void RosaWorld::CreateSkyCubemap( const SimpleString& CubemapName, RosaWorldCubemap& SkyCubemap, const RosaWorldCubemap& ReferenceCubemap )
{
	const SWorldDef&	WorldDef			= GetCurrentWorldDef();

	const Vector4		RegionFogScalar		= GetRegionFogScalar( CubemapName );

	Vector4 FogMeshColor;
	Vector4 FogMeshParams;
	const bool			GotFogMeshValues	= GetFogMeshValuesForCubemap( CubemapName, FogMeshColor, FogMeshParams );

	// Just get the size from environment reference cubemap, we don't use it after that
	const uint			Size		= ReferenceCubemap.GetData( 0 ).m_Width;
	const float			fSize		= static_cast<float>( Size );
	const float			RcpSize		= 1.0f / ( fSize - 1.0f );	// Subtract 1 so [0..7] scales into [0..1] instead of [0..7/8]
	Array<Vector4>		LightArray[6];

	// Gather lights in all directions first so we divide evenly
	for( uint Index = 0; Index < 6; ++Index )
	{
		LightArray[ Index ].Resize( Square( Size ) );

		for( uint X = 0; X < Size; ++X )
		{
			for( uint Y = 0; Y < Size; ++Y )
			{
				// Don't add pixel offsets here, I want [0..7] to scale into [0..1] instead of [0.0625..0.9375]
				// This way, cubemap edges will have same values as neighboring edges, just like I need.
				const float		fX				= ( static_cast<float>( X ) * RcpSize ) * 2.0f - 1.0f;
				const float		fY				= ( static_cast<float>( Y ) * RcpSize ) * 2.0f - 1.0f;
				const float		fZ				= 1.0f;

				// HACKHACK: Get the location inside the cube
				Vector			CubeLocation;
				if( Index == 0 )		{ CubeLocation = Vector(  fZ, -fX, -fY ); }
				else if( Index == 1 )	{ CubeLocation = Vector( -fZ,  fX, -fY ); }
				else if( Index == 2 )	{ CubeLocation = Vector(  fX,  fZ, -fY ); }
				else if( Index == 3 )	{ CubeLocation = Vector( -fX, -fZ, -fY ); }
				else if( Index == 4 )	{ CubeLocation = Vector(  fX,  fY,  fZ ); }
				else					{ CubeLocation = Vector(  fX, -fY, -fZ ); }

				const Vector	SkyDirection		= CubeLocation.GetNormalized();
				const float		SunValue			= 0.5f * ( 1.0f + SkyDirection.Dot( WorldDef.m_SunVector ) );
				const Vector4	SkyColor			= HSV::HSVToRGB( WorldDef.m_SkyColorLo.LERP( Square( SunValue ), WorldDef.m_SkyColorHi ) );
				// Reminder: sky color is already linear, not sRGB (I guess HSVtoRGB works for that purpose?)

				// Look up directional fog values. Don't care about height here,
				// because height fog is irrelevant at far Z.
				Vector4			FogColor			= RegionFogScalar * SampleFog( 1.0f, SunValue );
				if( GotFogMeshValues )
				{
					// Blend local fog mesh value over the global fog, to match the renderer.
					// We can ignore exponent and distance here (and skip SampleFogMeshValues) because it's the sky
					// (and we don't have a reference for the room size or whether there's more fog above it).
					FogColor						= FogColor.LERP( FogMeshColor.a, FogMeshColor );
				}

				LightArray[ Index ][ X + Y * Size ]	= SkyColor.LERP( FogColor.a, Vector( FogColor ) );
			}
		}
	}

	for( uint Index = 0; Index < 6; ++Index )
	{
		STextureData&	TextureData	= SkyCubemap.GetData( Index );
		TextureData.m_Format		= EIF_ARGB32F;
		TextureData.m_Width			= Size;
		TextureData.m_Height		= Size;
		TTextureMip&	Mip			= TextureData.m_MipChain.PushBack();
		Mip.Resize( 16 * Square( Size ) );

		FOR_EACH_ARRAY( LightIter, LightArray[ Index ], Vector4 )
		{
			const Vector4	Light		= LightIter.GetValue();
			const uint		ByteIndex	= 16 * LightIter.GetIndex();
			GetCubemapFloat( Mip, ByteIndex )		= Light.r;
			GetCubemapFloat( Mip, ByteIndex + 4 )	= Light.g;
			GetCubemapFloat( Mip, ByteIndex + 8 )	= Light.b;
			GetCubemapFloat( Mip, ByteIndex + 12 )	= 1.0f;
		}
	}
}

void RosaWorld::BlendSkyIntoEnvironmentCubemap( const RosaWorldCubemap& SkyCubemap, RosaWorldCubemap& InOutEnvironmentCubemap )
{
	for( uint Index = 0; Index < 6; ++Index )
	{
		const STextureData&	SkyTextureData	= SkyCubemap.GetData( Index );
		STextureData&		EnvTextureData	= InOutEnvironmentCubemap.GetData( Index );
		const uint			NumTexels		= Square( SkyTextureData.m_Width );
		const TTextureMip&	SkyMip			= SkyTextureData.m_MipChain[0];
		TTextureMip&		EnvMip			= EnvTextureData.m_MipChain[0];

		for( uint TexelIndex = 0; TexelIndex < NumTexels; ++TexelIndex )
		{
			const uint		ByteIndex	= TexelIndex * 16;

			const float		SkyR		= GetCubemapFloat( SkyMip, ByteIndex );
			const float		SkyG		= GetCubemapFloat( SkyMip, ByteIndex + 4 );
			const float		SkyB		= GetCubemapFloat( SkyMip, ByteIndex + 8 );

			const float		EnvR		= GetCubemapFloat( EnvMip, ByteIndex );
			const float		EnvG		= GetCubemapFloat( EnvMip, ByteIndex + 4 );
			const float		EnvB		= GetCubemapFloat( EnvMip, ByteIndex + 8 );
			const float		EnvA		= GetCubemapFloat( EnvMip, ByteIndex + 12 );

			const Vector	SkyColor	= Vector( SkyR, SkyG, SkyB );
			const Vector	EnvColor	= Vector( EnvR, EnvG, EnvB );
			const Vector	OutColor	= SkyColor.LERP( EnvA, EnvColor );

			GetCubemapFloat( EnvMip, ByteIndex )		= OutColor.r;
			GetCubemapFloat( EnvMip, ByteIndex + 4 )	= OutColor.g;
			GetCubemapFloat( EnvMip, ByteIndex + 8 )	= OutColor.b;
			GetCubemapFloat( EnvMip, ByteIndex + 12 )	= 1.0f;
		}
	}
}

void RosaWorld::GenerateIrradianceFromCubemap( const RosaWorldCubemap& SourceCubemap, SVoxelIrradiance& AmbientIrradiance )
{
	for( uint LightIndex = 0; LightIndex < 6; ++LightIndex )
	{
		Vector	Facing;
		if( LightIndex == 0 )		{ Facing = Vector(  1.0f,  0.0f,  0.0f ); }
		else if( LightIndex == 1 )	{ Facing = Vector( -1.0f,  0.0f,  0.0f ); }
		else if( LightIndex == 2 )	{ Facing = Vector(  0.0f,  1.0f,  0.0f ); }
		else if( LightIndex == 3 )	{ Facing = Vector(  0.0f, -1.0f,  0.0f ); }
		else if( LightIndex == 4 )	{ Facing = Vector(  0.0f,  0.0f,  1.0f ); }
		else						{ Facing = Vector(  0.0f,  0.0f, -1.0f ); }

		Vector	LightSum;
		float	WeightSum	= 0.0f;
		for( uint Index = 0; Index < 6; ++Index )
		{
			const STextureData&	TextureData	= SourceCubemap.GetData( Index );
			ASSERT( TextureData.m_Width == TextureData.m_Height );
			ASSERT( TextureData.m_Format == EIF_ARGB32F );
			const TTextureMip&	Mip			= TextureData.m_MipChain[0];
			const float			fSize		= static_cast<float>( TextureData.m_Width );
			const float			RcpSize		= 1.0f / fSize;
			const float			HalfTexel	= 0.5f * RcpSize;

			for( uint X = 0; X < TextureData.m_Width; ++X )
			{
				for( uint Y = 0; Y < TextureData.m_Height; ++Y )
				{
					const uint		ByteIndex	= 16 * ( X + Y * TextureData.m_Width );
					const float		R			= GetCubemapFloat( Mip, ByteIndex );
					const float		G			= GetCubemapFloat( Mip, ByteIndex + 4 );
					const float		B			= GetCubemapFloat( Mip, ByteIndex + 8 );
					const Vector	Light		= Vector( R, G, B );

					const float		fX			= ( static_cast<float>( X ) * RcpSize + HalfTexel ) * 2.0f - 1.0f;
					const float		fY			= ( static_cast<float>( Y ) * RcpSize + HalfTexel ) * 2.0f - 1.0f;
					const float		fZ			= 1.0f;

					// HACKHACK: Get the location inside the cube
					Vector	TexelLoc;
					if( Index == 0 )		{ TexelLoc = Vector(  fZ, -fX, -fY ); }
					else if( Index == 1 )	{ TexelLoc = Vector( -fZ,  fX, -fY ); }
					else if( Index == 2 )	{ TexelLoc = Vector(  fX,  fZ, -fY ); }
					else if( Index == 3 )	{ TexelLoc = Vector( -fX, -fZ, -fY ); }
					else if( Index == 4 )	{ TexelLoc = Vector(  fX,  fY,  fZ ); }
					else					{ TexelLoc = Vector(  fX, -fY, -fZ ); }

					const Vector	ToTexel		= TexelLoc.GetNormalized();	// i.e., (TexelLoc - Origin).Norm()
					const float		CosTheta	= Facing.Dot( ToTexel );
					const float		Weight		= Saturate( CosTheta );

					LightSum	+= Weight * Light;
					WeightSum	+= Weight;
				}
			}
		}

		AmbientIrradiance.m_Light[ LightIndex ] = LightSum / WeightSum;
	}
}

void RosaWorld::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;

	// Get the ambient region which contains the camera and use it to adjust exposure/reverb.
	// ROSANOTE: This expects regions to be ordered with bounding regions first.
	RosaFramework* const	pFramework				= RosaFramework::GetInstance();
	RosaGame* const			pGame					= pFramework->GetGame();
#if BUILD_DEV
	IRenderer* const		pRenderer				= pFramework->GetRenderer();
	const View&				Camera					= pRenderer->DEV_IsLockedFrustum() ? pRenderer->DEV_GetLockedFrustumView() : *pFramework->GetMainView();
#else
	const View&				Camera					= *pFramework->GetMainView();
#endif
	float					TargetExposure			= GetCurrentWorldDef().m_Exposure;
	Vector4					TargetRegionFogScalar	= Vector4( 1.0f, 1.0f, 1.0f, 1.0f );
	float					TargetMinimapScalar		= GetCurrentWorldDef().m_MinimapScalar;
	Vector					TargetEdgeColorHSV		= GetCurrentWorldDef().m_EdgeColorHSV;
	const SimpleString* 	pTargetAmbience			= &GetCurrentWorldDef().m_Ambience;
	const SimpleString* 	pTargetReverb			= &GetCurrentWorldDef().m_Reverb;
	bool					BreakIter				= false;
	FOR_EACH_ARRAY( SectorIter, m_Sectors, SSector )
	{
		const SSector& Sector = SectorIter.GetValue();

		if( !Sector.m_RenderBound.Intersects( Camera.GetLocation() ) )
		{
			continue;
		}

		FOR_EACH_ARRAY_REVERSE( AmbientLightIter, Sector.m_AmbientLights, SAmbientLight )
		{
			const SAmbientLight& AmbientLight = AmbientLightIter.GetValue();
			DEVASSERT( AmbientLight.m_Mesh );

			if( !AmbientLight.m_Hull.m_Hull.Contains( Camera.GetLocation() ) )
			{
				continue;
			}

			TargetExposure			= AmbientLight.m_Exposure;
			TargetRegionFogScalar	= AmbientLight.m_RegionFogScalar;
			TargetMinimapScalar		= AmbientLight.m_MinimapScalar;
			TargetEdgeColorHSV		= AmbientLight.m_EdgeColorHSV;
			pTargetAmbience			= &AmbientLight.m_Ambience;
			pTargetReverb			= &AmbientLight.m_Reverb;
			BreakIter				= true;
			break;
		}

		if( BreakIter )
		{
			break;
		}
	}

	pGame->AdjustExposure( TargetExposure, DeltaTime );
	pGame->AdjustFogRegionScalar( TargetRegionFogScalar, DeltaTime );
	pGame->AdjustMinimapScalar( TargetMinimapScalar, DeltaTime );
	pGame->AdjustEdgeColorHSV( TargetEdgeColorHSV, DeltaTime );
	pGame->SetAmbience( *pTargetAmbience );
	pGame->SetReverb( *pTargetReverb );

	TickWind();
}

void RosaWorld::TickWind()
{
	RosaFramework* const	pFramework		= RosaFramework::GetInstance();
	RosaGame* const			pGame			= pFramework->GetGame();

	const SWorldDef&		WorldDef		= GetCurrentWorldDef();

	// HACKHACK: Scroll both maps in XY, but one more X and one more Y
	const Vector4			WindWaterVector	= WorldDef.m_WindSwayIntensity * Vector4(
		WorldDef.m_WindSwayDirection.x,
		0.5f * WorldDef.m_WindSwayDirection.y,
		0.5f * WorldDef.m_WindSwayDirection.x,
		WorldDef.m_WindSwayDirection.y );
	pGame->SetWindWaterVector( WindWaterVector );

#if 0
	// Turn off wind noise for shader phase testing
	const Vector		FlapDirection	= Vector( 1.0f, 1.0f, 1.0f );
	const Vector4		WindSway		= Vector4( WorldDef.m_WindSwayDirection * ( WorldDef.m_WindSwayIntensity ), 0.0f );
	const Vector4		WindLift		= Vector4( 0.0f, 0.0f, WorldDef.m_WindLiftIntensity, 0.0f );
	const Vector4		WindFlap		= Vector4( FlapDirection * ( WorldDef.m_WindFlapIntensity ), 0.0f );
	const Matrix		WindMatrix		= Matrix( WindSway, WindLift, WindFlap, Vector4() );
	pGame->SetWindMatrix( WindMatrix );
#else
	const float			CurrentTime		= WBWorld::GetInstance()->GetTime();

	const float			SwayNoiseX		= ( WorldDef.m_WindSwayNoiseOffset + CurrentTime ) * WorldDef.m_WindSwayNoiseScalar;
	const float			SwayNoise		= Noise::SumNoise1( SwayNoiseX, WorldDef.m_WindSwayNoiseOctaves, Noise::CubicNoise1 );
	const float			SwayNoiseRange	= Noise::GetOctaveScale( WorldDef.m_WindSwayNoiseOctaves );
	const float			SwayNoiseAlpha	= 0.5f * ( 1.0f + ( SwayNoise / SwayNoiseRange ) );
	DEBUGASSERT( SwayNoiseAlpha >= 0.0f && SwayNoiseAlpha <= 1.0f );

	const float			LiftNoiseX		= ( WorldDef.m_WindLiftNoiseOffset + CurrentTime ) * WorldDef.m_WindLiftNoiseScalar;
	const float			LiftNoise		= Noise::SumNoise1( LiftNoiseX, WorldDef.m_WindLiftNoiseOctaves, Noise::CubicNoise1 );
	const float			LiftNoiseRange	= Noise::GetOctaveScale( WorldDef.m_WindLiftNoiseOctaves );
	const float			LiftNoiseAlpha	= LiftNoise / LiftNoiseRange;
	DEBUGASSERT( LiftNoiseAlpha >= -1.0f && LiftNoiseAlpha <= 1.0f );

	const float			FlapNoiseX		= ( WorldDef.m_WindFlapNoiseOffset + CurrentTime ) * WorldDef.m_WindFlapNoiseScalar;
	const float			FlapNoise		= Noise::SumNoise1( FlapNoiseX, WorldDef.m_WindFlapNoiseOctaves, Noise::CubicNoise1 );
	const float			FlapNoiseRange	= Noise::GetOctaveScale( WorldDef.m_WindFlapNoiseOctaves );
	const float			FlapNoiseAlpha	= FlapNoise / FlapNoiseRange;
	DEBUGASSERT( FlapNoiseAlpha >= -1.0f && FlapNoiseAlpha <= 1.0f );

	// Sway is XY, lift is Z, flap is vertex normal direction (see shader code)
	const Vector		FlapDirection	= Vector( 1.0f, 1.0f, 1.0f );

	const Vector4		WindSway		= Vector4( WorldDef.m_WindSwayDirection * ( WorldDef.m_WindSwayIntensity * SwayNoiseAlpha ), 0.0f );
	const Vector4		WindLift		= Vector4( 0.0f, 0.0f, WorldDef.m_WindLiftIntensity * LiftNoiseAlpha, 0.0f );
	const Vector4		WindFlap		= Vector4( FlapDirection * ( WorldDef.m_WindFlapIntensity * FlapNoiseAlpha ), 0.0f );
	const Matrix		WindMatrix		= Matrix( WindSway, WindLift, WindFlap, Vector4() );
	pGame->SetWindMatrix( WindMatrix );
#endif
}

void RosaWorld::RevealMinimapSector( const uint SectorIndex )
{
	DEVASSERT( m_Sectors.IsValidIndex( SectorIndex ) );

	if( m_VisitedSectors.PushBackUnique( SectorIndex ) )
	{
		// Make the minimap mesh (if any) renderable, if this is the first time it has been visited
		STATIC_HASHED_STRING( MinimapAParams );
		static const Vector4	skRenderableMinimapAParams		= Vector4( 1.0f, 0.0f, 0.0f, 0.0f );	// Only the x/red is used here
		const TMinimapMeshMap::Iterator	MinimapMeshIter	= m_MinimapMeshes.Search( SectorIndex );
		if( MinimapMeshIter.IsValid() )
		{
			Mesh* const pMinimapMesh = MinimapMeshIter.GetValue();
			DEVASSERT( pMinimapMesh );

			pMinimapMesh->SetShaderConstant( sMinimapAParams, skRenderableMinimapAParams );
		}
	}

	// Reveal any single-tile neighbors, mainly so we don't leave caps looking like valid exits to a room.
	const SSector&	Sector	= m_Sectors[ SectorIndex ];
	if( !Sector.m_IsSingleTile )
	{
		FOR_EACH_ARRAY( PortalIter, Sector.m_Portals, SSectorPortal )
		{
			const SSectorPortal&	SectorPortal	= PortalIter.GetValue();
			const uint				BackSectorIndex	= SectorPortal.m_BackSector;
			DEVASSERT( m_Sectors.IsValidIndex( BackSectorIndex ) );
			const SSector&			BackSector		= m_Sectors[ BackSectorIndex ];
			if( BackSector.m_IsSingleTile )
			{
				RevealMinimapSector( BackSectorIndex );
			}
		}
	}
}

void RosaWorld::RevealMinimap()
{
	 m_VisitedSectors.Reserve( m_Sectors.Size() );
	 FOR_EACH_INDEX( SectorIndex, m_Sectors.Size() )
	 {
		 RevealMinimapSector( SectorIndex );
	 }
}

void RosaWorld::Render()
{
	XTRACE_FUNCTION;

	IRenderer* const pRenderer = m_Framework->GetRenderer();

	if( m_SkyMesh )
	{
		pRenderer->AddMesh( m_SkyMesh );
	}

	if( m_SkylineMesh )
	{
		pRenderer->AddMesh( m_SkylineMesh );
	}

	if( m_SkyLightMesh )
	{
		pRenderer->AddMesh( m_SkyLightMesh );
	}

	m_VisibleSectors.Clear();
	m_VisibleSectors.Reserve( m_Sectors.Size() );
#if BUILD_DEV
	const View&		Camera		= pRenderer->DEV_IsLockedFrustum() ? pRenderer->DEV_GetLockedFrustumView() : *m_Framework->GetMainView();
#else
	const View&		Camera		= *m_Framework->GetMainView();
#endif
	const SRect		ViewBound	= SRect( -1.0f, 1.0f, 1.0f, -1.0f );
	FOR_EACH_ARRAY( SectorIter, m_Sectors, SSector )
	{
		const uint		SectorIndex	= SectorIter.GetIndex();
		const SSector&	Sector		= SectorIter.GetValue();
		if( Sector.m_RenderBound.Intersects( Camera.GetLocation() ) )
		{
			XTRACE_NAMED( GatherVisibleSectors );
			const Plane ViewPlane = Plane( Camera.GetRotation().ToVector(), Camera.GetLocation() );
			GatherVisibleSectors( Camera, ViewPlane, SectorIndex, ViewBound, m_VisibleSectors, 0 );

			// Mark the sector visited
			RevealMinimapSector( SectorIndex );
		}
	}

#if BUILD_DEV
	// HACKHACK for ghosting: if we're outside the world, all sectors will be visible
	if( m_VisibleSectors.Empty() )
	{
		FOR_EACH_ARRAY( SectorIter, m_Sectors, SSector )
		{
			m_VisibleSectors.PushBack( SectorIter.GetIndex() );
		}
	}
#endif

	AddShadowVisibleSectors();

	// Reset flags
	FOR_EACH_ARRAY( MeshIter, m_GeoMeshes, SGeoMesh )
	{
		SGeoMesh& GeoMesh = MeshIter.GetValue();
		GeoMesh.m_Rendered = false;
	}

	// Render only what's in visible sectors
	FOR_EACH_ARRAY( VisibleSectorIter, m_VisibleSectors, uint )
	{
		const uint		SectorIndex	= VisibleSectorIter.GetValue();
		const SSector&	Sector		= m_Sectors[ SectorIndex ];

		FOR_EACH_ARRAY( AmbientLightIter, Sector.m_AmbientLights, SAmbientLight )
		{
			const SAmbientLight& AmbientLight = AmbientLightIter.GetValue();
			DEVASSERT( AmbientLight.m_Mesh );
			pRenderer->AddMesh( AmbientLight.m_Mesh );
		}

		FOR_EACH_ARRAY( FogMeshIter, Sector.m_FogMeshes, RosaMesh* )
		{
			RosaMesh* const pFogMesh = FogMeshIter.GetValue();
			DEVASSERT( pFogMesh );
			pRenderer->AddMesh( pFogMesh );
		}

		FOR_EACH_ARRAY( GeoMeshIter, Sector.m_GeoMeshes, uint )
		{
			SGeoMesh& GeoMesh = m_GeoMeshes[ GeoMeshIter.GetValue() ];
			if( GeoMesh.m_Rendered )
			{
				continue;
			}

			DEVASSERT( GeoMesh.m_Mesh );
			pRenderer->AddMesh( GeoMesh.m_Mesh );
			GeoMesh.m_Rendered = true;
		}
	}

	// Render *all* minimap meshes; those not in visited sectors will be handled in the shader.
	// (See RosaWorldGen::CreateMinimapMeshes() and SDPRosaMinimapA::SetShaderParameters().)
	FOR_EACH_MAP( MinimapMeshIter, m_MinimapMeshes, uint, Mesh* )
	{
		Mesh* const pMinimapMesh = MinimapMeshIter.GetValue();
		DEVASSERT( pMinimapMesh );

		pRenderer->AddMesh( pMinimapMesh );
	}
}

uint RosaWorld::CountVisibleGeoMeshes()
{
	uint NumVisibleGeoMeshes = 0;

	FOR_EACH_ARRAY( MeshIter, m_GeoMeshes, SGeoMesh )
	{
		SGeoMesh& GeoMesh = MeshIter.GetValue();
		GeoMesh.m_Rendered = false;
	}

	FOR_EACH_ARRAY( VisibleSectorIter, m_VisibleSectors, uint )
	{
		const SSector& Sector = m_Sectors[ VisibleSectorIter.GetValue() ];
		FOR_EACH_ARRAY( GeoMeshIter, Sector.m_GeoMeshes, uint )
		{
			SGeoMesh& GeoMesh = m_GeoMeshes[ GeoMeshIter.GetValue() ];
			if( GeoMesh.m_Rendered )
			{
				continue;
			}

			DEVASSERT( GeoMesh.m_Mesh );
			GeoMesh.m_Rendered = true;
			++NumVisibleGeoMeshes;
		}
	}

	return NumVisibleGeoMeshes;
}

bool RosaWorld::IsLocationInVisibleSector( const Vector& Location ) const
{
	FOR_EACH_ARRAY( VisibleSectorIter, m_VisibleSectors, uint )
	{
		const uint&		VisibleSectorIndex	= VisibleSectorIter.GetValue();
		const SSector&	VisibleSector		= m_Sectors[ VisibleSectorIndex ];
		if( Location.Intersects( VisibleSector.m_RenderBound ) )
		{
			return true;
		}
	}

	return false;
}

bool RosaWorld::DoesAABBIntersectVisibleSector( const AABB& Bounds ) const
{
	FOR_EACH_ARRAY( VisibleSectorIter, m_VisibleSectors, uint )
	{
		const uint&		VisibleSectorIndex	= VisibleSectorIter.GetValue();
		const SSector&	VisibleSector		= m_Sectors[ VisibleSectorIndex ];
		if( Bounds.Intersects( VisibleSector.m_RenderBound ) )
		{
			return true;
		}
	}

	return false;
}

bool RosaWorld::GetContainingSectorIndex( const Vector& Location, uint& OutSectorIndex ) const
{
	FOR_EACH_INDEX( SectorIndex, m_Sectors.Size() )
	{
		const SSector& Sector = m_Sectors[ SectorIndex ];
		if( Location.Intersects( Sector.m_RenderBound ) )
		{
			OutSectorIndex = SectorIndex;
			return true;
		}
	}

	return false;
}

bool RosaWorld::IsSectorVisibleFromVisibleSector( const uint SectorIndex ) const
{
	const uint		SectorStride		= m_Sectors.Size();
	DEBUGASSERT( SectorIndex < SectorStride );

	FOR_EACH_ARRAY( VisibleSectorIter, m_VisibleSectors, uint )
	{
		const uint&	VisibleSectorIndex	= VisibleSectorIter.GetValue();
		if( m_SectorVisMatrix[ VisibleSectorIndex + SectorStride * SectorIndex ] )
		{
			return true;
		}
	}

	return false;
}

const Array<uint>* RosaWorld::GetSectorVisIncidentals( const uint SectorIndexA, const uint SectorIndexB ) const
{
	const uint SectorIndexLo = Min( SectorIndexA, SectorIndexB );
	const uint SectorIndexHi = Max( SectorIndexA, SectorIndexB );

	const Map<uint, Array<uint> >&			SectorVisIncidentalMap	= m_SectorVisIncidentals[ SectorIndexLo ];
	const Map<uint, Array<uint> >::Iterator	SectorVisIncidentalIter	= SectorVisIncidentalMap.Search( SectorIndexHi );
	return SectorVisIncidentalIter.IsValid() ? &SectorVisIncidentalIter.GetValue() : NULL;
}

bool RosaWorld::GetIncidentalSectorsFromVisibleSectors( const uint SectorIndex, Array<uint>& OutSectorVisIncidentals ) const
{
	const uint		SectorStride		= m_Sectors.Size();
	DEBUGASSERT( SectorIndex < SectorStride );

	bool IsSectorVisible = false;
	FOR_EACH_ARRAY( VisibleSectorIter, m_VisibleSectors, uint )
	{
		const uint&	VisibleSectorIndex	= VisibleSectorIter.GetValue();
		if( m_SectorVisMatrix[ VisibleSectorIndex + SectorStride * SectorIndex ] )
		{
			IsSectorVisible = true;
			const Array<uint>* const pSectorVisIncidentals = GetSectorVisIncidentals( VisibleSectorIndex, SectorIndex );
			if( pSectorVisIncidentals )
			{
				FOR_EACH_ARRAY( SectorVisIncidentalIter, *pSectorVisIncidentals, uint )
				{
					OutSectorVisIncidentals.PushBackUnique( SectorVisIncidentalIter.GetValue() );
				}
			}
		}
	}

	return IsSectorVisible;
}

bool RosaWorld::IsLocationVisibleFromAnyVisibleSector( const Vector& Location ) const
{
	uint SectorIndex;
	if( !GetContainingSectorIndex( Location, SectorIndex ) )
	{
		return false;
	}

	return IsSectorVisibleFromVisibleSector( SectorIndex );
}

void RosaWorld::AddShadowVisibleSectors()
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	m_SubVisibleSectors.Reserve( m_Sectors.Size() );

	uint SubVisibleSectorIndex;
	const Set<Mesh*>& ShadowLightMeshes = WBCompRosaMesh::GetAllShadowLightMeshes();
	FOR_EACH_SET( ShadowLightMeshIter, ShadowLightMeshes, Mesh* )
	{
		Mesh* const pShadowLightMesh = ShadowLightMeshIter.GetValue();
		DEVASSERT( pShadowLightMesh );

		// Skip lights that are in already-visible sectors
		if( IsLocationInVisibleSector( pShadowLightMesh->m_Location ) )
		{
			continue;
		}

		// Skip lights that don't intersect any visible sectors
		if( !DoesAABBIntersectVisibleSector( pShadowLightMesh->m_AABB ) )
		{
			continue;
		}

		// Skip lights outside world bounds (edge case)
		if( !GetContainingSectorIndex( pShadowLightMesh->m_Location, SubVisibleSectorIndex ) )
		{
			continue;
		}

		// Skip lights that are never potentially visible from any visible sector
		// It is *not* sufficient to only check visibility from camera's sector!
		//
		// Example: viewer at A, light in D; D is not visible from A, but B and C
		// are visible from each, so D should be added as a sub-visible sector.
		// A D
		// | |
		// B-C
		//
		// NOTE: This is matched with code in WBCompRosaMesh::Render() where the
		// shadow light mesh is not rendered if it will not be visible.
		m_SubVisibleSectorIncidentals.Clear();
		if( !GetIncidentalSectorsFromVisibleSectors( SubVisibleSectorIndex, m_SubVisibleSectorIncidentals ) )
		{
			continue;
		}

		m_SubVisibleSectors.PushBackUnique( SubVisibleSectorIndex );

		// Also add any incidentals that aren't already visible.
		//
		// Example: viewer at A, light in D; need to add C so its meshes
		// cast shadows into B.
		// A
		// |
		// B-C-D
		//
		// Here, C is an "incidental" between B and D, not visible from A
		// but necessary to render shadows from C onto B correctly.
		FOR_EACH_ARRAY( IncidentalIter, m_SubVisibleSectorIncidentals, uint )
		{
			const uint&	Incidental	= IncidentalIter.GetValue();
			if( m_VisibleSectors.Find( Incidental ) )
			{
				continue;
			}

			m_SubVisibleSectors.PushBackUnique( Incidental );
		}
	}

#if BUILD_DEBUG
	// Ensure we're not duplicating anything
	FOR_EACH_ARRAY( SubVisibleSectorIter, m_SubVisibleSectors, uint )
	{
		DEBUGASSERT( !m_VisibleSectors.Find( SubVisibleSectorIter.GetValue() ) );
	}
#endif

	m_VisibleSectors.Append( m_SubVisibleSectors );
	m_SubVisibleSectors.Clear();
}

bool RosaWorld::ClipPortal( const SSectorPortal& Portal, const View& Camera, const Plane& ViewPlane, const Matrix& ViewProjMat, const SRect& ViewBound, SRect& ClippedBound ) const
{
	//PROFILE_FUNCTION;

	const float	ViewZ					= -Camera.GetNearClip();
	uint		NumPointsBehindView		= 0;
	uint		NumPointsInFrontOfView	= 0;
	Vector4		Points[4];

	for( uint Index = 0; Index < 4; ++Index )
	{
		Vector4&		Point	= Points[ Index ];
		const Vector4	Corner	= Portal.m_Corners[ Index ];
		Point					= Corner * ViewProjMat;
		DEBUGASSERT( Point.w != 0.0f );
		const float		RecpW	= 1.0f / Abs( Point.w );

		// HACKHACK: Compare with ViewZ (negative near clip) instead of 0.
		// Handle the cases of the view being within near clip distance
		if( Point.z > ViewZ )
		{
			NumPointsInFrontOfView++;
		}
		else
		{
			NumPointsBehindView++;
		}

		// Don't divide Z by W, it's more useful to keep linear depth
		Point.x *= RecpW;
		Point.y *= RecpW;
	}

	if( NumPointsBehindView == 4 )
	{
		// The portal is fully behind the view plane. Reject portal.
		return false;
	}

	// Avoiding an Array for allocation perf
	Vector2 BoundPoints[8];
	uint	NumBoundPoints	= 0;

	if( NumPointsInFrontOfView == 4 )
	{
		// No need to clip the portal against the view plane.
		// all points are in front so we'll just get their bounding box.
		NumBoundPoints = 4;
		for( uint Index = 0; Index < 4; ++Index )
		{
			BoundPoints[ Index ] = Points[ Index ];
		}
	}
	else
	{
		// Some points are behind the view. We need to clip the edges
		// to the view so we get the proper bounds of the portal.

		uint Edges[4][2];
		Edges[0][0] = 0; Edges[0][1] = 1;
		Edges[1][0] = 1; Edges[1][1] = 3;
		Edges[2][0] = 3; Edges[2][1] = 2;
		Edges[3][0] = 2; Edges[3][1] = 0;

		for( uint EdgeIndex = 0; EdgeIndex < 4; ++EdgeIndex )
		{
			const uint Edge0 = Edges[ EdgeIndex ][0];
			const uint Edge1 = Edges[ EdgeIndex ][1];

			const Vector4& Point0 = Points[ Edge0 ];
			const Vector4& Point1 = Points[ Edge1 ];

			const Vector& Corner0 = Portal.m_Corners[ Edge0 ];
			const Vector& Corner1 = Portal.m_Corners[ Edge1 ];

			if( Point0.z <= ViewZ && Point1.z <= ViewZ )
			{
				// Edge is behind view plane, ignore it
				DoNothing;
			}
			else if( Point0.z > ViewZ && Point1.z > ViewZ )
			{
				// Edge is in front of view plane, add both bound points
				BoundPoints[ NumBoundPoints++ ] = Point0;	// BoundPoints.PushBack( Point0 );
				BoundPoints[ NumBoundPoints++ ] = Point1;	// BoundPoints.PushBack( Point1 );
			}
			else
			{
				// ROSANOTE: Edge clipping, take 2. Clip against view plane in world space instead of screen space.
				// This is doing a Segment::Intersects( Plane ), essentially.
				const float		PlaneDistance	= ViewPlane.m_Normal.Dot( Corner0 ) + ViewPlane.m_Distance;
				const Vector	SegmentOffset	= Corner1 - Corner0;
				const float		SegmentDistance	= ViewPlane.m_Normal.Dot( SegmentOffset );
				const float		SegmentT		= -PlaneDistance / SegmentDistance;
				const Vector4	IntersectionWS	= Corner0 + SegmentT * SegmentOffset;

				Vector4			IntersectionSS	= IntersectionWS * ViewProjMat;
				IntersectionSS.x /= Abs( IntersectionSS.w );
				IntersectionSS.y /= Abs( IntersectionSS.w );

				const Vector2&	FrontPoint		= ( Point0.z > ViewZ ) ? Point0 : Point1;
				BoundPoints[ NumBoundPoints++ ] = FrontPoint;		// BoundPoints.PushBack( FrontPoint );
				BoundPoints[ NumBoundPoints++ ] = IntersectionSS;	// BoundPoints.PushBack( IntersectionSS );
			}
		}
	}

	DEBUGASSERT( NumBoundPoints > 0 );
	const Vector2& FirstPoint = BoundPoints[0];
	ClippedBound.m_Left		= Clamp( FirstPoint.x, ViewBound.m_Left,	ViewBound.m_Right );
	ClippedBound.m_Right	= Clamp( FirstPoint.x, ViewBound.m_Left,	ViewBound.m_Right );
	ClippedBound.m_Bottom	= Clamp( FirstPoint.y, ViewBound.m_Bottom,	ViewBound.m_Top );
	ClippedBound.m_Top		= Clamp( FirstPoint.y, ViewBound.m_Bottom,	ViewBound.m_Top );
	for( uint Index = 1; Index < NumBoundPoints; ++Index )
	{
		const Vector2& BoundPoint = BoundPoints[ Index ];
		ClippedBound.m_Left		= Clamp( Min( BoundPoint.x, ClippedBound.m_Left ),		ViewBound.m_Left,	ViewBound.m_Right );
		ClippedBound.m_Right	= Clamp( Max( BoundPoint.x, ClippedBound.m_Right ),		ViewBound.m_Left,	ViewBound.m_Right );
		ClippedBound.m_Bottom	= Clamp( Min( BoundPoint.y, ClippedBound.m_Bottom ),	ViewBound.m_Bottom,	ViewBound.m_Top );
		ClippedBound.m_Top		= Clamp( Max( BoundPoint.y, ClippedBound.m_Top ),		ViewBound.m_Bottom,	ViewBound.m_Top );
	}

	DEBUGASSERT( ClippedBound.m_Left	<= ClippedBound.m_Right );
	DEBUGASSERT( ClippedBound.m_Bottom	<= ClippedBound.m_Top );
	if( Abs( ClippedBound.m_Right - ClippedBound.m_Left ) < EPSILON ||
		Abs( ClippedBound.m_Top	- ClippedBound.m_Bottom  ) < EPSILON )
	{
		// View rectangle has been clipped to zero
		return false;
	}

	return true;
}

void RosaWorld::GatherVisibleSectors( const View& Camera, const Plane& ViewPlane, const uint SectorIndex, const SRect& ViewBound, Array<uint>& OutVisibleSectors, const uint Depth ) const
{
	OutVisibleSectors.PushBackUnique( SectorIndex );

#if BUILD_DEV
	static uint MaxDepth = 0;
	if( Depth > MaxDepth )
	{
		MaxDepth = Depth;
		PRINTF( "GatherVisibleSectors: MaxDepth: %d\n", MaxDepth );
	}
#endif

	// HACKHACK to avoid endless recursion
	static const uint skMaxDepth = 30;
	if( Depth == skMaxDepth )
	{
		return;
	}

	const Vector&	ViewLocation	= Camera.GetLocation();
	const Matrix	ViewProjMat		= Camera.GetViewProjectionMatrix();
	const SSector&	Sector			= m_Sectors[ SectorIndex ];

	FOR_EACH_ARRAY( PortalIter, Sector.m_Portals, SSectorPortal )
	{
		const SSectorPortal& Portal = PortalIter.GetValue();

		if( Portal.m_FrontPlane.DistanceTo( ViewLocation ) < 0.0f )
		{
			// Camera is on back side of portal, we don't care
			continue;
		}

		SRect ClippedBound;
		if( !ClipPortal( Portal, Camera, ViewPlane, ViewProjMat, ViewBound, ClippedBound ) )
		{
			continue;
		}

#if BUILD_DEV && 1
		// HACKHACK: Can't do this in DebugRender() unless we store all of this, and nooo.

		STATICHASH( RosaWorld );
		STATICHASH( DebugRenderPortalRecursion );
		const bool DebugRenderPortalRecursion	= ConfigManager::GetBool( sDebugRenderPortalRecursion, false, sRosaWorld );

		if( DebugRenderPortalRecursion )
		{
			STATICHASH( DisplayWidth );
			STATICHASH( DisplayHeight );
			const float DisplayWidth	= static_cast<float>( ConfigManager::GetInt( sDisplayWidth ) - 1 );
			const float DisplayHeight	= static_cast<float>( ConfigManager::GetInt( sDisplayHeight ) - 1 );
			m_Framework->GetRenderer()->DEBUGDrawBox2D(
				Vector( ( ClippedBound.m_Left + 1.0f ) * 0.5f * DisplayWidth, 0.0f, ( -ClippedBound.m_Top + 1.0f ) * 0.5f * DisplayHeight ),
				Vector( ( ClippedBound.m_Right + 1.0f ) * 0.5f * DisplayWidth, 0.0f, ( -ClippedBound.m_Bottom + 1.0f ) * 0.5f * DisplayHeight ),
				ARGB_TO_COLOR( 255, 255, 0, 0 ) );
			m_Framework->GetRenderer()->DEBUGDrawTriangle( Portal.m_Corners[0],	Portal.m_Corners[1],	Portal.m_Corners[2],	ARGB_TO_COLOR( 255, 255, 0, 0 ) );
			m_Framework->GetRenderer()->DEBUGDrawTriangle( Portal.m_Corners[1],	Portal.m_Corners[3],	Portal.m_Corners[2],	ARGB_TO_COLOR( 255, 255, 0, 0 ) );
		}
#endif

		// All checks passed, gather through the portal
		GatherVisibleSectors( Camera, ViewPlane, Portal.m_BackSector, ClippedBound, OutVisibleSectors, Depth + 1 );
	}
}

#if BUILD_DEV
void RosaWorld::DebugRender() const
{
	IRenderer* const pRenderer = m_Framework->GetRenderer();
	Unused( pRenderer );

	STATICHASH( RosaWorld );

	STATICHASH( DebugRenderSectors );
	const bool DebugRenderSectors		= ConfigManager::GetBool( sDebugRenderSectors, false, sRosaWorld );

	STATICHASH( DebugRenderPortals );
	const bool DebugRenderPortals		= ConfigManager::GetBool( sDebugRenderPortals, false, sRosaWorld );

	STATICHASH( DebugRenderNavMesh );
	const bool DebugRenderNavMesh		= ConfigManager::GetBool( sDebugRenderNavMesh, false, sRosaWorld );

	STATICHASH( DebugRenderNavMeshInner );
	const bool DebugRenderNavMeshInner	= ConfigManager::GetBool( sDebugRenderNavMeshInner, false, sRosaWorld );

	STATICHASH( DebugRenderNavMeshHeight );
	const bool DebugRenderNavMeshHeight	= ConfigManager::GetBool( sDebugRenderNavMeshHeight, false, sRosaWorld );

	static const uint	kSectorRenderColor		= ARGB_TO_COLOR( 255, 255, 255, 0 );
	static const uint	kSectorCollisionColor	= ARGB_TO_COLOR( 255, 0, 255, 255 );
	static const uint	kPortalColor			= ARGB_TO_COLOR( 255, 0, 255, 0 );
	static const uint	kNavMeshInnerColor		= ARGB_TO_COLOR( 255, 0, 255, 255 );
	static const uint	kNavMeshOuterColor		= ARGB_TO_COLOR( 255, 255, 255, 0 );
	static const uint	kNavMeshHeightColor		= ARGB_TO_COLOR( 255, 255, 0, 255 );

	// Draw sectors and portals
	if( DebugRenderSectors || DebugRenderPortals )
	{
		FOR_EACH_ARRAY( SectorIter, m_Sectors, SSector )
		{
			const SSector& Sector = SectorIter.GetValue();

			if( DebugRenderSectors )
			{
				pRenderer->DEBUGDrawBox( Sector.m_RenderBound.m_Min,	Sector.m_RenderBound.m_Max,		kSectorRenderColor );
				pRenderer->DEBUGDrawBox( Sector.m_CollisionBound.m_Min,	Sector.m_CollisionBound.m_Max,	kSectorCollisionColor );
			}

			if( DebugRenderPortals )
			{
				FOR_EACH_ARRAY( PortalIter, Sector.m_Portals, SSectorPortal )
				{
					const SSectorPortal& Portal = PortalIter.GetValue();
					pRenderer->DEBUGDrawBox( Portal.m_Corners[0],		Portal.m_Corners[3],			kPortalColor );
				}
			}
		}
	}

	// Draw navmesh
	if( DebugRenderNavMesh )
	{
		FOR_EACH_ARRAY( NavNodeIter, m_NavNodes, SNavNode )
		{
			const SNavNode&		NavNode		= NavNodeIter.GetValue();
			const SNavEdge&		NavEdgeA	= m_NavEdges[ NavNode.m_EdgeA ];
			const SNavEdge&		NavEdgeB	= m_NavEdges[ NavNode.m_EdgeB ];
			const SNavEdge&		NavEdgeC	= m_NavEdges[ NavNode.m_EdgeC ];

			if( DebugRenderNavMeshInner || NavEdgeA.m_BackNode == NAV_NULL )
			{
				pRenderer->DEBUGDrawLine( NavEdgeA.m_VertA, NavEdgeA.m_VertB, ( NavEdgeA.m_BackNode == NAV_NULL ) ? kNavMeshOuterColor : kNavMeshInnerColor );
			}

			if( DebugRenderNavMeshInner || NavEdgeB.m_BackNode == NAV_NULL )
			{
				pRenderer->DEBUGDrawLine( NavEdgeB.m_VertA, NavEdgeB.m_VertB, ( NavEdgeB.m_BackNode == NAV_NULL ) ? kNavMeshOuterColor : kNavMeshInnerColor );
			}

			if( DebugRenderNavMeshInner || NavEdgeC.m_BackNode == NAV_NULL )
			{
				pRenderer->DEBUGDrawLine( NavEdgeC.m_VertA, NavEdgeC.m_VertB, ( NavEdgeC.m_BackNode == NAV_NULL ) ? kNavMeshOuterColor : kNavMeshInnerColor );
			}

			if( DebugRenderNavMeshHeight )
			{
				const Angles	UpAngles	= Angles( HALFPI, 0.0f, 0.0f );
				pRenderer->DEBUGDrawArrow( NavNode.m_Centroid, UpAngles, NavNode.m_Height, kNavMeshHeightColor );
			}
		}
	}
}
#endif

bool RosaWorld::Sweep( const Segment& SweepSegment, const Vector& HalfExtents, CollisionInfo& Info ) const
{
	CollisionInfo MinInfo;

	DEVASSERT( Info.m_In_CollideWorld || Info.m_In_CollideEntities );	// We need to be colliding with something
	DEVASSERT( 0 != ( Info.m_In_UserFlags & EECF_Mask_CollideAs ) );	// We need to be colliding as some type (this didn't used to be true for CollideWorld)

	Vector MinCorner;
	Vector MaxCorner;
	Vector::MinMax( SweepSegment.m_Point1, SweepSegment.m_Point2, MinCorner, MaxCorner );
	// Segment box is expanded by half extents for testing against un-expanded collision bounds
	const AABB SegmentBox = AABB( MinCorner - HalfExtents, MaxCorner + HalfExtents );

	if( Info.m_In_CollideWorld )
	{
		CollisionInfo HullInfo;
		HullInfo.CopyInParametersFrom( Info );
		if( SweepHulls( SweepSegment, HalfExtents, SegmentBox, HullInfo ) )
		{
			MinInfo.CopyOutParametersFrom( HullInfo );
		}
	}

	// ROSANOTE: I'm not sweeping entities through sectors, because they already
	// have an acceleration structure (the collision type arrays) and I'm not
	// going to give that up. Possible option would be to migrate those arrays
	// into sectors, but that would get pretty messy.
	if( Info.m_In_CollideEntities )
	{
		CollisionInfo EntitiesInfo;
		EntitiesInfo.CopyInParametersFrom( Info );
		if( SweepEntities( SweepSegment, HalfExtents, SegmentBox, EntitiesInfo ) )
		{
			if( EntitiesInfo.m_Out_HitT < MinInfo.m_Out_HitT || !MinInfo.m_Out_Collision )
			{
				MinInfo.CopyOutParametersFrom( EntitiesInfo );
			}
		}
	}

	Info.CopyOutParametersFrom( MinInfo );
	return Info.m_Out_Collision;
}

bool RosaWorld::SweepHulls( const Segment& SweepSegment, const Vector& HalfExtents, const AABB& SegmentBox, CollisionInfo& Info ) const
{
	DEVASSERT( 0 != ( Info.m_In_UserFlags & EECF_Mask_CollideAs ) ); // We need to be colliding as some type

	CollisionInfo MinInfo;
	CollisionInfo CheckInfo;

	const Vector&	SweepStart	= SweepSegment.m_Point1;
	const Vector	SweepTravel	= SweepSegment.m_Point2 - SweepSegment.m_Point1;

	bool		ContinueSweep	= true;
	const uint	NumSectors		= m_Sectors.Size();
	for( uint SectorIndex = 0; SectorIndex < NumSectors && ContinueSweep; ++SectorIndex )
	{
		const SSector&				Sector		= m_Sectors[ SectorIndex ];

		if( !SegmentBox.Intersects( Sector.m_CollisionBound ) )
		{
			continue;
		}

		const Array<SConvexHull>&	Hulls		= Sector.m_Hulls;
		const uint					NumHulls	= Hulls.Size();
		for( uint HullIndex = 0; HullIndex < NumHulls && ContinueSweep; ++HullIndex )
		{
			const SConvexHull& Hull = Hulls[ HullIndex ];

			const uint MatchedCollisionFlags	= ( Hull.m_CollisionFlags & Info.m_In_UserFlags );
			if( 0 == MatchedCollisionFlags )
			{
				continue;
			}

			if( !SegmentBox.Intersects( Hull.m_Bounds ) )
			{
				continue;
			}

			CheckInfo.m_Out_Collision = false;
			if( !Hull.m_Hull.Sweep( SweepStart, HalfExtents, SweepTravel, &CheckInfo ) )
			{
				continue;
			}

			// Hit!
			CheckInfo.m_Out_UserFlags = Hull.m_Surface;	// ROSAHACK: I only ever used OutUserFlags for surfaces in Neon, so that's all I'm doing here.
			if( CheckInfo.m_Out_HitT < MinInfo.m_Out_HitT || !MinInfo.m_Out_Collision )
			{
				MinInfo = CheckInfo;
				ContinueSweep = !Info.m_In_StopAtAnyCollision;
			}
		}
	}

	Info.CopyOutParametersFrom( MinInfo );
	return Info.m_Out_Collision;
}

bool RosaWorld::SweepEntities( const Segment& SweepSegment, const Vector& HalfExtents, const AABB& SegmentBox, CollisionInfo& Info ) const
{
	// This gets called enough that the profiler hook is a cost. >_<
	//PROFILE_FUNCTION;

	const uint	CollideAsType	= Info.m_In_UserFlags & EECF_Mask_CollideAs;
	const bool	CollideAll		= ( Info.m_In_UserFlags & EECF_CollideAllEntities ) > 0;

	DEVASSERT( 0 != ( CollideAsType ) );								// We need to be colliding as some type
	DEVASSERT( 1 == CountBits( CollideAsType ) );						// We should only be colliding as one type (optimization; can be changed, see WBCompRosaCollision::AddToCollisionMap)
	DEVASSERT( 0 != ( Info.m_In_UserFlags & EECF_Mask_EntityTypes ) );	// We need to be colliding against some type of entity

	const Array<WBCompRosaCollision*>* const pCollisionComponents =
		CollideAll ?
		WBComponentArrays::GetComponents<WBCompRosaCollision>() :
		WBCompRosaCollision::GetCollisionArray( CollideAsType );

	if( !pCollisionComponents )
	{
		return false;
	}

	// Not sure this aliasing is any faster than dereferencing a const pointer.
	// Probably not. But it makes me feel better.
	const Array<WBCompRosaCollision*>& CollisionComponents = *pCollisionComponents;

	CollisionInfo MinInfo;
	CollisionInfo CheckInfo;

	WBEntity* const pCollidingTopmostOwner = WBCompOwner::GetTopmostOwner( static_cast<WBEntity*>( Info.m_In_CollidingEntity ) );

	bool ContinueSweep = true;
	const uint NumCollisionComponents = CollisionComponents.Size();
	for( uint CollisionComponentIndex = 0; CollisionComponentIndex < NumCollisionComponents && ContinueSweep; ++CollisionComponentIndex )
	{
		WBCompRosaCollision* const pCollision = CollisionComponents[ CollisionComponentIndex ];
		DEVASSERT( pCollision );

		const uint CollisionFlags		= pCollision->GetCollisionFlags();
		const uint MatchedFlags			= ( CollisionFlags & Info.m_In_UserFlags );
		const uint MatchedEntityFlags	= MatchedFlags & EECF_Mask_EntityTypes;

#if BUILD_DEV
		if( !CollideAll )
		{
			const uint MatchedCollisionFlags = MatchedFlags & EECF_Mask_CollideAs;
			ASSERT( MatchedCollisionFlags != 0 );
		}
#endif

		if( 0 == MatchedEntityFlags )
		{
			continue;
		}

		WBEntity* const pEntity = pCollision->GetEntity();
		DEVASSERT( pEntity );

		if( pEntity == Info.m_In_CollidingEntity )
		{
			continue;
		}

		if( pEntity->IsDestroyed() )
		{
			continue;
		}

		// Don't collide if entities share a topmost owner
		if( WBCompOwner::GetTopmostOwner( pEntity ) == pCollidingTopmostOwner )
		{
			continue;
		}

		const bool				CollideBones	= ( ( Info.m_In_UserFlags & EECF_CollideBones ) > 0 );
		DEVASSERT( !CollideBones || HalfExtents.IsZero() );	// We shouldn't be using CollideBones on non-zero-extent checks
		WBCompRosaHitbox* const	pHitbox			= CollideBones ? WB_GETCOMP( pEntity, RosaHitbox ) : NULL;
		if( pHitbox && pHitbox->HasHitbox() )
		{
			// HACKHACK: Test against skeletal hitbox in special cases

			// Make sure hitbox is updated this frame
			pHitbox->UpdateHitbox();
			if( !SegmentBox.Intersects( pHitbox->GetHitboxBounds() ) )
			{
				continue;
			}

			const Array<SHitboxBone>& HitboxBones = pHitbox->GetHitboxBones();
			FOR_EACH_ARRAY( HitboxBoneIter, HitboxBones, SHitboxBone )
			{
				const SHitboxBone& HitboxBone = HitboxBoneIter.GetValue();

				Vector	NearestPointA;
				Vector	NearestPointB;
				float	T;
				SweepSegment.NearestPointTo( HitboxBone.m_BoneSegment, NearestPointA, NearestPointB, &T, NULL );

				const Vector	Offset	= NearestPointB - NearestPointA;
				const float		DistSq	= Offset.LengthSquared();

				if( DistSq > HitboxBone.m_BoneWidthSq )
				{
					continue;
				}

				// Hit!
				CheckInfo.m_Out_Collision		= true;
				CheckInfo.m_Out_HitT			= T;
				CheckInfo.m_Out_Intersection	= NearestPointA;			// ROSANOTE: This will be inside the bone's width, not on its "surface"
				CheckInfo.m_Out_Plane			= Plane( -Offset.GetFastNormalized(), NearestPointA );
				CheckInfo.m_Out_HitEntity		= pEntity;
				CheckInfo.m_Out_HitName			= HitboxBone.m_BoneName;	// Send back the hit bone's name for headshots, etc.

				if( CheckInfo.m_Out_HitT < MinInfo.m_Out_HitT || !MinInfo.m_Out_Collision )
				{
					MinInfo = CheckInfo;
					ContinueSweep = !Info.m_In_StopAtAnyCollision;
				}

				// Stop after first hit; bones should be priority sorted if I care about that.
				break;
			}
		}
		else
		{
			// Normal collision box test

			const AABB& CollisionBox = pCollision->GetBounds();
			if( !SegmentBox.Intersects( CollisionBox ) )
			{
				continue;
			}

			// After expanded-segment-bounds vs. unexpanded-collision-bounds test,
			// expand the collision bounds for tracing the segment.
			AABB ExpandedCollisionBox = CollisionBox;
			ExpandedCollisionBox.ExpandBy( HalfExtents );

			CheckInfo.m_Out_Collision = false;
			if( !SweepSegment.Intersects( ExpandedCollisionBox, &CheckInfo ) )
			{
				continue;
			}

			// Hit!
			CheckInfo.m_Out_HitEntity = pEntity;
			if( CheckInfo.m_Out_HitT < MinInfo.m_Out_HitT || !MinInfo.m_Out_Collision )
			{
				MinInfo = CheckInfo;
				ContinueSweep = !Info.m_In_StopAtAnyCollision;
			}
		}
	}

	Info.CopyOutParametersFrom( MinInfo );
	return Info.m_Out_Collision;
}

bool RosaWorld::Trace( const Ray& TraceRay, CollisionInfo& Info ) const
{
	// HACK: So that everything is just implemented via a segment sweep, use a fixed trace length.
	// This is set (in ::InitializeConfig) to a size big enough to cross the entire world at its
	// furthest corners, so it should be sufficient.
	const Segment TraceSegment( TraceRay.m_Point, TraceRay.m_Point + TraceRay.m_Direction * GetRayTraceLength() );

	// Since a sweep is just a trace against expanded boxes, a trace is just a sweep with zero extents!
	return Sweep( TraceSegment, Vector(), Info );
}

bool RosaWorld::Sweep( const Ray& TraceRay, const Vector& HalfExtents, CollisionInfo& Info ) const
{
	// HACK: So that everything is just implemented via a segment sweep, use a fixed trace length.
	// This is set (in ::InitializeConfig) to a size big enough to cross the entire world at its
	// furthest corners, so it should be sufficient.
	const Segment TraceSegment( TraceRay.m_Point, TraceRay.m_Point + TraceRay.m_Direction * GetRayTraceLength() );

	return Sweep( TraceSegment, HalfExtents, Info );
}

bool RosaWorld::Trace( const Segment& TraceSegment, CollisionInfo& Info ) const
{
	// Since a sweep is just a trace against expanded boxes, a trace is just a sweep with zero extents!
	return Sweep( TraceSegment, Vector(), Info );
}

bool RosaWorld::CheckClearance( const Vector& Location, const Vector& HalfExtents, CollisionInfo& Info ) const
{
	// A zero length sweep should work fine for a clearance check
	const Segment TraceSegment( Location, Location );
	return Sweep( TraceSegment, HalfExtents, Info );
}

bool RosaWorld::PointCheck( const Vector& Location, CollisionInfo& Info ) const
{
	// A zero length, zero extent sweep should work fine for a point check
	const Segment TraceSegment( Location, Location );
	return Sweep( TraceSegment, Vector(), Info );
}

bool RosaWorld::LineCheck( const Vector& Start, const Vector& End, CollisionInfo& Info ) const
{
	const Segment TraceSegment( Start, End );
	return Sweep( TraceSegment, Vector(), Info );
}

bool RosaWorld::SweepCheck( const Vector& Start, const Vector& End, const Vector& HalfExtents, CollisionInfo& Info ) const
{
	const Segment TraceSegment( Start, End );
	return Sweep( TraceSegment, HalfExtents, Info );
}

bool RosaWorld::FindSpot( Vector& InOutSpot, const Vector& Extents, CollisionInfo& Info ) const
{
	PROFILE_FUNCTION;

	// HACKHACK: If we have a zero-extents entity, just use the given spot.
	// This fixes some behavior that got broken by the cardinal direction check below.
	if( Extents.IsZero() )
	{
		return true;
	}

	// First, see if the given spot is ok.
	if( !CheckClearance( InOutSpot, Extents, Info ) )
	{
		return true;
	}

	// Given spot is not ok; we need to find a spot.

	// Check if the zero extents spot is usable.
	if( PointCheck( InOutSpot, Info ) )
	{
		// If it's not, try moving by 1m in each cardinal direction.
		const Vector UnitX( 1.0f, 0.0f, 0.0f );
		const Vector UnitY( 0.0f, 1.0f, 0.0f );
		const Vector UnitZ( 0.0f, 0.0f, 1.0f );

		if( !PointCheck( InOutSpot + UnitX, Info ) )		{ InOutSpot += UnitX; }
		else if( !PointCheck( InOutSpot - UnitX, Info ) )	{ InOutSpot -= UnitX; }
		else if( !PointCheck( InOutSpot + UnitY, Info ) )	{ InOutSpot += UnitY; }
		else if( !PointCheck( InOutSpot - UnitY, Info ) )	{ InOutSpot -= UnitY; }
		else if( !PointCheck( InOutSpot + UnitZ, Info ) )	{ InOutSpot += UnitZ; }
		else if( !PointCheck( InOutSpot - UnitZ, Info ) )	{ InOutSpot -= UnitZ; }
		else
		{
			// Moving in cardinal directions didn't help. For now, just bail out and don't try anymore.
			return false;
		}
	}

	// Zero extents spot is good.
	// Push InOutSpot away from nearby surfaces along each axis.
	for( uint Dir = 0; Dir < 3; ++Dir )
	{
		static const float kSmallDistance = 0.1f;

		Vector TraceDir;
		TraceDir.v[ Dir ] = Extents.v[ Dir ] + kSmallDistance;

		if( LineCheck( InOutSpot, InOutSpot + TraceDir, Info ) )
		{
			InOutSpot = Info.m_Out_Intersection - TraceDir;
		}
		// Using "else" here because there's no need to check the second direction
		// if the first collides; if there's collision there, we can't resolve it.
		else if( LineCheck( InOutSpot, InOutSpot - TraceDir, Info ) )
		{
			InOutSpot = Info.m_Out_Intersection + TraceDir;
		}
	}

	// Now we know that each axis (like an "axial caltrop") fits or doesn't
	// We might be able to fit now, by pushing away from occlusions directly along each axis.
	if( !CheckClearance( InOutSpot, Extents, Info ) )
	{
		return true;
	}

	// If not, then we need to sweep a line segment along each axis to find more complex intersections.
	for( uint Dir = 0; Dir < 3; ++Dir )
	{
		const uint NextDir = ( Dir + 1 ) % 3;
		static const float kSmallDistance = 0.1f;

		Vector TraceDir;
		TraceDir.v[ Dir ] = Extents.v[ Dir ] + kSmallDistance;

		Vector LineExtents;
		LineExtents.v[ NextDir ] = Extents.v[ NextDir ];

		if( SweepCheck( InOutSpot, InOutSpot + TraceDir, LineExtents, Info ) )
		{
			InOutSpot = Info.m_Out_Intersection - TraceDir;
		}
		// Using "else" here because there's no need to check the second direction
		// if the first collides; if there's collision there, we can't resolve it.
		else if( SweepCheck( InOutSpot, InOutSpot - TraceDir, LineExtents, Info ) )
		{
			InOutSpot = Info.m_Out_Intersection + TraceDir;
		}
	}

	// Now we know that each planar section fits or doesn't.
	// Try again to check for final clearance before continuing.
	if( !CheckClearance( InOutSpot, Extents, Info ) )
	{
		return true;
	}

	// Finally, we need to sweep a rectangle along each axis to find more complex intersections.
	for( uint Dir = 0; Dir < 3; ++Dir )
	{
		static const float kSmallDistance = 0.1f;

		Vector TraceDir;
		TraceDir.v[ Dir ] = Extents.v[ Dir ] + kSmallDistance;

		Vector PlaneExtents = Extents;
		PlaneExtents.v[ Dir ] = 0.0f;

		if( SweepCheck( InOutSpot, InOutSpot + TraceDir, PlaneExtents, Info ) )
		{
			InOutSpot = Info.m_Out_Intersection - TraceDir;
		}
		// Using "else" here because there's no need to check the second direction
		// if the first collides; if there's collision there, we can't resolve it.
		else if( SweepCheck( InOutSpot, InOutSpot - TraceDir, PlaneExtents, Info ) )
		{
			InOutSpot = Info.m_Out_Intersection + TraceDir;
		}
	}

	// Now, the whole volume fits, or doesn't. Check final clearance.
	return !CheckClearance( InOutSpot, Extents, Info );
}

#define VERSION_EMPTY						0
#define VERSION_BASE						1
#define VERSION_COALESCEDPORTALS			2
#define VERSION_VISITEDSECTORS				3
#define VERSION_MODULE_ISSINGLETILE			4
#define VERSION_MODULE_ISSINGLETILE_DEPR	5
#define VERSION_CURRENT						5

void RosaWorld::Save( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	PROFILE_FUNCTION;

	Stream.WriteUInt32( VERSION_CURRENT );

	// Write world def
	Stream.WriteString( m_CurrentWorldDef );

	// Write modules
	const Array<RosaWorldGen::SModule>& Modules = GetCurrentWorldDef().m_WorldGen->GetModules();
	Stream.WriteUInt32( Modules.Size() );
	FOR_EACH_ARRAY( ModuleIter, Modules, RosaWorldGen::SModule )
	{
		const RosaWorldGen::SModule& Module = ModuleIter.GetValue();
		Stream.WriteString( Module.m_Filename );
		Stream.Write<RosaWorldGen::SRoomLoc>( Module.m_Location );
		Stream.WriteUInt32( Module.m_Transform );
		Stream.WriteUInt32( Module.m_Portals.Size() );
		FOR_EACH_ARRAY( PortalIter, Module.m_Portals, RosaWorldGen::SSectorPortal )
		{
			const RosaWorldGen::SSectorPortal& Portal = PortalIter.GetValue();
			Stream.Write<RosaWorldGen::SRoomLoc>( Portal.m_LocationLo );
			Stream.Write<RosaWorldGen::SRoomLoc>( Portal.m_LocationHi );
			Stream.WriteUInt32( Portal.m_PortalIndex );
			Stream.WriteUInt32( Portal.m_BackSector );
		}
	}

	// Write current time
	const float CurrentTime = WBWorld::GetInstance()->GetTime();
	Stream.WriteFloat( CurrentTime );

	// Write visited sectors
	Stream.WriteArray( m_VisitedSectors );

	WBWorld::GetInstance()->Save( Stream );
}

void RosaWorld::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	PROFILE_FUNCTION;

	// Shut down in advance, because it may affect the world.
	WBWorld::GetInstance()->ShutDown();

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_BASE )
	{
		SetCurrentWorld( Stream.ReadString() );
	}

	Array<RosaWorldGen::SModule> Modules;
	if( Version >= VERSION_BASE )
	{
		const uint NumModules = Stream.ReadUInt32();
		Modules.Reserve( NumModules );
		for( uint ModuleIndex = 0; ModuleIndex < NumModules; ++ModuleIndex )
		{
			RosaWorldGen::SModule& Module = Modules.PushBack();
			Module.m_Filename	= Stream.ReadString();
			Module.m_Location	= Stream.Read<RosaWorldGen::SRoomLoc>();
			Module.m_Transform	= Stream.ReadUInt32();

			const uint NumPortals = Stream.ReadUInt32();
			Module.m_Portals.Resize( NumPortals );
			for( uint PortalIndex = 0; PortalIndex < NumPortals; ++PortalIndex )
			{
				RosaWorldGen::SSectorPortal& Portal = Module.m_Portals[ PortalIndex ];
				Portal.m_LocationLo		= Stream.Read<RosaWorldGen::SRoomLoc>();
				Portal.m_LocationHi		= ( Version >= VERSION_COALESCEDPORTALS ) ? Stream.Read<RosaWorldGen::SRoomLoc>() : Portal.m_LocationLo;
				Portal.m_PortalIndex	= Stream.ReadUInt32();
				Portal.m_BackSector		= Stream.ReadUInt32();
			}
			if( Version >= VERSION_MODULE_ISSINGLETILE && Version < VERSION_MODULE_ISSINGLETILE_DEPR )
			{
				Stream.ReadBool();
			}
		}
	}

	float GameTime = 0.0f;
	if( Version >= VERSION_BASE )
	{
		GameTime = Stream.ReadFloat();
	}

	m_VisitedSectors.Clear();
	if( Version >= VERSION_VISITEDSECTORS )
	{
		Stream.ReadArray( m_VisitedSectors );
	}

	PublishWorldProperties();

	WBWorld::GetInstance()->SetTime( GameTime );

	DeleteWorldGeo();
	GetCurrentWorldDef().m_WorldGen->GenerateFromModules( Modules );

	WBWorld::GetInstance()->Load( Stream );

	// Tick once to pump event queue
	WBWorld::GetInstance()->Tick( 0.0f );
}

const RosaWorld::SWorldDef& RosaWorld::GetCurrentWorldDef() const
{
	return m_WorldDef;
}

void RosaWorld::GatherStats()
{
	const SWorldDef& WorldDef = GetCurrentWorldDef();
	ASSERT( WorldDef.m_WorldGen );

	WorldDef.m_WorldGen->GatherStats();
}

SimpleString RosaWorld::GetSpawnerOverride( const SimpleString& OldSpawner ) const
{
	const SWorldDef& WorldDef = GetCurrentWorldDef();
	SWorldDef::TSpawnerMap::Iterator SpawnerIter = WorldDef.m_SpawnerOverrides.Search( OldSpawner );

	if( SpawnerIter.IsValid() )
	{
		return SpawnerIter.GetValue();
	}
	else
	{
		return OldSpawner;
	}
}

HashedString RosaWorld::GetCollisionSurface( const CollisionInfo& Info ) const
{
	DEVASSERT( Info.m_Out_Collision );

	if( Info.m_Out_HitEntity )
	{
		WBEntity* const				pEntity		= static_cast<WBEntity*>( Info.m_Out_HitEntity );
		WBCompRosaCollision* const	pCollision	= WB_GETCOMP( pEntity, RosaCollision );
		DEVASSERT( pCollision );

		return pCollision->GetSurface();
	}
	else
	{
		return Info.m_Out_UserFlags;
	}
}

HashedString RosaWorld::GetSurfaceBelowPoint( const Vector& Location, WBEntity* const pSourceEntity ) const
{
	CollisionInfo Info;
	Info.m_In_CollideWorld		= true;
	Info.m_In_CollideEntities	= true;
	Info.m_In_CollidingEntity	= pSourceEntity;
	Info.m_In_UserFlags			= EECF_BlockerCollision;

	Ray TraceRay( Location, Vector( 0.0f, 0.0f, -1.0f ) );

	if( Trace( TraceRay, Info ) )
	{
		return GetCollisionSurface( Info );
	}
	else
	{
		// Didn't collide with anything below point. Possibly a problem?
		WARNDESC( "RosaWorld::GetSurfaceBelowPoint: No collision for trace" );
		return HashedString::NullString;
	}
}

HashedString RosaWorld::GetSurfaceBelow( const Vector& Location, const Vector& Extents, WBEntity* const pSourceEntity ) const
{
	// First, try the surface directly below the location (as in GetSurfaceBelowPoint).
	// Then do a sweep down, and use that instead if its hit time is less than the direct trace.
	// This should resolve some erroneous footsteps when entity is halfway off a ledge, but
	// the sweep could hit multiple surfaces, so we don't want to use it unless necessary.

	Ray		TraceRay( Location, Vector( 0.0f, 0.0f, -1.0f ) );
	Vector	SweepExtents( Extents.x, Extents.y, 0.0f );			// Sweep a flat quad, so we get the same hit time as the trace

	CollisionInfo TraceInfo;
	TraceInfo.m_In_CollideWorld		= true;
	TraceInfo.m_In_CollideEntities	= true;
	TraceInfo.m_In_CollidingEntity	= pSourceEntity;
	TraceInfo.m_In_UserFlags		= EECF_BlockerCollision;
	Trace( TraceRay, TraceInfo );

	CollisionInfo SweepInfo;
	SweepInfo.m_In_CollideWorld		= true;
	SweepInfo.m_In_CollideEntities	= true;
	SweepInfo.m_In_CollidingEntity	= pSourceEntity;
	SweepInfo.m_In_UserFlags		= EECF_BlockerCollision;
	Sweep( TraceRay, SweepExtents, SweepInfo );

	if( SweepInfo.m_Out_Collision && ( !TraceInfo.m_Out_Collision || SweepInfo.m_Out_HitT < TraceInfo.m_Out_HitT ) )
	{
		return GetCollisionSurface( SweepInfo );
	}
	else if( TraceInfo.m_Out_Collision )
	{
		return GetCollisionSurface( TraceInfo );
	}
	else
	{
		return HashedString::NullString;
	}
}

void RosaWorld::CreateGeoMesh( const SimpleString& MeshName, const SimpleString& MaterialName, const bool CastsShadows, const HashedString& Mat, const Vector& Location, const Angles& Orientation, const float Scale )
{
	if( MeshName == "" )
	{
		return;
	}

	IRenderer* const		pRenderer		= m_Framework->GetRenderer();
	TextureManager* const	pTextureManager	= pRenderer->GetTextureManager();

	MeshFactory::SReadMeshCallback Callback;
	Callback.m_Callback		= &RosaWorld::ReadMeshCallback_SaveBuffers;
	Callback.m_Void			= this;

	// Push this before accessing the mesh, it's used in the callback
	m_GeoMeshNames.PushBack( MeshName );

	Mesh* const pGeoMesh = new Mesh;
	pRenderer->GetMeshFactory()->GetDynamicMesh( MeshName.CStr(), pGeoMesh, Callback );

	const SimpleString UsingMaterialName = ( MaterialName == "" ) ? "Material_World" : MaterialName;
	pGeoMesh->SetMaterialDefinition( UsingMaterialName, pRenderer );

	const uint MaterialFlags	= CastsShadows ? ( MAT_WORLD | MAT_SHADOW ) : ( MAT_WORLD );
	pGeoMesh->SetMaterialFlags( MaterialFlags );

	m_GeoMeshMaterials.PushBack( UsingMaterialName );

	// NOTE: Mat should *only* be used for textures; anything else can
	// and should be done in RoomBaker instead of at runtime.
	if( Mat != HashedString::NullString )
	{
		STATICHASH( Albedo );
		const char* const pAlbedo = ConfigManager::GetInheritedString( sAlbedo, NULL, Mat );
		DEVASSERT( pAlbedo );
		pGeoMesh->SetTexture( 0, pTextureManager->GetTexture( pAlbedo ) );

		STATICHASH( Normal );
		const char* const pNormal = ConfigManager::GetInheritedString( sNormal, NULL, Mat );
		DEVASSERT( pNormal );
		pGeoMesh->SetTexture( 1, pTextureManager->GetTexture( pNormal ) );

		STATICHASH( Spec );
		const char* const pSpec = ConfigManager::GetInheritedString( sSpec, NULL, Mat );
		DEVASSERT( pSpec );
		pGeoMesh->SetTexture( 2, pTextureManager->GetTexture( pSpec ) );

		STATICHASH( Overlay );
		const char* const pOverlay = ConfigManager::GetInheritedString( sOverlay, NULL, Mat );
		if( pOverlay )
		{
			pGeoMesh->SetTexture( 3, pTextureManager->GetTexture( pOverlay ) );
		}

		STATICHASH( Flow );
		const char* const pFlow = ConfigManager::GetInheritedString( sFlow, NULL, Mat );
		if( pFlow )
		{
			DEVASSERT( NULL == pOverlay );	// I'm not currently supporting both. They're each taking slot 3.
			pGeoMesh->SetTexture( 3, pTextureManager->GetTexture( pFlow ) );
		}
	}

	pGeoMesh->m_Location	= Location;
	pGeoMesh->m_Rotation	= Orientation;
	pGeoMesh->m_Scale		= Vector( Scale, Scale, Scale );
	pGeoMesh->RecomputeAABB();

	SGeoMesh& GeoMesh = m_GeoMeshes.PushBack();
	GeoMesh.m_Mesh = pGeoMesh;
}

/*static*/ void RosaWorld::ReadMeshCallback_SaveBuffers( void* pVoid, const SReadMeshBuffers& Buffers )
{
	RosaWorld* const pThis = static_cast<RosaWorld*>( pVoid );
	pThis->SaveBuffers( Buffers );
}

void RosaWorld::SaveBuffers( const SReadMeshBuffers& Buffers )
{
	DEVASSERT( m_SavedGeoMeshBuffers.Search( m_GeoMeshNames.Last() ).IsNull() );
	const SimpleString&		GeoMeshName		= m_GeoMeshNames.Last();
	SSavedGeoMeshBuffers&	SavedBuffers	= m_SavedGeoMeshBuffers[ GeoMeshName ];

	SavedBuffers.m_Positions.Resize(	Buffers.m_Header.m_NumVertices );
	memcpy( SavedBuffers.m_Positions.GetData(), Buffers.m_Positions, SavedBuffers.m_Positions.MemorySize() );

	if( Buffers.m_Header.m_HasColors )
	{
		SavedBuffers.m_Colors.Resize(	Buffers.m_Header.m_NumVertices );
		memcpy( SavedBuffers.m_Colors.GetData(), Buffers.m_Colors, SavedBuffers.m_Colors.MemorySize() );
	}

	if( Buffers.m_Header.m_HasUVs )
	{
		SavedBuffers.m_UVs.Resize(		Buffers.m_Header.m_NumVertices );
		memcpy( SavedBuffers.m_UVs.GetData(), Buffers.m_UVs, SavedBuffers.m_UVs.MemorySize() );
	}

	if( Buffers.m_Header.m_HasNormals )
	{
		SavedBuffers.m_Normals.Resize(	Buffers.m_Header.m_NumVertices );
		memcpy( SavedBuffers.m_Normals.GetData(), Buffers.m_Normals, SavedBuffers.m_Normals.MemorySize() );
	}

	if( Buffers.m_Header.m_HasNormalsB )
	{
		SavedBuffers.m_NormalsB.Resize(	Buffers.m_Header.m_NumVertices );
		memcpy( SavedBuffers.m_NormalsB.GetData(), Buffers.m_NormalsB, SavedBuffers.m_NormalsB.MemorySize() );
	}

	if( Buffers.m_Header.m_HasTangents )
	{
		SavedBuffers.m_Tangents.Resize(	Buffers.m_Header.m_NumVertices );
		memcpy( SavedBuffers.m_Tangents.GetData(), Buffers.m_Tangents, SavedBuffers.m_Tangents.MemorySize() );
	}

	SavedBuffers.m_Indices.Resize(		Buffers.m_Header.m_NumIndices );
	memcpy( SavedBuffers.m_Indices.GetData(), Buffers.m_Indices, SavedBuffers.m_Indices.MemorySize() );
}

void RosaWorld::FinalizeGeoMeshes()
{
	MergeGeoMeshes();
	SortGeoMeshes();
}

// DLP 12 Oct 2021: This now also merges static geo separately for shadows.
// DLP 27 Nov 2021: This now also merges static geo in single-tile sectors for their own shadows.
#define MERGE_SINGLETILE_SECTOR_SHADOWS 1
void RosaWorld::MergeGeoMeshes()
{
	PROFILE_FUNCTION;

#if BUILD_DEV
	PRINTF( "Merging geo meshes...\n" );

	uint	NumMeshesBefore			= 0;
	uint	NumMeshesIgnored		= 0;
	uint	NumMeshesRemoved		= 0;
	uint	NumMeshesAdded			= 0;
	uint	NumShadowMeshesBefore	= 0;
	uint	NumShadowMeshesAfter	= 0;

	DECLARE_AND_START_CLOCK( MergeGeoMeshesClock );
#endif

	IRenderer* const pRenderer = m_Framework->GetRenderer();

#if MERGE_SINGLETILE_SECTOR_SHADOWS
	// First, merge shadow meshes for tiny single-tile sectors
	Array<uint> SingleTileShadowMeshGeoIndices;
	{
		Array<uint>	StaticShadowMeshes;
		Vector		SingleTileRenderBoundMin;
		bool		HasSetSingleTileRenderBoundMin = false;
		FOR_EACH_ARRAY( SectorIter, m_Sectors, SSector )
		{
			SSector& Sector = SectorIter.GetValue();
			if( !Sector.m_IsSingleTile )
			{
				continue;
			}

			if( HasSetSingleTileRenderBoundMin )
			{
				SingleTileRenderBoundMin.x = Min( SingleTileRenderBoundMin.x, Sector.m_RenderBound.m_Min.x );
				SingleTileRenderBoundMin.y = Min( SingleTileRenderBoundMin.y, Sector.m_RenderBound.m_Min.y );
				SingleTileRenderBoundMin.z = Min( SingleTileRenderBoundMin.z, Sector.m_RenderBound.m_Min.z );
			}
			else
			{
				SingleTileRenderBoundMin = Sector.m_RenderBound.m_Min;
			}

			FOR_EACH_ARRAY( GeoMeshIter, Sector.m_GeoMeshes, uint )
			{
				const uint			GeoMeshIndex	= GeoMeshIter.GetValue();
				const SGeoMesh&		GeoMesh			= m_GeoMeshes[ GeoMeshIndex ];
				Material&			GeoMat			= GeoMesh.m_Mesh->m_Material;

				STATIC_HASHED_STRING( RosaShadowMaterialOverrides );
				const HashedString	ShadowOverrideMat	= ConfigManager::GetHash( GeoMat.GetName(), HashedString::NullString, sRosaShadowMaterialOverrides );
				DEVASSERTDESC( ShadowOverrideMat != HashedString::NullString, "Geo mesh has no shadow material override." );

				// DLP 12 Oct 2021: HACKHACK: This is content-aware, but I'm unlikely to ever change the name. (See also the SetMaterialDefinition call below.)
				STATIC_HASHED_STRING( Material_Shadow_Static );
				const bool			IsStaticShadowMesh	= ( sMaterial_Shadow_Static == ShadowOverrideMat );
				if( !IsStaticShadowMesh )
				{
					continue;
				}

				// It's possible this flag has already been turned off if two sectors include this mesh. That's fine.
				GeoMat.SetFlag( MAT_SHADOW, false );	// Turn off the shadow on the geo, whether we use it or merge it.
				StaticShadowMeshes.PushBack( GeoMeshIndex );
#if BUILD_DEV
				++NumShadowMeshesBefore;
#endif
			}
		}

		// Merge single-tile sector static shadow geo meshes. This is duplicated from normal shadow merging below, I should probably clean this up.
		Array<SGeoMeshMergeBucket> MergeBuckets;
		FOR_EACH_ARRAY( StaticShadowMeshIter, StaticShadowMeshes, uint )
		{
			const uint					GeoMeshIndex	= StaticShadowMeshIter.GetValue();
			const SimpleString&			MeshName		= m_GeoMeshNames[ GeoMeshIndex ];
			DEVASSERT( m_SavedGeoMeshBuffers.Search( MeshName ).IsValid() );
			const SSavedGeoMeshBuffers&	SavedBuffers	= m_SavedGeoMeshBuffers[ MeshName ];
			const uint					NumVerts		= SavedBuffers.m_Positions.Size();

			uint BucketIndex = MergeBuckets.Size();
			FOR_EACH_ARRAY( BucketIter, MergeBuckets, SGeoMeshMergeBucket )
			{
				const SGeoMeshMergeBucket& MergeBucket = BucketIter.GetValue();
#if !USE_LONG_INDICES
				if( MergeBucket.m_NumVerts + NumVerts > 65535 )
				{
					continue;
				}
#endif

				BucketIndex = BucketIter.GetIndex();
			}
			if( BucketIndex == MergeBuckets.Size() )
			{
				MergeBuckets.PushBack();
			}

			SGeoMeshMergeBucket& MergeBucket = MergeBuckets[ BucketIndex ];
			MergeBucket.m_NumVerts += NumVerts;
			MergeBucket.m_Meshes.PushBack( GeoMeshIndex );
		}

		FOR_EACH_ARRAY( BucketIter, MergeBuckets, SGeoMeshMergeBucket )
		{
			const SGeoMeshMergeBucket&	MergeBucket		= BucketIter.GetValue();

			DEVASSERT( MergeBucket.m_Meshes.Size() > 0 );
			// DLP 12 Oct 2021: Note that unlike normal mesh merging, we DO handle the case of just 1 mesh in a bucket,
			// because we still have to make it a shadow-casting mesh since we stripped that flag from the real geo.

			const uint					FirstMeshIndex	= MergeBucket.m_Meshes[0];
			const SGeoMesh&				FirstMesh		= m_GeoMeshes[ FirstMeshIndex ];

			SSavedGeoMeshBuffers		MergeBuffers;
			AABB						MergedAABB		= FirstMesh.m_Mesh->m_AABB;

			FOR_EACH_ARRAY( GeoMeshIter, MergeBucket.m_Meshes, uint )
			{
				const uint					GeoMeshIndex	= GeoMeshIter.GetValue();
				const SGeoMesh&				GeoMesh			= m_GeoMeshes[ GeoMeshIndex ];
				const SimpleString&			MeshName		= m_GeoMeshNames[ GeoMeshIndex ];
				DEVASSERT( m_SavedGeoMeshBuffers.Search( MeshName ).IsValid() );
				const SSavedGeoMeshBuffers&	SavedBuffers	= m_SavedGeoMeshBuffers[ MeshName ];

				// Unlike normal merging, shadows don't need identical VD signatures. We only use positions.
				DEVASSERT( ( GeoMesh.m_Mesh->m_VertexDeclaration->GetSignature() & VD_POSITIONS ) != 0 );

				MergedAABB.ExpandTo( GeoMesh.m_Mesh->m_AABB );

				const index_t				IndexOffset		= static_cast<index_t>( MergeBuffers.m_Positions.Size() );

				const Angles				Rotation		= GeoMesh.m_Mesh->m_Rotation;
				const Vector				SectorLocation	= GeoMesh.m_Mesh->m_Location - SingleTileRenderBoundMin;
				const Matrix				MeshTransform	= Matrix::CreateScale( GeoMesh.m_Mesh->m_Scale ) * Rotation.ToMatrix() * Matrix::CreateTranslation( SectorLocation );
				MergeBuffers.m_Positions.Reserve( MergeBuffers.m_Positions.Size() + SavedBuffers.m_Positions.Size() );
#if !USE_LONG_INDICES
				// This shouldn't happen, because we did merge buckets above.
				DEVASSERT( MergeBuffers.m_Positions.Size() < 65536 );
#endif
				FOR_EACH_ARRAY( PositionIter, SavedBuffers.m_Positions, Vector )
				{
					const Vector&	OldPosition	= PositionIter.GetValue();
					const Vector	NewPosition	= OldPosition * MeshTransform;
					MergeBuffers.m_Positions.PushBack( NewPosition );
				}

				MergeBuffers.m_Indices.Reserve( MergeBuffers.m_Indices.Size() + SavedBuffers.m_Indices.Size() );
				FOR_EACH_ARRAY( IndexIter, SavedBuffers.m_Indices, index_t )
				{
					const index_t&	OldIndex	= IndexIter.GetValue();
					const index_t	NewIndex	= OldIndex + IndexOffset;
					MergeBuffers.m_Indices.PushBack( NewIndex );
				}
			}

			IVertexBuffer*		pVertexBuffer		= pRenderer->CreateVertexBuffer();
			IVertexDeclaration*	pVertexDeclaration	= pRenderer->GetVertexDeclaration( VD_POSITIONS );
			IIndexBuffer*		pIndexBuffer		= pRenderer->CreateIndexBuffer();
			IVertexBuffer::SInit InitStruct;
			InitStruct.NumVertices	= MergeBuffers.m_Positions.Size();
			InitStruct.Positions	= MergeBuffers.m_Positions.GetData();
			pVertexBuffer->Init( InitStruct );
			pIndexBuffer->Init( MergeBuffers.m_Indices.Size(), MergeBuffers.m_Indices.GetData() );
			pIndexBuffer->SetPrimitiveType( EPT_TRIANGLELIST );

			Mesh* const pMergedGeoMesh = new Mesh( pVertexBuffer, pVertexDeclaration, pIndexBuffer );

			// DLP 12 Oct 2021: HACKHACK: This is content-aware, but I'm unlikely to ever change the name. (See also IsStaticShadowMesh above.)
			pMergedGeoMesh->SetMaterialDefinition( "Material_Shadow_Static", pRenderer );
			pMergedGeoMesh->SetMaterialFlags( MAT_SHADOW );
			// HACKHACK: Don't override this material in the shadow bucket, it's already manually overridden.
			// (I could also just override Material_Shadow_Static with itself, but this saves the material construction cost.)
			pMergedGeoMesh->SetAllowMaterialOverrides( false );
			pMergedGeoMesh->m_Location	= SingleTileRenderBoundMin;	// Positioned at sector min, and not rotated at all
			pMergedGeoMesh->m_AABB		= MergedAABB;

#if BUILD_DEBUG
			pMergedGeoMesh->m_DEBUG_Name = SimpleString::PrintF( "Merged Static Shadow GeoMesh %d for Single-Tile Sectors", BucketIter.GetIndex() );
#endif

			const uint NewGeoMeshIndex = m_GeoMeshes.Size();
			SingleTileShadowMeshGeoIndices.PushBack( NewGeoMeshIndex );

			// Add the merged mesh to the world
			SGeoMesh& MergedGeoMesh = m_GeoMeshes.PushBack();
			MergedGeoMesh.m_Mesh = pMergedGeoMesh;

			// Add the merged mesh to all single-tile sectors
			FOR_EACH_ARRAY( SectorIter, m_Sectors, SSector )
			{
				SSector& Sector = SectorIter.GetValue();
				if( !Sector.m_IsSingleTile )
				{
					continue;
				}

				Sector.m_GeoMeshes.PushBack( NewGeoMeshIndex );
			}

#if BUILD_DEV
			++NumShadowMeshesAfter;
#endif
		}
	}
#endif // MERGE_SINGLETILE_SECTOR_SHADOWS

	FOR_EACH_ARRAY( SectorIter, m_Sectors, SSector )
	{
		SSector& Sector = SectorIter.GetValue();

		// Iterate through the meshes in this sector, gather all the unique material sets
		Map<SMaterialMatcher, Array<uint>>	MatchedMeshes;
		Array<uint> StaticShadowMeshes;
		FOR_EACH_ARRAY( GeoMeshIter, Sector.m_GeoMeshes, uint )
		{
			const uint			GeoMeshIndex	= GeoMeshIter.GetValue();

#if MERGE_SINGLETILE_SECTOR_SHADOWS
			if( SingleTileShadowMeshGeoIndices.Contains( GeoMeshIndex ) )
			{
				// This is a single-tile room shadow mesh we already merged, ignore it.
				continue;
			}
#endif // MERGE_SINGLETILE_SECTOR_SHADOWS

			const SGeoMesh&		GeoMesh			= m_GeoMeshes[ GeoMeshIndex ];
			Material&			GeoMat			= GeoMesh.m_Mesh->m_Material;

			const uint			NumSamplers		= GeoMat.GetNumSamplers();
			DEVASSERT( NumSamplers == 3 || NumSamplers == 4 );	// Assumptions! Albedo, normal, spec, and optional overlay.

#if MERGE_SINGLETILE_SECTOR_SHADOWS
			// Single-tile sector shadows were handled above
			if( !Sector.m_IsSingleTile )
#endif // MERGE_SINGLETILE_SECTOR_SHADOWS
			{
				STATIC_HASHED_STRING( RosaShadowMaterialOverrides );
				const HashedString	ShadowOverrideMat	= ConfigManager::GetHash( GeoMat.GetName(), HashedString::NullString, sRosaShadowMaterialOverrides );
				DEVASSERTDESC( ShadowOverrideMat != HashedString::NullString, "Geo mesh has no shadow material override." );

				// DLP 12 Oct 2021: HACKHACK: This is content-aware, but I'm unlikely to ever change the name. (See also the SetMaterialDefinition call below.)
				STATIC_HASHED_STRING( Material_Shadow_Static );
				const bool			IsStaticShadowMesh	= ( sMaterial_Shadow_Static == ShadowOverrideMat );
				if( IsStaticShadowMesh )
				{
					// It's possible this flag has already been turned off if two sectors include this mesh. That's fine.
					GeoMat.SetFlag( MAT_SHADOW, false );	// Turn off the shadow on the geo, whether we use it or merge it.
					StaticShadowMeshes.PushBack( GeoMeshIndex );
#if BUILD_DEV
					++NumShadowMeshesBefore;
#endif
				}
			}

			SMaterialMatcher	GeoMatcher;
			GeoMatcher.m_Flags				= GeoMat.GetFlags();
			GeoMatcher.m_VertexSignature	= GeoMesh.m_Mesh->m_VertexDeclaration->GetSignature();
			DEVASSERT( GeoMat.GetExpectedVD() == ( GeoMatcher.m_VertexSignature & GeoMat.GetExpectedVD() ) );
			GeoMatcher.m_Albedo				= GeoMat.GetSamplerState( 0 ).m_Texture;
			GeoMatcher.m_Normal				= GeoMat.GetSamplerState( 1 ).m_Texture;
			GeoMatcher.m_Spec				= GeoMat.GetSamplerState( 2 ).m_Texture;
			GeoMatcher.m_Overlay			= ( NumSamplers == 4 ) ? GeoMat.GetSamplerState( 3 ).m_Texture : NULL;

			MatchedMeshes[ GeoMatcher ].PushBack( GeoMeshIndex );

#if BUILD_DEV
			++NumMeshesBefore;
#endif
		}

		// Merge larger sector static shadow geo meshes. This is mostly copied from the "real" merging below, see that for more comments.
#if MERGE_SINGLETILE_SECTOR_SHADOWS
		if( !Sector.m_IsSingleTile )
#endif // MERGE_SINGLETILE_SECTOR_SHADOWS
		{
			Array<SGeoMeshMergeBucket> MergeBuckets;
			FOR_EACH_ARRAY( StaticShadowMeshIter, StaticShadowMeshes, uint )
			{
				const uint					GeoMeshIndex	= StaticShadowMeshIter.GetValue();
				const SimpleString&			MeshName		= m_GeoMeshNames[ GeoMeshIndex ];
				DEVASSERT( m_SavedGeoMeshBuffers.Search( MeshName ).IsValid() );
				const SSavedGeoMeshBuffers&	SavedBuffers	= m_SavedGeoMeshBuffers[ MeshName ];
				const uint					NumVerts		= SavedBuffers.m_Positions.Size();

				uint BucketIndex = MergeBuckets.Size();
				FOR_EACH_ARRAY( BucketIter, MergeBuckets, SGeoMeshMergeBucket )
				{
					const SGeoMeshMergeBucket& MergeBucket = BucketIter.GetValue();
#if !USE_LONG_INDICES
					if( MergeBucket.m_NumVerts + NumVerts > 65535 )
					{
						continue;
					}
#endif

					BucketIndex = BucketIter.GetIndex();
				}
				if( BucketIndex == MergeBuckets.Size() )
				{
					MergeBuckets.PushBack();
				}

				SGeoMeshMergeBucket& MergeBucket = MergeBuckets[ BucketIndex ];
				MergeBucket.m_NumVerts += NumVerts;
				MergeBucket.m_Meshes.PushBack( GeoMeshIndex );
			}

			FOR_EACH_ARRAY( BucketIter, MergeBuckets, SGeoMeshMergeBucket )
			{
				const SGeoMeshMergeBucket&	MergeBucket		= BucketIter.GetValue();

				DEVASSERT( MergeBucket.m_Meshes.Size() > 0 );
				// DLP 12 Oct 2021: Note that unlike normal mesh merging, we DO handle the case of just 1 mesh in a bucket,
				// because we still have to make it a shadow-casting mesh since we stripped that flag from the real geo.

				const uint					FirstMeshIndex	= MergeBucket.m_Meshes[0];
				const SGeoMesh&				FirstMesh		= m_GeoMeshes[ FirstMeshIndex ];

				SSavedGeoMeshBuffers		MergeBuffers;
				AABB						MergedAABB		= FirstMesh.m_Mesh->m_AABB;

				FOR_EACH_ARRAY( GeoMeshIter, MergeBucket.m_Meshes, uint )
				{
					const uint					GeoMeshIndex	= GeoMeshIter.GetValue();
					const SGeoMesh&				GeoMesh			= m_GeoMeshes[ GeoMeshIndex ];
					const SimpleString&			MeshName		= m_GeoMeshNames[ GeoMeshIndex ];
					DEVASSERT( m_SavedGeoMeshBuffers.Search( MeshName ).IsValid() );
					const SSavedGeoMeshBuffers&	SavedBuffers	= m_SavedGeoMeshBuffers[ MeshName ];

					// Unlike normal merging, shadows don't need identical VD signatures. We only use positions.
					DEVASSERT( ( GeoMesh.m_Mesh->m_VertexDeclaration->GetSignature() & VD_POSITIONS ) != 0 );

					MergedAABB.ExpandTo( GeoMesh.m_Mesh->m_AABB );

					const index_t				IndexOffset		= static_cast<index_t>( MergeBuffers.m_Positions.Size() );

					const Angles				Rotation		= GeoMesh.m_Mesh->m_Rotation;
					const Vector				SectorLocation	= GeoMesh.m_Mesh->m_Location - Sector.m_RenderBound.m_Min;
					const Matrix				MeshTransform	= Matrix::CreateScale( GeoMesh.m_Mesh->m_Scale ) * Rotation.ToMatrix() * Matrix::CreateTranslation( SectorLocation );
					MergeBuffers.m_Positions.Reserve( MergeBuffers.m_Positions.Size() + SavedBuffers.m_Positions.Size() );
#if !USE_LONG_INDICES
					// This shouldn't happen, because we did merge buckets above.
					DEVASSERT( MergeBuffers.m_Positions.Size() < 65536 );
#endif
					FOR_EACH_ARRAY( PositionIter, SavedBuffers.m_Positions, Vector )
					{
						const Vector&	OldPosition	= PositionIter.GetValue();
						const Vector	NewPosition	= OldPosition * MeshTransform;
						MergeBuffers.m_Positions.PushBack( NewPosition );
					}

					MergeBuffers.m_Indices.Reserve( MergeBuffers.m_Indices.Size() + SavedBuffers.m_Indices.Size() );
					FOR_EACH_ARRAY( IndexIter, SavedBuffers.m_Indices, index_t )
					{
						const index_t&	OldIndex	= IndexIter.GetValue();
						const index_t	NewIndex	= OldIndex + IndexOffset;
						MergeBuffers.m_Indices.PushBack( NewIndex );
					}
				}

				IVertexBuffer*		pVertexBuffer		= pRenderer->CreateVertexBuffer();
				IVertexDeclaration*	pVertexDeclaration	= pRenderer->GetVertexDeclaration( VD_POSITIONS );
				IIndexBuffer*		pIndexBuffer		= pRenderer->CreateIndexBuffer();
				IVertexBuffer::SInit InitStruct;
				InitStruct.NumVertices	= MergeBuffers.m_Positions.Size();
				InitStruct.Positions	= MergeBuffers.m_Positions.GetData();
				pVertexBuffer->Init( InitStruct );
				pIndexBuffer->Init( MergeBuffers.m_Indices.Size(), MergeBuffers.m_Indices.GetData() );
				pIndexBuffer->SetPrimitiveType( EPT_TRIANGLELIST );

				Mesh* const pMergedGeoMesh = new Mesh( pVertexBuffer, pVertexDeclaration, pIndexBuffer );

				// DLP 12 Oct 2021: HACKHACK: This is content-aware, but I'm unlikely to ever change the name. (See also IsStaticShadowMesh above.)
				pMergedGeoMesh->SetMaterialDefinition( "Material_Shadow_Static", pRenderer );
				pMergedGeoMesh->SetMaterialFlags( MAT_SHADOW );
				// HACKHACK: Don't override this material in the shadow bucket, it's already manually overridden.
				// (I could also just override Material_Shadow_Static with itself, but this saves the material construction cost.)
				pMergedGeoMesh->SetAllowMaterialOverrides( false );
				pMergedGeoMesh->m_Location	= Sector.m_RenderBound.m_Min;	// Positioned at sector min, and not rotated at all
				pMergedGeoMesh->m_AABB		= MergedAABB;

#if BUILD_DEBUG
				pMergedGeoMesh->m_DEBUG_Name = SimpleString::PrintF( "Merged Static Shadow GeoMesh %d for Sector %d", BucketIter.GetIndex(), SectorIter.GetIndex() );
#endif

				const uint NewGeoMeshIndex = m_GeoMeshes.Size();

				// Add the merged mesh to the world
				SGeoMesh& MergedGeoMesh = m_GeoMeshes.PushBack();
				MergedGeoMesh.m_Mesh = pMergedGeoMesh;

				// Add the merged mesh to this sector
				Sector.m_GeoMeshes.PushBack( NewGeoMeshIndex );

#if BUILD_DEV
				++NumShadowMeshesAfter;
#endif
			}
		}

		// Merge these sets to create new meshes
		Set<uint> RemovedMeshIndices;	// Using a set for fast lookup when removing elements
		FOR_EACH_MAP( MatchIter, MatchedMeshes, SMaterialMatcher, Array<uint> )
		{
			const SMaterialMatcher&	Matcher		= MatchIter.GetKey();
			const Array<uint>&		GeoMeshes	= MatchIter.GetValue();

			if( GeoMeshes.Size() == 1 )
			{
#if BUILD_DEV
				++NumMeshesIgnored;
#endif
				// Easiest case to reject: no other meshes share this material, so need to merge
				continue;
			}

			// For each set of matched materials, make an array of merge buckets to
			// count verts and meshes; greedily fill buckets where meshes will fit,
			// drop buckets that only contain one mesh.
			Array<SGeoMeshMergeBucket> MergeBuckets;
			FOR_EACH_ARRAY( GeoMeshIter, GeoMeshes, uint )
			{
				const uint					GeoMeshIndex	= GeoMeshIter.GetValue();
				const SimpleString&			MeshName		= m_GeoMeshNames[ GeoMeshIndex ];
				DEVASSERT( m_SavedGeoMeshBuffers.Search( MeshName ).IsValid() );
				const SSavedGeoMeshBuffers&	SavedBuffers	= m_SavedGeoMeshBuffers[ MeshName ];
				const uint					NumVerts		= SavedBuffers.m_Positions.Size();

				// Find a bucket that this mesh fits in, or add a new one
				uint BucketIndex = MergeBuckets.Size();
				FOR_EACH_ARRAY( BucketIter, MergeBuckets, SGeoMeshMergeBucket )
				{
					const SGeoMeshMergeBucket& MergeBucket = BucketIter.GetValue();
#if !USE_LONG_INDICES
					if( MergeBucket.m_NumVerts + NumVerts > 65535 )
					{
						continue;
					}
#endif

					BucketIndex = BucketIter.GetIndex();
				}
				if( BucketIndex == MergeBuckets.Size() )
				{
					MergeBuckets.PushBack();
				}

				SGeoMeshMergeBucket& MergeBucket = MergeBuckets[ BucketIndex ];
				MergeBucket.m_NumVerts += NumVerts;
				MergeBucket.m_Meshes.PushBack( GeoMeshIndex );
			}

			FOR_EACH_ARRAY( BucketIter, MergeBuckets, SGeoMeshMergeBucket )
			{
				const SGeoMeshMergeBucket&	MergeBucket		= BucketIter.GetValue();

				DEVASSERT( MergeBucket.m_Meshes.Size() > 0 );
				if( 1 == MergeBucket.m_Meshes.Size() )
				{
#if BUILD_DEV
					++NumMeshesIgnored;
#endif
					// No need to merge a mesh that is in a bucket alone (maybe it was too big for anything else to fit)
					continue;
				}

				const uint					FirstMeshIndex	= MergeBucket.m_Meshes[0];
				const SGeoMesh&				FirstMesh		= m_GeoMeshes[ FirstMeshIndex ];
				const SimpleString&			FirstMaterial	= m_GeoMeshMaterials[ FirstMeshIndex ];

				SSavedGeoMeshBuffers		MergeBuffers;
				AABB						MergedAABB		= FirstMesh.m_Mesh->m_AABB;

				FOR_EACH_ARRAY( GeoMeshIter, MergeBucket.m_Meshes, uint )
				{
					const uint					GeoMeshIndex	= GeoMeshIter.GetValue();
					const SGeoMesh&				GeoMesh			= m_GeoMeshes[ GeoMeshIndex ];
					const SimpleString&			MeshName		= m_GeoMeshNames[ GeoMeshIndex ];
					DEVASSERT( m_SavedGeoMeshBuffers.Search( MeshName ).IsValid() );
					const SSavedGeoMeshBuffers&	SavedBuffers	= m_SavedGeoMeshBuffers[ MeshName ];

					// Make sure expected and actual vertex signatures match
					DEVASSERT( GeoMesh.m_Mesh->GetMaterial().GetExpectedVD()		== FirstMesh.m_Mesh->GetMaterial().GetExpectedVD() );
					DEVASSERT( GeoMesh.m_Mesh->m_VertexDeclaration->GetSignature()	== FirstMesh.m_Mesh->m_VertexDeclaration->GetSignature() );

					// Make sure all element arrays are the same size
					DEVASSERT( 0 == SavedBuffers.m_Colors.Size()	|| SavedBuffers.m_Positions.Size() == SavedBuffers.m_Colors.Size() );
					DEVASSERT( 0 == SavedBuffers.m_UVs.Size()		|| SavedBuffers.m_Positions.Size() == SavedBuffers.m_UVs.Size() );
					DEVASSERT( 0 == SavedBuffers.m_Normals.Size()	|| SavedBuffers.m_Positions.Size() == SavedBuffers.m_Normals.Size() );
					DEVASSERT( 0 == SavedBuffers.m_NormalsB.Size()	|| SavedBuffers.m_Positions.Size() == SavedBuffers.m_NormalsB.Size() );
					DEVASSERT( 0 == SavedBuffers.m_Tangents.Size()	|| SavedBuffers.m_Positions.Size() == SavedBuffers.m_Tangents.Size() );

					// Make sure we have all expected elements (and no extras)
					DEVASSERT( ( 0 == SavedBuffers.m_Colors.Size() )	== ( 0 == ( GeoMesh.m_Mesh->m_VertexDeclaration->GetSignature() & VD_FLOATCOLORS ) ) );
					DEVASSERT( ( 0 == SavedBuffers.m_UVs.Size() )		== ( 0 == ( GeoMesh.m_Mesh->m_VertexDeclaration->GetSignature() & VD_UVS ) ) );
					DEVASSERT( ( 0 == SavedBuffers.m_Normals.Size() )	== ( 0 == ( GeoMesh.m_Mesh->m_VertexDeclaration->GetSignature() & VD_NORMALS ) ) );
					DEVASSERT( ( 0 == SavedBuffers.m_NormalsB.Size() )	== ( 0 == ( GeoMesh.m_Mesh->m_VertexDeclaration->GetSignature() & VD_NORMALS_B ) ) );
					DEVASSERT( ( 0 == SavedBuffers.m_Tangents.Size() )	== ( 0 == ( GeoMesh.m_Mesh->m_VertexDeclaration->GetSignature() & VD_TANGENTS ) ) );

					MergedAABB.ExpandTo( GeoMesh.m_Mesh->m_AABB );

					const index_t				IndexOffset		= static_cast<index_t>( MergeBuffers.m_Positions.Size() );

					const Angles				Rotation		= GeoMesh.m_Mesh->m_Rotation;
					const Vector				SectorLocation	= GeoMesh.m_Mesh->m_Location - Sector.m_RenderBound.m_Min;
					const Matrix				NormalTransform	= Rotation.ToMatrix();
					const Matrix				MeshTransform	= Matrix::CreateScale( GeoMesh.m_Mesh->m_Scale ) * Rotation.ToMatrix() * Matrix::CreateTranslation( SectorLocation );
					MergeBuffers.m_Positions.Reserve( MergeBuffers.m_Positions.Size() + SavedBuffers.m_Positions.Size() );
#if !USE_LONG_INDICES
					// This shouldn't happen, because we did merge buckets above.
					DEVASSERT( MergeBuffers.m_Positions.Size() < 65536 );
#endif
					FOR_EACH_ARRAY( PositionIter, SavedBuffers.m_Positions, Vector )
					{
						const Vector&	OldPosition	= PositionIter.GetValue();
						const Vector	NewPosition	= OldPosition * MeshTransform;
						MergeBuffers.m_Positions.PushBack( NewPosition );
					}

					MergeBuffers.m_Colors.Append(		SavedBuffers.m_Colors );
					MergeBuffers.m_UVs.Append(			SavedBuffers.m_UVs );

					MergeBuffers.m_Normals.Reserve( MergeBuffers.m_Normals.Size() + SavedBuffers.m_Normals.Size() );
					FOR_EACH_ARRAY( NormalIter, SavedBuffers.m_Normals, Vector )
					{
						const Vector&	OldNormal	= NormalIter.GetValue();
						const Vector	NewNormal	= OldNormal * NormalTransform;
						MergeBuffers.m_Normals.PushBack( NewNormal );
					}

					MergeBuffers.m_NormalsB.Reserve( MergeBuffers.m_NormalsB.Size() + SavedBuffers.m_NormalsB.Size() );
					FOR_EACH_ARRAY( NormalBIter, SavedBuffers.m_NormalsB, Vector )
					{
						const Vector&	OldNormalB	= NormalBIter.GetValue();
						const Vector	NewNormalB	= OldNormalB * NormalTransform;
						MergeBuffers.m_NormalsB.PushBack( NewNormalB );
					}

					MergeBuffers.m_Tangents.Reserve( MergeBuffers.m_Tangents.Size() + SavedBuffers.m_Tangents.Size() );
					FOR_EACH_ARRAY( TangentIter, SavedBuffers.m_Tangents, Vector4 )
					{
						const Vector4&	OldTangent	= TangentIter.GetValue();
						const Vector4	NewTangent	= OldTangent * NormalTransform;
						MergeBuffers.m_Tangents.PushBack( NewTangent );
					}

					MergeBuffers.m_Indices.Reserve( MergeBuffers.m_Indices.Size() + SavedBuffers.m_Indices.Size() );
					FOR_EACH_ARRAY( IndexIter, SavedBuffers.m_Indices, index_t )
					{
						const index_t&	OldIndex	= IndexIter.GetValue();
						const index_t	NewIndex	= OldIndex + IndexOffset;
						MergeBuffers.m_Indices.PushBack( NewIndex );
					}

					DEVASSERT( RemovedMeshIndices.Search( GeoMeshIndex ).IsNull() );
					RemovedMeshIndices.Insert( GeoMeshIndex );
#if BUILD_DEV
					++NumMeshesRemoved;
#endif
				}

				const uint			VertexSignature		= Matcher.m_VertexSignature;
				// Make sure all element arrays are the same size
				DEVASSERT( 0 == MergeBuffers.m_Colors.Size()	|| MergeBuffers.m_Positions.Size() == MergeBuffers.m_Colors.Size() );
				DEVASSERT( 0 == MergeBuffers.m_UVs.Size()		|| MergeBuffers.m_Positions.Size() == MergeBuffers.m_UVs.Size() );
				DEVASSERT( 0 == MergeBuffers.m_Normals.Size()	|| MergeBuffers.m_Positions.Size() == MergeBuffers.m_Normals.Size() );
				DEVASSERT( 0 == MergeBuffers.m_NormalsB.Size()	|| MergeBuffers.m_Positions.Size() == MergeBuffers.m_NormalsB.Size() );
				DEVASSERT( 0 == MergeBuffers.m_Tangents.Size()	|| MergeBuffers.m_Positions.Size() == MergeBuffers.m_Tangents.Size() );

				// Make sure we have all expected elements (and no extras)
				DEVASSERT( ( 0 == MergeBuffers.m_Colors.Size() )	== ( 0 == ( VertexSignature & VD_FLOATCOLORS ) ) );
				DEVASSERT( ( 0 == MergeBuffers.m_UVs.Size() )		== ( 0 == ( VertexSignature & VD_UVS ) ) );
				DEVASSERT( ( 0 == MergeBuffers.m_Normals.Size() )	== ( 0 == ( VertexSignature & VD_NORMALS ) ) );
				DEVASSERT( ( 0 == MergeBuffers.m_NormalsB.Size() )	== ( 0 == ( VertexSignature & VD_NORMALS_B ) ) );
				DEVASSERT( ( 0 == MergeBuffers.m_Tangents.Size() )	== ( 0 == ( VertexSignature & VD_TANGENTS ) ) );

				IVertexBuffer*		pVertexBuffer		= pRenderer->CreateVertexBuffer();
				IVertexDeclaration*	pVertexDeclaration	= pRenderer->GetVertexDeclaration( VertexSignature );
				IIndexBuffer*		pIndexBuffer		= pRenderer->CreateIndexBuffer();
				IVertexBuffer::SInit InitStruct;
				InitStruct.NumVertices	= MergeBuffers.m_Positions.Size();
				InitStruct.Positions	= MergeBuffers.m_Positions.GetData();
				InitStruct.FloatColors	= MergeBuffers.m_Colors.GetData();
				InitStruct.UVs			= MergeBuffers.m_UVs.GetData();
				InitStruct.Normals		= MergeBuffers.m_Normals.GetData();
				InitStruct.NormalsB		= MergeBuffers.m_NormalsB.GetData();
				InitStruct.Tangents		= MergeBuffers.m_Tangents.GetData();
				pVertexBuffer->Init( InitStruct );
				pIndexBuffer->Init( MergeBuffers.m_Indices.Size(), MergeBuffers.m_Indices.GetData() );
				pIndexBuffer->SetPrimitiveType( EPT_TRIANGLELIST );

				Mesh* const pMergedGeoMesh = new Mesh( pVertexBuffer, pVertexDeclaration, pIndexBuffer );

				pMergedGeoMesh->SetMaterialDefinition( FirstMaterial, pRenderer );
				pMergedGeoMesh->SetMaterialFlags( Matcher.m_Flags );
				pMergedGeoMesh->m_Location	= Sector.m_RenderBound.m_Min;	// Positioned at sector min, and not rotated at all
				pMergedGeoMesh->m_AABB		= MergedAABB;

				// Set textures from SMaterialMatcher (shouldn't need to AddRef these?)
				pMergedGeoMesh->SetTexture( 0, static_cast<ITexture*>( Matcher.m_Albedo ) );
				pMergedGeoMesh->SetTexture( 1, static_cast<ITexture*>( Matcher.m_Normal ) );
				pMergedGeoMesh->SetTexture( 2, static_cast<ITexture*>( Matcher.m_Spec ) );
				if( Matcher.m_Overlay )
				{
					pMergedGeoMesh->SetTexture( 3, static_cast<ITexture*>( Matcher.m_Overlay ) );
				}

#if BUILD_DEBUG
				pMergedGeoMesh->m_DEBUG_Name = SimpleString::PrintF( "Merged GeoMesh %d for Sector %d", BucketIter.GetIndex(), SectorIter.GetIndex() );
#endif

				const uint NewGeoMeshIndex = m_GeoMeshes.Size();

				// Add the merged mesh to the world
				SGeoMesh& MergedGeoMesh = m_GeoMeshes.PushBack();
				MergedGeoMesh.m_Mesh = pMergedGeoMesh;

				// Add the merged mesh to this sector
				Sector.m_GeoMeshes.PushBack( NewGeoMeshIndex );

#if BUILD_DEV
				++NumMeshesAdded;
#endif
			}
		}

		// Remove the pre-merge mesh indices from this sector (after all merges are done, to avoid indexing errors)
		FOR_EACH_ARRAY_REVERSE( SectorMeshIter, Sector.m_GeoMeshes, uint )
		{
			const uint SectorMeshIndex = SectorMeshIter.GetValue();
			if( RemovedMeshIndices.Search( SectorMeshIndex ).IsValid() )
			{
				Sector.m_GeoMeshes.FastRemove( SectorMeshIter );
			}
		}
	}

	// Finally (and optionally), reduce m_GeoMeshes by deleting anything that's not referenced by any sector.
	// Create a list of all geomeshes referenced by sectors
	Set<uint> ReferencedMeshes;
	FOR_EACH_ARRAY( SectorIter, m_Sectors, SSector )
	{
		const SSector& Sector = SectorIter.GetValue();
		FOR_EACH_ARRAY( MeshIter, Sector.m_GeoMeshes, uint )
		{
			const uint MeshIndex = MeshIter.GetValue();
			ReferencedMeshes.Insert( MeshIndex );
		}
	}
	// Make a map from old indices to new ones to fix up sector geomesh references.
	Map<uint, uint> RemappedGeoMeshIndices;
	uint NumRemovedGeoMeshes = 0;
	FOR_EACH_ARRAY( MeshIter, m_GeoMeshes, SGeoMesh )
	{
		const uint	MeshIndex	= MeshIter.GetIndex();
		if( ReferencedMeshes.Search( MeshIndex ).IsValid() )
		{
			RemappedGeoMeshIndices.Insert( MeshIndex, MeshIndex - NumRemovedGeoMeshes );
		}
		else
		{
			NumRemovedGeoMeshes++;
		}
	}
	// Next, actually delete and remove all the unreferenced meshes in reverse order now that we're done counting.
	FOR_EACH_ARRAY_REVERSE( MeshIter, m_GeoMeshes, SGeoMesh )
	{
		const uint	MeshIndex	= MeshIter.GetIndex();
		if( ReferencedMeshes.Search( MeshIndex ).IsNull() )
		{
			SGeoMesh&	GeoMesh		= MeshIter.GetValue();
			SafeDelete( GeoMesh.m_Mesh );
			m_GeoMeshes.Remove( MeshIter );
		}
	}
	// And then fix up sector meshes from the remap
	FOR_EACH_ARRAY( SectorIter, m_Sectors, SSector )
	{
		const SSector& Sector = SectorIter.GetValue();
		FOR_EACH_ARRAY( MeshIter, Sector.m_GeoMeshes, uint )
		{
			uint& MeshIndex = MeshIter.GetValue();
			DEVASSERT( RemappedGeoMeshIndices.Contains( MeshIndex ) );
			MeshIndex = RemappedGeoMeshIndices[ MeshIndex ];
		}
	}

	// And dump all the redundant data. Keep m_SavedGeoMeshBuffers around, its lifetime should match DynamicMeshManager.
	m_GeoMeshNames.Clear();
	m_GeoMeshMaterials.Clear();

#if BUILD_DEV
	STOP_CLOCK( MergeGeoMeshesClock );
	const float MergeGeoMeshesTimeSeconds = GET_CLOCK( MergeGeoMeshesClock );

	DEVASSERT( NumMeshesBefore == NumMeshesIgnored + NumMeshesRemoved );
	PRINTF( "  Merged %d geo meshes into %d.\n  %d geo meshes not merged.\n  Merged %d geo meshes into %d shadow casters.\n  %d geo meshes are their own shadow casters (foliage, etc.).\n  Merging geo meshes took %.3fs.\n",
		NumMeshesRemoved, NumMeshesAdded, NumMeshesIgnored, NumShadowMeshesBefore, NumShadowMeshesAfter, NumMeshesBefore - NumShadowMeshesBefore, MergeGeoMeshesTimeSeconds );
#endif
}

// DLP 16 Oct 2021: Adapted from Bucket::SortByMaterials(). Sorts within each sector, since that's obviously all we can do here.
void RosaWorld::SortGeoMeshes()
{
	struct SSectorMatSortHelper
	{
		uint			m_GeoMeshIndex;
		IVertexShader*	m_VertexShader;
		IPixelShader*	m_PixelShader;
		ITexture*		m_BaseTexture;

		bool operator<( const SSectorMatSortHelper& Helper ) const
		{
			if( m_VertexShader < Helper.m_VertexShader )
			{
				return true;
			}

			if( m_VertexShader > Helper.m_VertexShader )
			{
				return false;
			}

			if( m_PixelShader < Helper.m_PixelShader )
			{
				return true;
			}

			if( m_PixelShader > Helper.m_PixelShader )
			{
				return false;
			}

			if( m_BaseTexture < Helper.m_BaseTexture )
			{
				return true;
			}

			return false;
		}
	};

	FOR_EACH_ARRAY( SectorIter, m_Sectors, SSector )
	{
		const SSector& Sector = SectorIter.GetValue();

		Array<SSectorMatSortHelper> SortHelpers;
		SortHelpers.Resize( Sector.m_GeoMeshes.Size() );

		FOR_EACH_ARRAY( MeshIter, Sector.m_GeoMeshes, uint )
		{
			const uint				GeoMeshIndex	= MeshIter.GetValue();
			DEBUGASSERT( m_GeoMeshes.IsValidIndex( GeoMeshIndex ) );
			const SGeoMesh&			GeoMesh			= m_GeoMeshes[ GeoMeshIndex ];
			DEBUGASSERT( GeoMesh.m_Mesh );

			SSectorMatSortHelper&	SortHelper		= SortHelpers[ MeshIter.GetIndex() ];

			const Material&			Material		= GeoMesh.m_Mesh->GetMaterial();
			IShaderProgram* const	pShaderProgram	= Material.GetShaderProgram();
			DEBUGASSERT( pShaderProgram );

			SortHelper.m_GeoMeshIndex	= GeoMeshIndex;
			SortHelper.m_VertexShader	= pShaderProgram->GetVertexShader();
			SortHelper.m_PixelShader	= pShaderProgram->GetPixelShader();
			SortHelper.m_BaseTexture	= Material.GetTexture( 0 );
		}

		SortHelpers.InsertionSort();

		FOR_EACH_ARRAY( MeshIter, Sector.m_GeoMeshes, uint )
		{
			uint&						GeoMeshIndex	= MeshIter.GetValue();
			const SSectorMatSortHelper&	SortHelper		= SortHelpers[ MeshIter.GetIndex() ];
			GeoMeshIndex								= SortHelper.m_GeoMeshIndex;
		}
	}
}

void RosaWorld::ConditionalCreateBoundingFogMesh( SSector& Sector, const SimpleString& RoomFilename )
{
	STATICHASH( RosaWorld_Cubemaps );

	SimpleString RoomPath;
	SimpleString RoomFile_Unused;
	FileUtil::SplitLeadingPath( RoomFilename.CStr(), RoomPath, RoomFile_Unused );

	MAKEHASH( RoomPath );
	const SimpleString PathCubemapName = ConfigManager::GetString( sRoomPath, "", sRosaWorld_Cubemaps );

	MAKEHASH( RoomFilename );
	const SimpleString CubemapName = ConfigManager::GetString( sRoomFilename, PathCubemapName.CStr(), sRosaWorld_Cubemaps );

	if( CubemapName == "" )
	{
		// This sector doesn't have a cubemap, so it can't have a fog mesh
		return;
	}

	MAKEHASH( CubemapName );
	STATICHASH( FogMeshDef );
	const HashedString FogMeshDefName = ConfigManager::GetHash( sFogMeshDef, HashedString::NullString, sCubemapName );

	if( HashedString::NullString == FogMeshDefName )
	{
		// This cubemap doesn't want a bounding fog mesh
		return;
	}

	// Contract these bounds like with ambient light meshes to avoid z-fighting with exposed geo.
	static const float	skFogMeshBoundExpansion			= -0.01f;
	static const Vector	skFogMeshBoundExpansionVector	= Vector( skFogMeshBoundExpansion, skFogMeshBoundExpansion, skFogMeshBoundExpansion );
	AABB			Bounds								= Sector.m_RenderBound;
	Bounds.ExpandBy( skFogMeshBoundExpansionVector );

	IRenderer* const			pRenderer		= m_Framework->GetRenderer();

	const uint NumVertices	= 8;
	const uint NumIndices	= 36;

	Array<Vector> Positions;
	Positions.Reserve( NumVertices );

	Array<index_t> Indices;
	Indices.Reserve( NumIndices );

	Positions.PushBack( Vector( Bounds.m_Min.x, Bounds.m_Min.y, Bounds.m_Min.z ) );
	Positions.PushBack( Vector( Bounds.m_Max.x, Bounds.m_Min.y, Bounds.m_Min.z ) );
	Positions.PushBack( Vector( Bounds.m_Min.x, Bounds.m_Max.y, Bounds.m_Min.z ) );
	Positions.PushBack( Vector( Bounds.m_Max.x, Bounds.m_Max.y, Bounds.m_Min.z ) );
	Positions.PushBack( Vector( Bounds.m_Min.x, Bounds.m_Min.y, Bounds.m_Max.z ) );
	Positions.PushBack( Vector( Bounds.m_Max.x, Bounds.m_Min.y, Bounds.m_Max.z ) );
	Positions.PushBack( Vector( Bounds.m_Min.x, Bounds.m_Max.y, Bounds.m_Max.z ) );
	Positions.PushBack( Vector( Bounds.m_Max.x, Bounds.m_Max.y, Bounds.m_Max.z ) );

#define PUSH_INDICES( a, b, c, d ) \
	Indices.PushBack( a );	Indices.PushBack( b );	Indices.PushBack( c ); \
	Indices.PushBack( b );	Indices.PushBack( d );	Indices.PushBack( c )
	PUSH_INDICES( 2, 0, 6, 4 );	// Left face
	PUSH_INDICES( 1, 3, 5, 7 );	// Right face
	PUSH_INDICES( 0, 1, 4, 5 );	// Back face
	PUSH_INDICES( 3, 2, 7, 6 );	// Front face
	PUSH_INDICES( 2, 3, 0, 1 );	// Bottom face
	PUSH_INDICES( 4, 5, 6, 7 );	// Top face
#undef PUSH_INDICES

	IVertexBuffer*		pVertexBuffer		= pRenderer->CreateVertexBuffer();
	IVertexDeclaration*	pVertexDeclaration	= pRenderer->GetVertexDeclaration( VD_POSITIONS );
	IIndexBuffer*		pIndexBuffer		= pRenderer->CreateIndexBuffer();
	IVertexBuffer::SInit InitStruct;
	InitStruct.NumVertices	= NumVertices;
	InitStruct.Positions	= Positions.GetData();
	pVertexBuffer->Init( InitStruct );
	pIndexBuffer->Init( NumIndices, Indices.GetData() );
	pIndexBuffer->SetPrimitiveType( EPT_TRIANGLELIST );

	RosaMesh* const			pFogMesh		= new RosaMesh;
	pFogMesh->Initialize( pVertexBuffer, pVertexDeclaration, pIndexBuffer, NULL /*pBones*/ );

	InitializeFogMesh( pFogMesh );

	pFogMesh->SetAABB( Bounds );
#if BUILD_DEBUG
	pFogMesh->m_DEBUG_Name = "Bounding Fog Mesh";
#endif

	AddFogMesh( Sector, pFogMesh, FogMeshDefName );
}

void RosaWorld::CreateFogMesh( SSector& Sector, const SimpleString& MeshName, const HashedString& FogMeshDefName, const Vector& Location, const Angles& Orientation, const float Scale )
{
	DEVASSERT( MeshName != "" );
	DEVASSERT( FogMeshDefName != HashedString::NullString );

	IRenderer* const			pRenderer		= m_Framework->GetRenderer();

	RosaMesh* const pFogMesh = new RosaMesh;
	pRenderer->GetMeshFactory()->GetDynamicMesh( MeshName.CStr(), pFogMesh );

	InitializeFogMesh( pFogMesh );

	pFogMesh->m_Location	= Location;
	pFogMesh->m_Rotation	= Orientation;
	pFogMesh->m_Scale		= Vector( Scale, Scale, Scale );
	pFogMesh->RecomputeAABB();
#if BUILD_DEBUG
	pFogMesh->m_DEBUG_Name = "Fog Mesh";
#endif

	AddFogMesh( Sector, pFogMesh, FogMeshDefName );
}

void RosaWorld::AddFogMesh( SSector& Sector, RosaMesh* const pFogMesh, const HashedString& FogMeshDefName )
{
	AddFogMeshDef( FogMeshDefName );

	pFogMesh->SetSection( FogMeshDefName );

	Sector.m_FogMeshes.PushBack( pFogMesh );
}

void  RosaWorld::AddFogMeshDef( const HashedString& FogMeshDefName )
{
	if( m_FogMeshDefs.Contains( FogMeshDefName ) )
	{
		// We've already initialized this def.
		return;
	}

	SFogMeshDef& FogMeshDef = m_FogMeshDefs.Insert( FogMeshDefName );

	{
		// Fog color values are linear, not sRGB!
		const Vector	FogMeshColorHSV	= HSV::GetConfigHSV( "FogMeshColor", FogMeshDefName, Vector() );
		FogMeshDef.m_FogMeshColor		= HSV::GetConfigRGBA( "FogMeshColor", FogMeshDefName, Vector4( HSV::HSVToRGB( FogMeshColorHSV ), 1.0f ) );

		// NOTE: Fog mesh near is implicitly zero; that's almost certainly what I'd always want,
		// but also non-zero near values didn't play nice with the way the shader works.
		// (There may actually be a bug with converting scene depth to WS pos in the shader?)
		STATICHASH( FogMeshFar );
		const float FogMeshFar = ConfigManager::GetInheritedFloat( sFogMeshFar, 1.0f, FogMeshDefName );
		DEVASSERT( FogMeshFar > 0.0f );
		FogMeshDef.m_FogMeshParams.x = 1.0f / FogMeshFar;

		STATICHASH( FogMeshExp );
		FogMeshDef.m_FogMeshParams.y = ConfigManager::GetInheritedFloat( sFogMeshExp, 1.0f, FogMeshDefName );
		DEVASSERT( FogMeshDef.m_FogMeshParams.y > 0.0f );
	}
}

void RosaWorld::GetFogMeshValues( const HashedString& FogMeshDefName, Vector4& OutFogMeshColor, Vector4& OutFogMeshParams )
{
	Map<HashedString, SFogMeshDef>::Iterator FogMeshDefIter = m_FogMeshDefs.Search( FogMeshDefName );
	if( FogMeshDefIter.IsNull() )
	{
		DEVWARNDESC( "Given FogMeshDefName not found!" );
		return;
	}

	OutFogMeshColor		= FogMeshDefIter.GetValue().m_FogMeshColor;
	OutFogMeshParams	= FogMeshDefIter.GetValue().m_FogMeshParams;
}

// DLP 29 May 2021: This version is currently only used when setting up cubemaps.
// During rendering, the SDP calls GetFogMeshValues directly using the mesh's Section.
bool RosaWorld::GetFogMeshValuesForCubemap( const SimpleString& CubemapName, Vector4& OutFogMeshColor, Vector4& OutFogMeshParams )
{
	MAKEHASH( CubemapName );

	STATICHASH( FogMeshDef );
	const HashedString	FogMeshDefName	= ConfigManager::GetHash( sFogMeshDef, "", sCubemapName );

	if( HashedString::NullString == FogMeshDefName )
	{
		return false;
	}

	GetFogMeshValues( FogMeshDefName, OutFogMeshColor, OutFogMeshParams );
	return true;
}

void RosaWorld::InitializeFogMesh( Mesh* const pFogMesh )
{
	DEVASSERT( pFogMesh );

	IRenderer* const			pRenderer		= m_Framework->GetRenderer();
	RosaTargetManager* const	pTargetManager	= m_Framework->GetTargetManager();
	IRenderTarget* const		pPrimaryRT		= pTargetManager->GetRenderTarget( "Primary" );
	IRenderTarget* const		pGB_Albedo		= pTargetManager->GetRenderTarget( "GB_Albedo" );
	IRenderTarget* const		pGB_Depth		= pTargetManager->GetRenderTarget( "GB_Depth" );
	ITexture* const				pAlbedoTexture	= pGB_Albedo->GetColorTextureHandle( 0 );
	ITexture* const				pDepthTexture	= pGB_Depth->GetColorTextureHandle( 0 );

	// HACKHACK
	pFogMesh->SetMaterialFlags( MAT_FOG );

	pFogMesh->ClearMultiPassMaterials();
	pFogMesh->AddMultiPassMaterialDefinition( "Material_FogMeshPassA", pRenderer );
	pFogMesh->AddMultiPassMaterialDefinition( "Material_FogMeshPassB", pRenderer );
	pFogMesh->GetMultiPassMaterial( 0 ).SetRenderTarget( pGB_Albedo );
	pFogMesh->GetMultiPassMaterial( 1 ).SetRenderTarget( pPrimaryRT );
	pFogMesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pAlbedoTexture );
	pFogMesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pDepthTexture );
}

void RosaWorld::UpdateFogMeshes()
{
	RosaTargetManager* const	pTargetManager	= m_Framework->GetTargetManager();
	IRenderTarget* const		pPrimaryRT		= pTargetManager->GetRenderTarget( "Primary" );
	IRenderTarget* const		pGB_Albedo		= pTargetManager->GetRenderTarget( "GB_Albedo" );
	IRenderTarget* const		pGB_Depth		= pTargetManager->GetRenderTarget( "GB_Depth" );
	ITexture* const				pAlbedoTexture	= pGB_Albedo->GetColorTextureHandle( 0 );
	ITexture* const				pDepthTexture	= pGB_Depth->GetColorTextureHandle( 0 );

	FOR_EACH_ARRAY( SectorIter, m_Sectors, SSector )
	{
		SSector& Sector = SectorIter.GetValue();
		FOR_EACH_ARRAY( FogMeshIter, Sector.m_FogMeshes, RosaMesh* )
		{
			RosaMesh* const pFogMesh = FogMeshIter.GetValue();
			DEVASSERT( pFogMesh );

			pFogMesh->GetMultiPassMaterial( 0 ).SetRenderTarget( pGB_Albedo );
			pFogMesh->GetMultiPassMaterial( 1 ).SetRenderTarget( pPrimaryRT );
			pFogMesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pAlbedoTexture );
			pFogMesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pDepthTexture );
		}
	}
}

void RosaWorld::ConditionalCreateBoundingAmbientLight( SSector& Sector, const SimpleString& RoomFilename )
{
	STATICHASH( RosaWorld_Cubemaps );

	SimpleString RoomPath;
	SimpleString RoomFile_Unused;
	FileUtil::SplitLeadingPath( RoomFilename.CStr(), RoomPath, RoomFile_Unused );

	MAKEHASH( RoomPath );
	const SimpleString PathCubemapName = ConfigManager::GetString( sRoomPath, "", sRosaWorld_Cubemaps );

	MAKEHASH( RoomFilename );
	const SimpleString CubemapName = ConfigManager::GetString( sRoomFilename, PathCubemapName.CStr(), sRosaWorld_Cubemaps );

	if( CubemapName == "" )
	{
		// This sector doesn't need an ambient light mesh, we'll just use the global ambience
		return;
	}

	// Expand the render bound by a small amount to avoid flicker on exposed faces.
	// If portals are set up so there can never be any exposed faces, this can go away.
	// (Note that this produces a minor artifact when moving between two rooms with
	// ambient bounds, because of the order the faces are drawn. You have to be looking
	// for it, and there's no z-fighting between ambient regions, so eh it's fine.)
	// DLP 24 May 2020: Maybe contracting instead of expanding would be better.
	static const float	skAmbientBoundExpansion			= -0.01f;
	static const Vector	skAmbientBoundExpansionVector	= Vector( skAmbientBoundExpansion, skAmbientBoundExpansion, skAmbientBoundExpansion );
	AABB			Bounds								= Sector.m_RenderBound;
	Bounds.ExpandBy( skAmbientBoundExpansionVector );

	// Create a convex hull to match the AABB
	SConvexHull Hull;
	Hull.m_Bounds = Bounds;
	Hull.m_Hull.AddPlane( Plane( Vector( -1.0f,  0.0f,  0.0f ),  Bounds.m_Min.x ) );
	Hull.m_Hull.AddPlane( Plane( Vector(  1.0f,  0.0f,  0.0f ), -Bounds.m_Max.x ) );
	Hull.m_Hull.AddPlane( Plane( Vector(  0.0f, -1.0f,  0.0f ),  Bounds.m_Min.y ) );
	Hull.m_Hull.AddPlane( Plane( Vector(  0.0f,  1.0f,  0.0f ), -Bounds.m_Max.y ) );
	Hull.m_Hull.AddPlane( Plane( Vector(  0.0f,  0.0f, -1.0f ),  Bounds.m_Min.z ) );
	Hull.m_Hull.AddPlane( Plane( Vector(  0.0f,  0.0f,  1.0f ), -Bounds.m_Max.z ) );

	IRenderer* const			pRenderer		= m_Framework->GetRenderer();

	const uint NumVertices	= 8;
	const uint NumIndices	= 36;

	Array<Vector> Positions;
	Positions.Reserve( NumVertices );

	Array<index_t> Indices;
	Indices.Reserve( NumIndices );

	Positions.PushBack( Vector( Bounds.m_Min.x, Bounds.m_Min.y, Bounds.m_Min.z ) );
	Positions.PushBack( Vector( Bounds.m_Max.x, Bounds.m_Min.y, Bounds.m_Min.z ) );
	Positions.PushBack( Vector( Bounds.m_Min.x, Bounds.m_Max.y, Bounds.m_Min.z ) );
	Positions.PushBack( Vector( Bounds.m_Max.x, Bounds.m_Max.y, Bounds.m_Min.z ) );
	Positions.PushBack( Vector( Bounds.m_Min.x, Bounds.m_Min.y, Bounds.m_Max.z ) );
	Positions.PushBack( Vector( Bounds.m_Max.x, Bounds.m_Min.y, Bounds.m_Max.z ) );
	Positions.PushBack( Vector( Bounds.m_Min.x, Bounds.m_Max.y, Bounds.m_Max.z ) );
	Positions.PushBack( Vector( Bounds.m_Max.x, Bounds.m_Max.y, Bounds.m_Max.z ) );

#define PUSH_INDICES( a, b, c, d ) \
	Indices.PushBack( a );	Indices.PushBack( b );	Indices.PushBack( c ); \
	Indices.PushBack( b );	Indices.PushBack( d );	Indices.PushBack( c )
	PUSH_INDICES( 2, 0, 6, 4 );	// Left face
	PUSH_INDICES( 1, 3, 5, 7 );	// Right face
	PUSH_INDICES( 0, 1, 4, 5 );	// Back face
	PUSH_INDICES( 3, 2, 7, 6 );	// Front face
	PUSH_INDICES( 2, 3, 0, 1 );	// Bottom face
	PUSH_INDICES( 4, 5, 6, 7 );	// Top face
#undef PUSH_INDICES

	IVertexBuffer*		pVertexBuffer		= pRenderer->CreateVertexBuffer();
	IVertexDeclaration*	pVertexDeclaration	= pRenderer->GetVertexDeclaration( VD_POSITIONS );
	IIndexBuffer*		pIndexBuffer		= pRenderer->CreateIndexBuffer();
	IVertexBuffer::SInit InitStruct;
	InitStruct.NumVertices	= NumVertices;
	InitStruct.Positions	= Positions.GetData();
	pVertexBuffer->Init( InitStruct );
	pIndexBuffer->Init( NumIndices, Indices.GetData() );
	pIndexBuffer->SetPrimitiveType( EPT_TRIANGLELIST );

	Mesh* const			pAmbientLightMesh	= new Mesh( pVertexBuffer, pVertexDeclaration, pIndexBuffer );

	InitializeAmbientLightMesh( pAmbientLightMesh, CubemapName );

	pAmbientLightMesh->SetAABB( Bounds );
#if BUILD_DEBUG
	pAmbientLightMesh->m_DEBUG_Name = "Bounding Ambient Light";
#endif

	AddAmbientLight( Sector, pAmbientLightMesh, Hull, CubemapName );
}

void RosaWorld::CreateAmbientLight( SSector& Sector, const SimpleString& MeshName, const SimpleString& CubemapName, const SConvexHull& Hull, const Vector& Location, const Angles& Orientation, const float Scale )
{
	DEVASSERT( MeshName != "" );
	DEVASSERT( CubemapName != "" );

	IRenderer* const			pRenderer		= m_Framework->GetRenderer();

	Mesh* const pAmbientLightMesh = new Mesh;
	pRenderer->GetMeshFactory()->GetDynamicMesh( MeshName.CStr(), pAmbientLightMesh );

	InitializeAmbientLightMesh( pAmbientLightMesh, CubemapName );

	pAmbientLightMesh->m_Location	= Location;
	pAmbientLightMesh->m_Rotation	= Orientation;
	pAmbientLightMesh->m_Scale		= Vector( Scale, Scale, Scale );
	pAmbientLightMesh->RecomputeAABB();
#if BUILD_DEBUG
	pAmbientLightMesh->m_DEBUG_Name = "Ambient Light Mesh";
#endif

	AddAmbientLight( Sector, pAmbientLightMesh, Hull, CubemapName );
}

void RosaWorld::AddAmbientLight( SSector& Sector, Mesh* const pAmbientLightMesh, const SConvexHull& Hull, const SimpleString& CubemapName )
{
	SAmbientLight& AmbientLight	= Sector.m_AmbientLights.PushBack();

	AmbientLight.m_Mesh			= pAmbientLightMesh;

	AmbientLight.m_Hull			= Hull;

	MAKEHASH( CubemapName );

	STATICHASH( Exposure );
	AmbientLight.m_Exposure			= ConfigManager::GetInheritedFloat( sExposure, GetCurrentWorldDef().m_Exposure, sCubemapName );

	AmbientLight.m_RegionFogScalar	= GetRegionFogScalar( CubemapName );

	STATICHASH( MinimapScalar );
	AmbientLight.m_MinimapScalar	= ConfigManager::GetInheritedFloat( sMinimapScalar, GetCurrentWorldDef().m_MinimapScalar, sCubemapName );

	AmbientLight.m_EdgeColorHSV		= HSV::GetConfigHSV( "EdgeColor", sCubemapName, GetCurrentWorldDef().m_EdgeColorHSV );

	STATICHASH( Ambience );
	AmbientLight.m_Ambience			= ConfigManager::GetInheritedString( sAmbience, GetCurrentWorldDef().m_Ambience.CStr(), sCubemapName );

	STATICHASH( Reverb );
	AmbientLight.m_Reverb			= ConfigManager::GetInheritedString( sReverb, GetCurrentWorldDef().m_Reverb.CStr(), sCubemapName );
	DEVASSERT( AmbientLight.m_Reverb != "" );
}

void RosaWorld::InitializeAmbientLightMesh( Mesh* const pAmbientLightMesh, const SimpleString& CubemapName )
{
	DEVASSERT( pAmbientLightMesh );

	IRenderer* const			pRenderer		= m_Framework->GetRenderer();
	RosaTargetManager* const	pTargetManager	= m_Framework->GetTargetManager();

	STATIC_HASHED_STRING( AmbientLights );
	pAmbientLightMesh->SetBucket( sAmbientLights );
	pAmbientLightMesh->ClearMultiPassMaterials();
	pAmbientLightMesh->AddMultiPassMaterialDefinition( "Material_AmbientLightPassA", pRenderer );
	pAmbientLightMesh->AddMultiPassMaterialDefinition( "Material_AmbientLightPassB", pRenderer );
	pAmbientLightMesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pTargetManager->GetRenderTarget( "GB_Albedo" )->GetColorTextureHandle( 0 ) );
	pAmbientLightMesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pTargetManager->GetRenderTarget( "GB_Normal" )->GetColorTextureHandle( 0 ) );
	pAmbientLightMesh->GetMultiPassMaterial( 1 ).SetTexture( 2, pTargetManager->GetRenderTarget( "GB_Depth" )->GetColorTextureHandle( 0 ) );
	pAmbientLightMesh->GetMultiPassMaterial( 1 ).SetTexture( 3, GetCubemap( CubemapName, GetCurrentWorldDef().m_IrradianceDef ) );
}

void RosaWorld::UpdateAmbientLightMeshes()
{
	RosaTargetManager* const	pTargetManager	= m_Framework->GetTargetManager();
	ITexture* const				pAlbedoTexture	= pTargetManager->GetRenderTarget( "GB_Albedo" )->GetColorTextureHandle( 0 );
	ITexture* const				pNormalTexture	= pTargetManager->GetRenderTarget( "GB_Normal" )->GetColorTextureHandle( 0 );
	ITexture* const				pDepthTexture	= pTargetManager->GetRenderTarget( "GB_Depth" )->GetColorTextureHandle( 0 );

	FOR_EACH_ARRAY( SectorIter, m_Sectors, SSector )
	{
		SSector& Sector = SectorIter.GetValue();
		FOR_EACH_ARRAY( AmbientLightIter, Sector.m_AmbientLights, SAmbientLight )
		{
			const SAmbientLight& AmbientLight = AmbientLightIter.GetValue();
			DEVASSERT( AmbientLight.m_Mesh );

			AmbientLight.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pAlbedoTexture );
			AmbientLight.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pNormalTexture );
			AmbientLight.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 2, pDepthTexture );
		}
	}
}

bool RosaWorld::IsNavNodeUnder( const SNavNode& NavNode, const Vector& Location, float& OutDistance ) const
{
	// Quick cull for distant nodes
	if( Location.x < NavNode.m_Bounds.m_Min.x ||
		Location.x > NavNode.m_Bounds.m_Max.x ||
		Location.y < NavNode.m_Bounds.m_Min.y ||
		Location.y > NavNode.m_Bounds.m_Max.y )
	{
		return false;
	}

	const Vector	DownVector	= Vector( 0.0f, 0.0f, -1.0f );
	const Ray		TraceRay	= Ray( Location, DownVector );
	CollisionInfo	Info;
	if( !TraceRay.Intersects( NavNode.m_Tri, &Info ) )
	{
		return false;
	}

	OutDistance = Info.m_Out_HitT;
	return true;
}

bool RosaWorld::FindNavNodeUnder( const Vector& Location, uint& OutNavNodeIndex ) const
{
	float MinDistance	= FLT_MAX;
	float Distance		= 0.0f;

	for( uint NavNodeIndex = 0; NavNodeIndex < m_NavNodes.Size(); ++NavNodeIndex )
	{
		const SNavNode& NavNode = m_NavNodes[ NavNodeIndex ];
		if( !IsNavNodeUnder( NavNode, Location, Distance ) )
		{
			continue;
		}

		if( Distance >= MinDistance )
		{
			continue;
		}

		OutNavNodeIndex	= NavNodeIndex;
		MinDistance		= Distance;
	}

	return ( MinDistance < FLT_MAX );
}

bool RosaWorld::IsNavMeshUnder( const Vector& Location ) const
{
	uint DummyNavNodeIndex;
	return FindNavNodeUnder( Location, DummyNavNodeIndex );
}

WBEntity* RosaWorld::GetNavEntity( const uint NavNodeIndex ) const
{
	Map<uint, WBEntityRef>::Iterator NavEntityIter = m_NavEntities.Search( NavNodeIndex );
	return NavEntityIter.IsValid() ? NavEntityIter.GetValue().Get() : NULL;
}

void RosaWorld::SetNavEntity( const uint NavNodeIndex, WBEntity* const pEntity )
{
	DEVASSERT( pEntity );
	DEVASSERT( NULL == m_NavEntities[ NavNodeIndex ].Get() );
	m_NavEntities[ NavNodeIndex ] = pEntity;
}

void RosaWorld::ClearNavEntity( const uint NavNodeIndex, WBEntity* const pEntity )
{
	DEVASSERT( pEntity );
	DEVASSERT( pEntity == m_NavEntities[ NavNodeIndex ].Get() );
	Unused( pEntity );
	m_NavEntities.Remove( NavNodeIndex );
}
