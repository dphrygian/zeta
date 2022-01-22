#include "core.h"
#include "wbcomprosamesh.h"
#include "configmanager.h"
#include "rosaframework.h"
#include "irenderer.h"
#include "meshfactory.h"
#include "wbcomprosatransform.h"
#include "wbentity.h"
#include "rosaworld.h"
#include "shadermanager.h"
#include "idatastream.h"
#include "texturemanager.h"
#include "mathcore.h"
#include "wbeventmanager.h"
#include "animation.h"
#include "animationstate.h"
#include "rosagame.h"
#include "wbcomprosafrobbable.h"
#include "wbcomprosacollision.h"
#include "wbcomprosasign.h"
#include "wbcomprosaparticles.h"
#include "wbcomprosasound.h"
#include "wbcomprosacharacterconfig.h"
#include "wbcomprosalight.h"
#include "wbcomprosaantilight.h"
#include "rosawardrobe.h"
#include "plane.h"
#include "mathfunc.h"
#include "rosatargetmanager.h"
#include "view.h"
#include "Components/wbcompstatmod.h"
#include "hsv.h"

// Default highlight is 1,1,1,0 because RGB highlight is multiplicative.
static const Vector4 kDefaultHighlight = Vector4( 1.0f, 1.0f, 1.0f, 0.0f );

// HACKHACK to make sure shadow casting meshes outside visible sectors
Set<Mesh*> sShadowLightMeshes;
/*static*/ const Set<Mesh*>& WBCompRosaMesh::GetAllShadowLightMeshes() { return sShadowLightMeshes; }

WBCompRosaMesh::WBCompRosaMesh()
:	m_Mesh( NULL )
,	m_OriginalMeshAABB()
,	m_Hidden( false )
,	m_BaseHidden( false )
,	m_AcceptsDecals( false )
,	m_CastsShadows( false )
,	m_CastsSelfShadows( false )
,	m_Offset()
,	m_TransientOffset()
,	m_UseOverrideBound( false )
,	m_OverrideBound()
,	m_DefaultAnimBlendTime( 0.0f )
,	m_TransformDependentComponents()
,	m_AnimationDependentComponents()
,	m_ForceUpdateTransform( false )
,	m_OldTransform_Location()
,	m_OldTransform_Rotation()
,	m_OldTransform_Scale()
,	m_MeshName()
,	m_AlbedoMapName()
,	m_NormalMapName()
,	m_SpecMapName()
,	m_AlbedoMapOverride()
,	m_NormalMapOverride()
,	m_SpecMapOverride()
,	m_MaterialOverride()
,	m_AlwaysDraw( false )
,	m_DrawForeground( false )
,	m_IsLight( false )
,	m_IsShadowLight( false )
,	m_IsFogLight( false )
,	m_HasAddedIgnoredMeshes( false )
,	m_IsAntiLight( false )
,	m_IsDecal( false )
,	m_IsFogMesh( false )
,	m_LightCubeMask( 0 )
,	m_AnimationBonesTickRate( 0.0f )
,	m_AnimationTransformTickRate( 0.0f )
,	m_NextAnimationTransformTime( 0.0f )
,	m_CurrentHighlight()
,	m_HighlightInterpTime( 0.0f )
,	m_CullDistanceSq( 0.0f )
,	m_AttachedMeshes()
,	m_AttachmentSets()
{
	STATIC_HASHED_STRING( OnRenderTargetsUpdated );
	GetEventManager()->AddObserver( sOnRenderTargetsUpdated, this );
}

WBCompRosaMesh::~WBCompRosaMesh()
{
	sShadowLightMeshes.Remove( m_Mesh );
	SafeDelete( m_Mesh );

	RemoveAttachments();

	WBEventManager* const pEventManager = GetEventManager();
	if( pEventManager )
	{
		STATIC_HASHED_STRING( OnRenderTargetsUpdated );
		pEventManager->RemoveObserver( sOnRenderTargetsUpdated, this );
	}
}

void WBCompRosaMesh::RemoveAttachments()
{
	FOR_EACH_ARRAY( AttachedMeshIter, m_AttachedMeshes, SAttachedMesh )
	{
		SAttachedMesh& AttachedMesh = AttachedMeshIter.GetValue();

		sShadowLightMeshes.Remove( AttachedMesh.m_Mesh );
		SafeDelete( AttachedMesh.m_Mesh );
	}

	m_AttachedMeshes.Clear();
}

void WBCompRosaMesh::SetMesh( const SimpleString& Mesh )
{
	if( Mesh == "" )
	{
		return;
	}

	m_MeshName = Mesh;

	sShadowLightMeshes.Remove( m_Mesh );
	SafeDelete( m_Mesh );

	m_Mesh = new RosaMesh;
	m_Mesh->SetEntity( GetEntity() );

	IRenderer* const pRenderer = GetFramework()->GetRenderer();
	pRenderer->GetMeshFactory()->GetDynamicMesh( Mesh.CStr(), m_Mesh );

	// Store the true original for blending/restoring with rise from ragdoll
	m_OriginalMeshAABB = m_Mesh->GetAABB();

	if( m_UseOverrideBound )
	{
		m_Mesh->SetAABB( m_OverrideBound );
	}

	if( m_Mesh->IsAnimated() )
	{
		SAnimationListener AnimationListener;
		AnimationListener.m_NotifyFinishedFunc = NotifyAnimationFinished;
		AnimationListener.m_Void = this;
		m_Mesh->AddAnimationListener( AnimationListener );

		m_Mesh->SetAnimationBonesTickRate( m_AnimationBonesTickRate );
	}

	if( m_AlbedoMapOverride != "" )	{ m_Mesh->SetTexture( 0, pRenderer->GetTextureManager()->GetTexture( m_AlbedoMapOverride.CStr() ) ); }
	if( m_NormalMapOverride != "" )	{ m_Mesh->SetTexture( 1, pRenderer->GetTextureManager()->GetTexture( m_NormalMapOverride.CStr() ) ); }
	if( m_SpecMapOverride != "" )	{ m_Mesh->SetTexture( 2, pRenderer->GetTextureManager()->GetTexture( m_SpecMapOverride.CStr() ) ); }

	if( m_MaterialOverride != "" )
	{
		m_Mesh->SetMaterialDefinition( m_MaterialOverride, pRenderer );
	}
	else if( m_IsFogLight )
	{
		m_Mesh->ClearMultiPassMaterials();
		m_Mesh->AddMultiPassMaterialDefinition( "Material_FogLightPassA", pRenderer );
		m_Mesh->AddMultiPassMaterialDefinition( "Material_FogLightPassB", pRenderer );

		// ROSAHACK: Set up the g-buffer texture the light will sample
		RosaFramework* const		pFramework		= RosaFramework::GetInstance();
		RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
		IRenderTarget* const		pPrimaryRT		= pTargetManager->GetRenderTarget( "Primary" );
		IRenderTarget* const		pGB_Albedo		= pTargetManager->GetRenderTarget( "GB_Albedo" );
		IRenderTarget* const		pGB_Depth		= pTargetManager->GetRenderTarget( "GB_Depth" );
		ITexture* const				pAlbedoTexture	= pGB_Albedo->GetColorTextureHandle( 0 );
		ITexture* const				pDepthTexture	= pGB_Depth->GetColorTextureHandle( 0 );
		m_Mesh->GetMultiPassMaterial( 0 ).SetRenderTarget( pGB_Albedo );
		m_Mesh->GetMultiPassMaterial( 1 ).SetRenderTarget( pPrimaryRT );
		m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pAlbedoTexture );
		m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pDepthTexture );
	}
	else if( m_IsShadowLight )
	{
		m_Mesh->ClearMultiPassMaterials();
		m_Mesh->AddMultiPassMaterialDefinition( "Material_LightPassA", pRenderer );
		m_Mesh->AddMultiPassMaterialDefinition( "Material_LightPassB", pRenderer );
		m_Mesh->AddMultiPassMaterialDefinition( "Material_LightPassC", pRenderer );

		// ROSAHACK: Set up the g-buffer texture the light will sample
		RosaFramework* const		pFramework		= RosaFramework::GetInstance();
		RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
		IRenderTarget* const		pGB_Albedo		= pTargetManager->GetRenderTarget( "GB_Albedo" );
		IRenderTarget* const		pGB_Normal		= pTargetManager->GetRenderTarget( "GB_Normal" );
		IRenderTarget* const		pGB_Depth		= pTargetManager->GetRenderTarget( "GB_Depth" );
		IRenderTarget* const		pShadows		= pTargetManager->GetRenderTarget( "ShadowCube" );
		ITexture* const				pAlbedoTexture	= pGB_Albedo->GetColorTextureHandle( 0 );
		ITexture* const				pNormalTexture	= pGB_Normal->GetColorTextureHandle( 0 );
		ITexture* const				pDepthTexture	= pGB_Depth->GetColorTextureHandle( 0 );
		ITexture* const				pShadowCube		= pShadows->GetColorTextureHandle( 0 );
		m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pShadowCube );
		m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pNormalTexture );
		m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 2, pDepthTexture );
		m_Mesh->GetMultiPassMaterial( 2 ).SetTexture( 0, pAlbedoTexture );
		m_Mesh->GetMultiPassMaterial( 2 ).SetTexture( 1, pNormalTexture );
		m_Mesh->GetMultiPassMaterial( 2 ).SetTexture( 2, pDepthTexture );
	}
	else if( m_IsLight )
	{
		m_Mesh->ClearMultiPassMaterials();
		m_Mesh->AddMultiPassMaterialDefinition( "Material_LightPassA", pRenderer );
		m_Mesh->AddMultiPassMaterialDefinition( "Material_LightPassC", pRenderer );

		// ROSAHACK: Set up the g-buffer texture the light will sample
		RosaFramework* const		pFramework		= RosaFramework::GetInstance();
		RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
		IRenderTarget* const		pGB_Albedo		= pTargetManager->GetRenderTarget( "GB_Albedo" );
		IRenderTarget* const		pGB_Normal		= pTargetManager->GetRenderTarget( "GB_Normal" );
		IRenderTarget* const		pGB_Depth		= pTargetManager->GetRenderTarget( "GB_Depth" );
		ITexture* const				pAlbedoTexture	= pGB_Albedo->GetColorTextureHandle( 0 );
		ITexture* const				pNormalTexture	= pGB_Normal->GetColorTextureHandle( 0 );
		ITexture* const				pDepthTexture	= pGB_Depth->GetColorTextureHandle( 0 );
		m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pAlbedoTexture );
		m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pNormalTexture );
		m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 2, pDepthTexture );
	}
	else if( m_IsAntiLight )
	{
		m_Mesh->ClearMultiPassMaterials();
		m_Mesh->AddMultiPassMaterialDefinition( "Material_AntiLightPassA", pRenderer );
		m_Mesh->AddMultiPassMaterialDefinition( "Material_AntiLightPassB", pRenderer );

		// ROSAHACK: Set up the g-buffer texture the anti-light will sample
		RosaFramework* const		pFramework		= RosaFramework::GetInstance();
		RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
		IRenderTarget* const		pGB_Normal		= pTargetManager->GetRenderTarget( "GB_Normal" );
		IRenderTarget* const		pGB_Depth		= pTargetManager->GetRenderTarget( "GB_Depth" );
		ITexture* const				pNormalTexture	= pGB_Normal->GetColorTextureHandle( 0 );
		ITexture* const				pDepthTexture	= pGB_Depth->GetColorTextureHandle( 0 );
		m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pNormalTexture );
		m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pDepthTexture );
	}
	else if( m_IsDecal )
	{
		// ROSAHACK: Set up the g-buffer texture the decal will sample
		RosaFramework* const		pFramework		= RosaFramework::GetInstance();
		RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
		IRenderTarget* const		pGB_Depth		= pTargetManager->GetRenderTarget( "GB_Depth" );
		ITexture* const				pDepthTexture	= pGB_Depth->GetColorTextureHandle( 0 );

		m_Mesh->SetMaterialDefinition( "Material_Decal", pRenderer );
		m_Mesh->SetTexture( 3, pDepthTexture );	// 0-2 should be albedo/normal/spec maps
	}
	else if( m_IsFogMesh )
	{
		m_Mesh->ClearMultiPassMaterials();
		m_Mesh->AddMultiPassMaterialDefinition( "Material_FogMeshPassA", pRenderer );
		m_Mesh->AddMultiPassMaterialDefinition( "Material_FogMeshPassB", pRenderer );

		// ROSAHACK: Set up the g-buffer texture the fog mesh will sample
		RosaFramework* const		pFramework		= RosaFramework::GetInstance();
		RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
		IRenderTarget* const		pPrimaryRT		= pTargetManager->GetRenderTarget( "Primary" );
		IRenderTarget* const		pGB_Albedo		= pTargetManager->GetRenderTarget( "GB_Albedo" );
		IRenderTarget* const		pGB_Depth		= pTargetManager->GetRenderTarget( "GB_Depth" );
		ITexture* const				pAlbedoTexture	= pGB_Albedo->GetColorTextureHandle( 0 );
		ITexture* const				pDepthTexture	= pGB_Depth->GetColorTextureHandle( 0 );
		m_Mesh->GetMultiPassMaterial( 0 ).SetRenderTarget( pGB_Albedo );
		m_Mesh->GetMultiPassMaterial( 1 ).SetRenderTarget( pPrimaryRT );
		m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pAlbedoTexture );
		m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pDepthTexture );
	}
	else if( m_Mesh->IsAnimated() && m_DrawForeground )
	{
		m_Mesh->SetMaterialDefinition( "Material_EntityAnimatedForeground", pRenderer );
	}
	else if( m_Mesh->IsAnimated() )
	{
		m_Mesh->SetMaterialDefinition( "Material_EntityAnimated", pRenderer );
	}
	else
	{
		m_Mesh->SetMaterialDefinition( "Material_EntityStatic", pRenderer );
	}

	if( m_IsFogLight )
	{
		m_Mesh->SetMaterialFlags( MAT_FOGLIGHTS );
	}
	else if( m_IsShadowLight )
	{
		STATIC_HASHED_STRING( ShadowLights );
		m_Mesh->SetBucket( sShadowLights );
		m_Mesh->m_Material.SetLightCubeMask( m_LightCubeMask );

		sShadowLightMeshes.Insert( m_Mesh );
	}
	else if( m_IsLight )
	{
		STATIC_HASHED_STRING( Lights );
		m_Mesh->SetBucket( sLights );
	}
	else if( m_IsAntiLight )
	{
		STATIC_HASHED_STRING( AntiLights );
		m_Mesh->SetBucket( sAntiLights );
	}
	else if( m_IsDecal )
	{
		STATIC_HASHED_STRING( Decals );
		m_Mesh->SetBucket( sDecals );
	}
	else if( m_IsFogMesh )
	{
		m_Mesh->SetMaterialFlags( MAT_FOG );
	}
	else
	{
		uint MaterialFlags	= MAT_DYNAMIC;
		MaterialFlags		|= m_AcceptsDecals				? MAT_DECALS		: MAT_NONE;
		MaterialFlags		|= m_CastsShadows				? MAT_SHADOW		: MAT_NONE;
		MaterialFlags		|= m_AlwaysDraw					? MAT_ALWAYS		: MAT_NONE;
		MaterialFlags		|= m_DrawForeground				? MAT_FOREGROUND	: MAT_WORLD;
		MaterialFlags		|= m_Mesh->SupportsAlphaBlend()	? MAT_ALPHA			: MAT_NONE;
		m_Mesh->SetMaterialFlags( MaterialFlags );
	}

	m_ForceUpdateTransform = true;
}

void WBCompRosaMesh::SetAlbedoMap( const SimpleString& AlbedoMap )
{
	if( AlbedoMap == "" || AlbedoMap == m_AlbedoMapName )
	{
		return;
	}

	m_AlbedoMapName = AlbedoMap;
	m_Mesh->SetTexture( 0, GetFramework()->GetRenderer()->GetTextureManager()->GetTexture( AlbedoMap.CStr() ) );
}

void WBCompRosaMesh::SetNormalMap( const SimpleString& NormalMap )
{
	if( NormalMap == "" || NormalMap == m_NormalMapName )
	{
		return;
	}

	m_NormalMapName = NormalMap;
	m_Mesh->SetTexture( 1, GetFramework()->GetRenderer()->GetTextureManager()->GetTexture( NormalMap.CStr() ) );
}

void WBCompRosaMesh::SetSpecMap( const SimpleString& SpecMap )
{
	if( SpecMap == "" || SpecMap == m_SpecMapName )
	{
		return;
	}

	m_SpecMapName = SpecMap;
	m_Mesh->SetTexture( 2, GetFramework()->GetRenderer()->GetTextureManager()->GetTexture( SpecMap.CStr() ) );
}

/*virtual*/ void WBCompRosaMesh::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( AcceptsDecals );
	m_AcceptsDecals = ConfigManager::GetInheritedBool( sAcceptsDecals, false, sDefinitionName );

	// Set these first so the costume can modify them (defaulting to self like m_MaterialOverride wouldn't work here)
	STATICHASH( CastsShadows );
	m_CastsShadows = ConfigManager::GetInheritedBool( sCastsShadows, true, sDefinitionName );

	STATICHASH( CastsSelfShadows );
	m_CastsSelfShadows = ConfigManager::GetInheritedBool( sCastsSelfShadows, true, sDefinitionName );

	// Set this before SetMesh ever gets called
	STATICHASH( AnimationBonesTickRate );
	m_AnimationBonesTickRate = ConfigManager::GetInheritedFloat( sAnimationBonesTickRate, 0.0f, sDefinitionName );

	STATICHASH( AnimationTransformTickRate );
	m_AnimationTransformTickRate = ConfigManager::GetInheritedFloat( sAnimationTransformTickRate, 0.0f, sDefinitionName );

	WBCompRosaCharacterConfig* const pCharacterConfig = WB_GETCOMP( GetEntity(), RosaCharacterConfig );
	if( pCharacterConfig && pCharacterConfig->HasCostume() )
	{
		InitializeFromCostume();
	}

	STATICHASH( RosaMesh );

	STATICHASH( HighlightInterpTime );
	const float DefaultHighlightInterpTime = ConfigManager::GetFloat( sHighlightInterpTime, 0.0f, sRosaMesh );
	m_HighlightInterpTime = ConfigManager::GetInheritedFloat( sHighlightInterpTime, DefaultHighlightInterpTime, sDefinitionName );
	m_CurrentHighlight.Reset( Interpolator<Vector4>::EIT_None, kDefaultHighlight, kDefaultHighlight, 0.0f );

	STATICHASH( AlbedoMap );
	m_AlbedoMapOverride = ConfigManager::GetInheritedString( sAlbedoMap, "", sDefinitionName );

	STATICHASH( NormalMap );
	m_NormalMapOverride = ConfigManager::GetInheritedString( sNormalMap, "", sDefinitionName );

	STATICHASH( SpecMap );
	m_SpecMapOverride = ConfigManager::GetInheritedString( sSpecMap, "", sDefinitionName );

	STATICHASH( Material );
	m_MaterialOverride = ConfigManager::GetInheritedString( sMaterial, m_MaterialOverride.CStr(), sDefinitionName );	// Default to itself so we don't stomp costume properties

	STATICHASH( IsShadowLight );
	m_IsShadowLight = ConfigManager::GetInheritedBool( sIsShadowLight, false, sDefinitionName );

	STATICHASH( IsLight );
	m_IsLight = m_IsShadowLight || ConfigManager::GetInheritedBool( sIsLight, false, sDefinitionName );

	STATICHASH( IsFogLight );
	m_IsFogLight = ConfigManager::GetInheritedBool( sIsFogLight, false, sDefinitionName );

	STATICHASH( IsAntiLight );
	m_IsAntiLight = ConfigManager::GetInheritedBool( sIsAntiLight, false, sDefinitionName );

	STATICHASH( IsDecal );
	m_IsDecal = ConfigManager::GetInheritedBool( sIsDecal, false, sDefinitionName );

	STATICHASH( IsFogMesh );
	m_IsFogMesh = ConfigManager::GetInheritedBool( sIsFogMesh, false, sDefinitionName );

	// TODO: Make this friendlier if I use it a lot
	// 0x1 = X+, 0x2 = X-, 0x4 = Y+, 0x8 = Y-, 0x10 = Z+, 0x20 = Z-
	STATICHASH( LightCubeMask );
	m_LightCubeMask = ConfigManager::GetInheritedInt( sLightCubeMask, 0x3f, sDefinitionName );

	WBCompRosaTransform* const pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	STATICHASH( OverrideBoundRadius );
	const float		OverrideBoundRadius	= pTransform->GetScale() * ConfigManager::GetInheritedFloat( sOverrideBoundRadius, 0.0f, sDefinitionName );
	const Vector	OverrideBoundVector	= Vector( OverrideBoundRadius, OverrideBoundRadius, OverrideBoundRadius );
	m_UseOverrideBound	= ( OverrideBoundRadius > 0.0f );
	m_OverrideBound		= AABB( -OverrideBoundVector, OverrideBoundVector );

	STATICHASH( DefaultAnimBlendTime );
	m_DefaultAnimBlendTime = ConfigManager::GetInheritedFloat( sDefaultAnimBlendTime, 0.0f, sDefinitionName );

	STATICHASH( AlwaysDraw );
	m_AlwaysDraw = ConfigManager::GetInheritedBool( sAlwaysDraw, false, sDefinitionName );

	STATICHASH( Hidden );
	m_Hidden = ConfigManager::GetInheritedBool( sHidden, false, sDefinitionName );

	STATICHASH( BaseHidden );
	m_BaseHidden = ConfigManager::GetInheritedBool( sBaseHidden, false, sDefinitionName );

	STATICHASH( DrawForeground );
	m_DrawForeground = ConfigManager::GetInheritedBool( sDrawForeground, false, sDefinitionName );

	STATICHASH( NumRandomMeshes );
	const uint			NumRandomMeshes	= ConfigManager::GetInheritedInt( sNumRandomMeshes, 0, sDefinitionName );
	if( NumRandomMeshes > 0 )
	{
		const uint			RandomMeshIndex	= Math::Random( NumRandomMeshes );
		const SimpleString	RandomMesh		= ConfigManager::GetInheritedSequenceString( "RandomMesh%d", RandomMeshIndex, "", sDefinitionName );
		SetMesh( RandomMesh );

		// HACKHACK: Add an attachment set for this particular mesh
		const SimpleString	RandomMeshAttachmentSetDefinitionName	= ConfigManager::GetInheritedSequenceString( "RandomMesh%dAttachmentSet", RandomMeshIndex, "", sDefinitionName );

		const bool FromSerialization = false;
		AddAttachmentSetFromDefinition( RandomMeshAttachmentSetDefinitionName, FromSerialization );
	}
	else
	{
		STATICHASH( Mesh );
		const SimpleString Mesh = ConfigManager::GetInheritedString( sMesh, "", sDefinitionName );
		SetMesh( Mesh );
	}
	DEVASSERT( m_Mesh );

	STATICHASH( Section );
	const HashedString Section = ConfigManager::GetInheritedHash( sSection, m_Mesh->GetSection(), sDefinitionName );
	m_Mesh->SetSection( Section );

	// We have to do this after setting the section, which has to happen after creating the mesh
	if( m_IsFogMesh )
	{
		GetWorld()->AddFogMeshDef( m_Mesh->GetSection() );
	}

	STATICHASH( OffsetZ );
	m_Offset.z = pTransform->GetScale() * ConfigManager::GetInheritedFloat( sOffsetZ, 0.0f, sDefinitionName );

	STATICHASH( CullDistance );
	const float DefaultCullDistance = ConfigManager::GetFloat( sCullDistance, 0.0f, sRosaMesh );
	m_CullDistanceSq = Square( ConfigManager::GetInheritedFloat( sCullDistance, DefaultCullDistance, sDefinitionName ) );

	STATICHASH( Scale );
	const float Scale = ConfigManager::GetInheritedFloat( sScale, 1.0f, sDefinitionName );

	STATICHASH( ScaleX );
	Vector MeshScale;
	MeshScale.x = ConfigManager::GetInheritedFloat( sScaleX, Scale, sDefinitionName );

	STATICHASH( ScaleY );
	MeshScale.y = ConfigManager::GetInheritedFloat( sScaleY, Scale, sDefinitionName );

	STATICHASH( ScaleZ );
	MeshScale.z = ConfigManager::GetInheritedFloat( sScaleZ, Scale, sDefinitionName );

	SetMeshScale( MeshScale * pTransform->GetScale() );

	const bool FromSerialization = false;
	AddAttachmentSetFromDefinition( DefinitionName, FromSerialization );
}

void WBCompRosaMesh::InitializeFromCostume()
{
	WBCompRosaCharacterConfig* const	pCharacterConfig	= WB_GETCOMP( GetEntity(), RosaCharacterConfig );
	DEVASSERT( pCharacterConfig );
	DEVASSERT( pCharacterConfig->HasCostume() );

	RosaGame* const						pGame				= GetGame();
	DEVASSERT( pGame );

	RosaWardrobe* const					pWardrobe			= pGame->GetWardrobe();
	DEVASSERT( pWardrobe );

	const RosaWardrobe::SCostume&		Costume				= pCharacterConfig->GetCostume();
	const RosaWardrobe::SWardrobe&		Wardrobe			= pWardrobe->GetWardrobe( Costume.m_Wardrobe );

	// ROSATODO: Maybe define this sort of assumption in [RosaMesh] or something
	const SimpleString&					CharacterMaterial	= "Material_Character";
	const SimpleString&					AccessoryMaterial	= "Material_EntityStatic";
	STATIC_HASHED_STRING( Skin );
	STATIC_HASHED_STRING( Clothes );

	const RosaWardrobe::SBody&			Body				= pWardrobe->GetBody( Costume.m_Body );
	m_CastsShadows											= Wardrobe.m_CastsShadows;
	m_CastsSelfShadows										= Wardrobe.m_CastsSelfShadows;
	m_MaterialOverride										= CharacterMaterial;
	SetMesh( Body.m_Mesh );
	m_Mesh->SetSection( sSkin );

	if( Costume.m_Head )
	{
		const RosaWardrobe::SHead&			Head			= pWardrobe->GetHead( Costume.m_Head );
		const SimpleString&					HeadMeshName	= Costume.m_IsHuman ? Head.m_HumanMesh : Head.m_VampireMesh;
		if( HeadMeshName != "" )
		{
			SAttachedMesh&					HeadMesh		= m_AttachedMeshes.PushBack();
			HeadMesh.m_CastsShadows							= Wardrobe.m_CastsShadows;
			HeadMesh.m_CastsSelfShadows						= Wardrobe.m_CastsSelfShadows;
			HeadMesh.m_IsSkinned							= true;
			HeadMesh.m_MaterialOverride						= CharacterMaterial;
			SetAttachmentMesh( HeadMesh, HeadMeshName );
			HeadMesh.m_Mesh->SetSection( sSkin );
		}
	}

	if( Costume.m_Style )
	{
		const RosaWardrobe::SStyle&			Style			= pWardrobe->GetStyle( Costume.m_Style );
		if( Style.m_Mesh != "" )
		{
			SAttachedMesh&					StyleMesh		= m_AttachedMeshes.PushBack();
			StyleMesh.m_CastsShadows						= Wardrobe.m_CastsShadows;
			StyleMesh.m_CastsSelfShadows					= Wardrobe.m_CastsSelfShadows;
			StyleMesh.m_IsSkinned							= true;
			StyleMesh.m_MaterialOverride					= CharacterMaterial;
			SetAttachmentMesh( StyleMesh, Style.m_Mesh );
			StyleMesh.m_Mesh->SetSection( sSkin );
		}
	}

	if( Costume.m_Accessory )
	{
		const RosaWardrobe::SAccessory&		Accessory			= pWardrobe->GetAccessory( Costume.m_Accessory );
		if( Accessory.m_Mesh != "" )
		{
			SAttachedMesh&					AccessoryMesh		= m_AttachedMeshes.PushBack();
			AccessoryMesh.m_CastsShadows						= Wardrobe.m_CastsShadows;
			AccessoryMesh.m_CastsSelfShadows					= Wardrobe.m_CastsSelfShadows;
			AccessoryMesh.m_TranslationOffset					= Accessory.m_LocationOffset;
			AccessoryMesh.m_MaterialOverride					= AccessoryMaterial;
			SetAttachmentOrientationOffset( AccessoryMesh, Accessory.m_OrientationOffset );

			SetAttachmentBone( AccessoryMesh, Accessory.m_Bone );
			SetAttachmentMesh( AccessoryMesh, Accessory.m_Mesh );
		}
	}

	FOR_EACH_ARRAY( OptionIter, Costume.m_Options, HashedString )
	{
		const RosaWardrobe::SOption&	Option				= pWardrobe->GetOption( OptionIter.GetValue() );

		if( Option.m_Mesh == "" )
		{
			// Skip empty options (we can't exclude them earlier because
			// they're not necessarily instantiated until GetOption()).
			continue;
		}

		SAttachedMesh&					OptionMesh			= m_AttachedMeshes.PushBack();
		OptionMesh.m_CastsShadows							= Wardrobe.m_CastsShadows;
		OptionMesh.m_CastsSelfShadows						= Wardrobe.m_CastsSelfShadows;
		OptionMesh.m_IsSkinned								= true;
		OptionMesh.m_MaterialOverride						= CharacterMaterial;
		SetAttachmentMesh( OptionMesh, Option.m_Mesh );
		OptionMesh.m_Mesh->SetSection( sClothes );
	}
}

void WBCompRosaMesh::AddAttachmentSetsFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( NumAttachmentSets );
	const uint NumAttachmentSets = ConfigManager::GetInheritedInt( sNumAttachmentSets, 0, sDefinitionName );
	for( uint AttachmentSetIndex = 0; AttachmentSetIndex < NumAttachmentSets; ++AttachmentSetIndex )
	{
		const SimpleString AttachmentSetDefinitionName = ConfigManager::GetInheritedSequenceString( "AttachmentSet%d", AttachmentSetIndex, "", sDefinitionName );

		const bool FromSerialization = false;
		AddAttachmentSetFromDefinition( AttachmentSetDefinitionName, FromSerialization );
	}

	STATICHASH( NumRandomAttachmentSets );
	const uint NumRandomAttachmentSets = ConfigManager::GetInheritedInt( sNumRandomAttachmentSets, 0, sDefinitionName );
	if( NumRandomAttachmentSets > 0 )
	{
		const uint			RandomAttachmentSetIndex			= Math::Random( NumRandomAttachmentSets );
		const SimpleString	RandomAttachmentSetDefinitionName	= ConfigManager::GetInheritedSequenceString( "RandomAttachmentSet%d", RandomAttachmentSetIndex, "", sDefinitionName );

		const bool FromSerialization = false;
		AddAttachmentSetFromDefinition( RandomAttachmentSetDefinitionName, FromSerialization );
	}
}

void WBCompRosaMesh::AddAttachmentSetFromDefinition( const SimpleString& AttachmentSetDefinitionName, const bool FromSerialization )
{
	MAKEHASH( AttachmentSetDefinitionName );

	STATICHASH( NumAttachedMeshes );
	const uint NumAttachedMeshes = ConfigManager::GetInheritedInt( sNumAttachedMeshes, 0, sAttachmentSetDefinitionName );
	for( uint AttachedMeshIndex = 0; AttachedMeshIndex < NumAttachedMeshes; ++AttachedMeshIndex )
	{
		SAttachedMesh& AttachedMesh = m_AttachedMeshes.PushBack();

		AttachedMesh.m_Tag = ConfigManager::GetInheritedSequenceHash(							"AttachedMesh%dTag",				AttachedMeshIndex, HashedString::NullString, sAttachmentSetDefinitionName );

		const HashedString Bone = ConfigManager::GetInheritedSequenceHash(						"AttachedMesh%dBone",				AttachedMeshIndex, HashedString::NullString, sAttachmentSetDefinitionName );
		SetAttachmentBone( AttachedMesh, Bone );

		AttachedMesh.m_AbsoluteOffset		= ConfigManager::GetInheritedSequenceBool(			"AttachedMesh%dAbsoluteOffset",		AttachedMeshIndex, false,	sAttachmentSetDefinitionName );

		AttachedMesh.m_TranslationOffset.x = ConfigManager::GetInheritedSequenceFloat(			"AttachedMesh%dOffsetX",			AttachedMeshIndex, 0.0f,	sAttachmentSetDefinitionName );
		AttachedMesh.m_TranslationOffset.y = ConfigManager::GetInheritedSequenceFloat(			"AttachedMesh%dOffsetY",			AttachedMeshIndex, 0.0f,	sAttachmentSetDefinitionName );
		AttachedMesh.m_TranslationOffset.z = ConfigManager::GetInheritedSequenceFloat(			"AttachedMesh%dOffsetZ",			AttachedMeshIndex, 0.0f,	sAttachmentSetDefinitionName );

		Angles OrientationOffset;
		OrientationOffset.Pitch	= DEGREES_TO_RADIANS( ConfigManager::GetInheritedSequenceFloat(	"AttachedMesh%dOffsetPitch",		AttachedMeshIndex, 0.0f,	sAttachmentSetDefinitionName ) );
		OrientationOffset.Roll	= DEGREES_TO_RADIANS( ConfigManager::GetInheritedSequenceFloat(	"AttachedMesh%dOffsetRoll",			AttachedMeshIndex, 0.0f,	sAttachmentSetDefinitionName ) );
		OrientationOffset.Yaw	= DEGREES_TO_RADIANS( ConfigManager::GetInheritedSequenceFloat(	"AttachedMesh%dOffsetYaw",			AttachedMeshIndex, 0.0f,	sAttachmentSetDefinitionName ) );
		SetAttachmentOrientationOffset( AttachedMesh, OrientationOffset );

		AttachedMesh.m_AlbedoMapOverride	= ConfigManager::GetInheritedSequenceString(		"AttachedMesh%dAlbedoMap",			AttachedMeshIndex, "",		sAttachmentSetDefinitionName );
		AttachedMesh.m_NormalMapOverride	= ConfigManager::GetInheritedSequenceString(		"AttachedMesh%dNormalMap",			AttachedMeshIndex, "",		sAttachmentSetDefinitionName );
		AttachedMesh.m_SpecMapOverride		= ConfigManager::GetInheritedSequenceString(		"AttachedMesh%dSpecMap",			AttachedMeshIndex, "",		sAttachmentSetDefinitionName );
		AttachedMesh.m_MaterialOverride		= ConfigManager::GetInheritedSequenceString(		"AttachedMesh%dMaterial",			AttachedMeshIndex, "",		sAttachmentSetDefinitionName );
		AttachedMesh.m_IsShadowLight		= ConfigManager::GetInheritedSequenceBool(			"AttachedMesh%dIsShadowLight",		AttachedMeshIndex, false,	sAttachmentSetDefinitionName );
		AttachedMesh.m_IsLight				= AttachedMesh.m_IsShadowLight ||
											  ConfigManager::GetInheritedSequenceBool(			"AttachedMesh%dIsLight",			AttachedMeshIndex, false,	sAttachmentSetDefinitionName );
		AttachedMesh.m_IsFogLight			= ConfigManager::GetInheritedSequenceBool(			"AttachedMesh%dIsFogLight",			AttachedMeshIndex, false,	sAttachmentSetDefinitionName );
		AttachedMesh.m_IsAntiLight			= ConfigManager::GetInheritedSequenceBool(			"AttachedMesh%dIsAntiLight",		AttachedMeshIndex, false,	sAttachmentSetDefinitionName );
		AttachedMesh.m_IsFogMesh			= ConfigManager::GetInheritedSequenceBool(			"AttachedMesh%dIsFogMesh",			AttachedMeshIndex, false,	sAttachmentSetDefinitionName );
		// TODO: Make this friendlier if I use it a lot
		// 0x1 = X+, 0x2 = X-, 0x4 = Y+, 0x8 = Y-, 0x10 = Z+, 0x20 = Z-
		AttachedMesh.m_LightCubeMask		= ConfigManager::GetInheritedSequenceInt(			"AttachedMesh%dLightCubeMask",		AttachedMeshIndex, 0x3f,	sAttachmentSetDefinitionName );
		AttachedMesh.m_Hidden				= ConfigManager::GetInheritedSequenceBool(			"AttachedMesh%dHidden",				AttachedMeshIndex, false,	sAttachmentSetDefinitionName );
		AttachedMesh.m_AcceptsDecals		= ConfigManager::GetInheritedSequenceBool(			"AttachedMesh%AcceptsDecals",		AttachedMeshIndex, false,	sAttachmentSetDefinitionName );
		AttachedMesh.m_CastsShadows			= ConfigManager::GetInheritedSequenceBool(			"AttachedMesh%dCastsShadows",		AttachedMeshIndex, true,	sAttachmentSetDefinitionName );
		AttachedMesh.m_CastsSelfShadows		= ConfigManager::GetInheritedSequenceBool(			"AttachedMesh%dCastsSelfShadows",	AttachedMeshIndex, true,	sAttachmentSetDefinitionName );

		// By default, any attached mesh that is a light or anti-light should use its own bounds.
		const bool DefaultOwnBounds			= AttachedMesh.m_IsLight || AttachedMesh.m_IsAntiLight || AttachedMesh.m_IsFogLight;
		AttachedMesh.m_OwnBounds			= ConfigManager::GetInheritedSequenceBool(			"AttachedMesh%dOwnBounds",			AttachedMeshIndex, DefaultOwnBounds, sAttachmentSetDefinitionName );

		// ROSANOTE: Currently, "animated" attachments are just for things like clothing, and share animation state with owner.
		// It's unlikely I'll ever need actually *animated* attachments, but I'm calling this "skinned" instead to be safe.
		AttachedMesh.m_IsSkinned			= ConfigManager::GetInheritedSequenceBool(			"AttachedMesh%dSkinned",			AttachedMeshIndex, false,	sAttachmentSetDefinitionName );

		const SimpleString AttachedMeshFilename = ConfigManager::GetInheritedSequenceString(	"AttachedMesh%d",					AttachedMeshIndex, "",		sAttachmentSetDefinitionName );
		SetAttachmentMesh( AttachedMesh, AttachedMeshFilename );

		if( AttachedMesh.m_Mesh )
		{
			const HashedString Section			= ConfigManager::GetInheritedSequenceHash(		"AttachedMesh%dSection",			AttachedMeshIndex, AttachedMesh.m_Mesh->GetSection(), sAttachmentSetDefinitionName );
			AttachedMesh.m_Mesh->SetSection( Section );

			// We have to do this after setting the section, which has to happen after creating the mesh
			if( AttachedMesh.m_IsFogMesh )
			{
				GetWorld()->AddFogMeshDef( m_Mesh->GetSection() );
			}

			const float Scale					= ConfigManager::GetInheritedSequenceFloat(		"AttachedMesh%dScale",				AttachedMeshIndex, 1.0f,	sAttachmentSetDefinitionName );
			AttachedMesh.m_Mesh->m_Scale		*= Scale;
		}
	}

	if( NumAttachedMeshes > 0 )
	{
		m_AttachmentSets.PushBack( AttachmentSetDefinitionName );
	}

	if( !FromSerialization )
	{
		// Recursively define sets of sets
		AddAttachmentSetsFromDefinition( AttachmentSetDefinitionName );
	}
}

void WBCompRosaMesh::SetAttachmentBone( SAttachedMesh& AttachedMesh, const HashedString& Bone )
{
	DEVASSERT( m_Mesh );

	const bool	AttachedToRoot	= ( Bone == HashedString::NullString );
	DEVASSERT( AttachedToRoot || m_Mesh->GetBones() );

	AttachedMesh.m_Bone			= Bone;
	AttachedMesh.m_BoneIndex	= AttachedToRoot ? INVALID_INDEX : m_Mesh->GetBones()->GetBoneIndex( Bone );

	DEVASSERT( AttachedToRoot || AttachedMesh.m_BoneIndex != INVALID_INDEX );
}

void WBCompRosaMesh::SetAttachmentOrientationOffset( SAttachedMesh& AttachedMesh, const Angles& OrientationOffset )
{
	AttachedMesh.m_OrientationOffset		= OrientationOffset;
	AttachedMesh.m_OrientationOffsetMatrix	= OrientationOffset.ToMatrix();
}

void WBCompRosaMesh::SetAttachmentMesh( SAttachedMesh& AttachedMesh, const SimpleString& AttachedMeshFilename )
{
	DEVASSERT( AttachedMeshFilename != "" );

	sShadowLightMeshes.Remove( AttachedMesh.m_Mesh );
	SafeDelete( AttachedMesh.m_Mesh );

	AttachedMesh.m_Mesh = new RosaMesh;
	AttachedMesh.m_Mesh->SetEntity( GetEntity() );	// ROSANOTE: Eldritch and Neon never did this, but this way I can treat attachments the same as the base mesh

	IRenderer* const pRenderer = GetFramework()->GetRenderer();
	pRenderer->GetMeshFactory()->GetDynamicMesh( AttachedMeshFilename.CStr(), AttachedMesh.m_Mesh );

	if( AttachedMesh.m_IsSkinned )
	{
		DEVASSERT( m_Mesh->IsAnimated() );
		AttachedMesh.m_Mesh->CopyAnimationsFrom( m_Mesh );
	}

	if( AttachedMesh.m_AlbedoMapOverride != "" )	{ AttachedMesh.m_Mesh->SetTexture( 0, pRenderer->GetTextureManager()->GetTexture( AttachedMesh.m_AlbedoMapOverride.CStr() ) ); }
	if( AttachedMesh.m_NormalMapOverride != "" )	{ AttachedMesh.m_Mesh->SetTexture( 1, pRenderer->GetTextureManager()->GetTexture( AttachedMesh.m_NormalMapOverride.CStr() ) ); }
	if( AttachedMesh.m_SpecMapOverride != "" )		{ AttachedMesh.m_Mesh->SetTexture( 2, pRenderer->GetTextureManager()->GetTexture( AttachedMesh.m_SpecMapOverride.CStr() ) ); }

	if( AttachedMesh.m_MaterialOverride != "" )
	{
		AttachedMesh.m_Mesh->SetMaterialDefinition( AttachedMesh.m_MaterialOverride, pRenderer );
	}
	else if( AttachedMesh.m_IsFogLight )
	{
		AttachedMesh.m_Mesh->ClearMultiPassMaterials();
		AttachedMesh.m_Mesh->AddMultiPassMaterialDefinition( "Material_FogLightPassA", pRenderer );
		AttachedMesh.m_Mesh->AddMultiPassMaterialDefinition( "Material_FogLightPassB", pRenderer );

		// ROSAHACK: Set up the g-buffer texture the light will sample
		RosaFramework* const		pFramework		= RosaFramework::GetInstance();
		RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
		IRenderTarget* const		pPrimaryRT		= pTargetManager->GetRenderTarget( "Primary" );
		IRenderTarget* const		pGB_Albedo		= pTargetManager->GetRenderTarget( "GB_Albedo" );
		IRenderTarget* const		pGB_Depth		= pTargetManager->GetRenderTarget( "GB_Depth" );
		ITexture* const				pAlbedoTexture	= pGB_Albedo->GetColorTextureHandle( 0 );
		ITexture* const				pDepthTexture	= pGB_Depth->GetColorTextureHandle( 0 );
		AttachedMesh.m_Mesh->GetMultiPassMaterial( 0 ).SetRenderTarget( pGB_Albedo );
		AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetRenderTarget( pPrimaryRT );
		AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pAlbedoTexture );
		AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pDepthTexture );
	}
	else if( AttachedMesh.m_IsShadowLight )
	{
		AttachedMesh.m_Mesh->ClearMultiPassMaterials();
		AttachedMesh.m_Mesh->AddMultiPassMaterialDefinition( "Material_LightPassA", pRenderer );
		AttachedMesh.m_Mesh->AddMultiPassMaterialDefinition( "Material_LightPassB", pRenderer );
		AttachedMesh.m_Mesh->AddMultiPassMaterialDefinition( "Material_LightPassC", pRenderer );

		// ROSAHACK: Set up the g-buffer texture the light will sample
		RosaFramework* const		pFramework		= RosaFramework::GetInstance();
		RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
		IRenderTarget* const		pGB_Albedo		= pTargetManager->GetRenderTarget( "GB_Albedo" );
		IRenderTarget* const		pGB_Normal		= pTargetManager->GetRenderTarget( "GB_Normal" );
		IRenderTarget* const		pGB_Depth		= pTargetManager->GetRenderTarget( "GB_Depth" );
		IRenderTarget* const		pShadows		= pTargetManager->GetRenderTarget( "ShadowCube" );
		ITexture* const				pAlbedoTexture	= pGB_Albedo->GetColorTextureHandle( 0 );
		ITexture* const				pNormalTexture	= pGB_Normal->GetColorTextureHandle( 0 );
		ITexture* const				pDepthTexture	= pGB_Depth->GetColorTextureHandle( 0 );
		ITexture* const				pShadowCube		= pShadows->GetColorTextureHandle( 0 );
		AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pShadowCube );
		AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pNormalTexture );
		AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 2, pDepthTexture );
		AttachedMesh.m_Mesh->GetMultiPassMaterial( 2 ).SetTexture( 0, pAlbedoTexture );
		AttachedMesh.m_Mesh->GetMultiPassMaterial( 2 ).SetTexture( 1, pNormalTexture );
		AttachedMesh.m_Mesh->GetMultiPassMaterial( 2 ).SetTexture( 2, pDepthTexture );
	}
	else if( AttachedMesh.m_IsLight )
	{
		AttachedMesh.m_Mesh->ClearMultiPassMaterials();
		AttachedMesh.m_Mesh->AddMultiPassMaterialDefinition( "Material_LightPassA", pRenderer );
		AttachedMesh.m_Mesh->AddMultiPassMaterialDefinition( "Material_LightPassC", pRenderer );

		// ROSAHACK: Set up the g-buffer texture the light will sample
		RosaFramework* const		pFramework		= RosaFramework::GetInstance();
		RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
		IRenderTarget* const		pGB_Albedo		= pTargetManager->GetRenderTarget( "GB_Albedo" );
		IRenderTarget* const		pGB_Normal		= pTargetManager->GetRenderTarget( "GB_Normal" );
		IRenderTarget* const		pGB_Depth		= pTargetManager->GetRenderTarget( "GB_Depth" );
		ITexture* const				pAlbedoTexture	= pGB_Albedo->GetColorTextureHandle( 0 );
		ITexture* const				pNormalTexture	= pGB_Normal->GetColorTextureHandle( 0 );
		ITexture* const				pDepthTexture	= pGB_Depth->GetColorTextureHandle( 0 );
		AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pAlbedoTexture );
		AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pNormalTexture );
		AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 2, pDepthTexture );
	}
	else if( AttachedMesh.m_IsAntiLight )
	{
		AttachedMesh.m_Mesh->ClearMultiPassMaterials();
		AttachedMesh.m_Mesh->AddMultiPassMaterialDefinition( "Material_AntiLightPassA", pRenderer );
		AttachedMesh.m_Mesh->AddMultiPassMaterialDefinition( "Material_AntiLightPassB", pRenderer );

		// ROSAHACK: Set up the g-buffer texture the anti-light will sample
		RosaFramework* const		pFramework		= RosaFramework::GetInstance();
		RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
		IRenderTarget* const		pGB_Normal		= pTargetManager->GetRenderTarget( "GB_Normal" );
		IRenderTarget* const		pGB_Depth		= pTargetManager->GetRenderTarget( "GB_Depth" );
		ITexture* const				pNormalTexture	= pGB_Normal->GetColorTextureHandle( 0 );
		ITexture* const				pDepthTexture	= pGB_Depth->GetColorTextureHandle( 0 );
		AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pNormalTexture );
		AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pDepthTexture );
	}
	else if( AttachedMesh.m_IsFogMesh )
	{
		AttachedMesh.m_Mesh->ClearMultiPassMaterials();
		AttachedMesh.m_Mesh->AddMultiPassMaterialDefinition( "Material_FogMeshPassA", pRenderer );
		AttachedMesh.m_Mesh->AddMultiPassMaterialDefinition( "Material_FogMeshPassB", pRenderer );

		// ROSAHACK: Set up the g-buffer texture the fog mesh will sample
		RosaFramework* const		pFramework		= RosaFramework::GetInstance();
		RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
		IRenderTarget* const		pPrimaryRT		= pTargetManager->GetRenderTarget( "Primary" );
		IRenderTarget* const		pGB_Albedo		= pTargetManager->GetRenderTarget( "GB_Albedo" );
		IRenderTarget* const		pGB_Depth		= pTargetManager->GetRenderTarget( "GB_Depth" );
		ITexture* const				pAlbedoTexture	= pGB_Albedo->GetColorTextureHandle( 0 );
		ITexture* const				pDepthTexture	= pGB_Depth->GetColorTextureHandle( 0 );
		AttachedMesh.m_Mesh->GetMultiPassMaterial( 0 ).SetRenderTarget( pGB_Albedo );
		AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetRenderTarget( pPrimaryRT );
		AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pAlbedoTexture );
		AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pDepthTexture );
	}
	else if( AttachedMesh.m_Mesh->IsAnimated() && m_DrawForeground )
	{
		AttachedMesh.m_Mesh->SetMaterialDefinition( "Material_EntityAnimatedForeground", pRenderer );
	}
	else if( AttachedMesh.m_Mesh->IsAnimated() )
	{
		AttachedMesh.m_Mesh->SetMaterialDefinition( "Material_EntityAnimated", pRenderer );
	}
	else
	{
		AttachedMesh.m_Mesh->SetMaterialDefinition( "Material_EntityStatic", pRenderer );
	}

	if( AttachedMesh.m_IsFogLight )
	{
		AttachedMesh.m_Mesh->SetMaterialFlags( MAT_FOGLIGHTS );
	}
	else if( AttachedMesh.m_IsShadowLight )
	{
		STATIC_HASHED_STRING( ShadowLights );
		AttachedMesh.m_Mesh->SetBucket( sShadowLights );
		AttachedMesh.m_Mesh->m_Material.SetLightCubeMask( AttachedMesh.m_LightCubeMask );

		sShadowLightMeshes.Insert( AttachedMesh.m_Mesh );
	}
	else if( AttachedMesh.m_IsLight )
	{
		STATIC_HASHED_STRING( Lights );
		AttachedMesh.m_Mesh->SetBucket( sLights );
	}
	else if( AttachedMesh.m_IsAntiLight )
	{
		STATIC_HASHED_STRING( AntiLights );
		AttachedMesh.m_Mesh->SetBucket( sAntiLights );
	}
	else if( AttachedMesh.m_IsFogMesh )
	{
		AttachedMesh.m_Mesh->SetMaterialFlags( MAT_FOG );
	}
	else
	{
		uint MaterialFlags	= MAT_DYNAMIC;
		MaterialFlags		|= AttachedMesh.m_AcceptsDecals					? MAT_DECALS		: MAT_NONE;
		MaterialFlags		|= AttachedMesh.m_CastsShadows					? MAT_SHADOW		: MAT_NONE;
		MaterialFlags		|= m_AlwaysDraw									? MAT_ALWAYS		: MAT_NONE;
		MaterialFlags		|= m_DrawForeground								? MAT_FOREGROUND	: MAT_WORLD;
		MaterialFlags		|= AttachedMesh.m_Mesh->SupportsAlphaBlend()	? MAT_ALPHA			: MAT_NONE;
		AttachedMesh.m_Mesh->SetMaterialFlags( MaterialFlags );
	}

	if( AttachedMesh.m_IsLight || AttachedMesh.m_IsAntiLight )
	{
		// HACKHACK: Don't scale light or anti-light attachments by transform scale, because I'm not scaling their radius to compensate
		// (This is mainly for applying randomized scaling on characters; attachment meshes can still have a config scale applied that
		// will work as expected.)
	}
	else
	{
		WBCompRosaTransform* pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
		DEVASSERT( pTransform );
		const float Scale = pTransform->GetScale();
		AttachedMesh.m_Mesh->m_Scale = Vector( Scale, Scale, Scale );
	}
}

bool WBCompRosaMesh::UpdateMeshTransform()
{
	DEVASSERT( m_Mesh );

	WBCompRosaTransform* pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );	// Makes no sense to have a mesh and no transform

	const Vector EntityLocation		= pTransform->GetLocation();
	const Vector CurrentLocation	= EntityLocation + GetMeshOffset();
	m_Mesh->m_Location = CurrentLocation;

	// HACKHACK: Inject the light offset
	if( m_IsLight )
	{
		const WBCompRosaLight* const	pLight	= WB_GETCOMP( GetEntity(), RosaLight );
		DEVASSERT( pLight );

		m_Mesh->m_Location += pLight->GetTranslation();
	}

	const Angles EntityOrientation	= pTransform->GetOrientation();
	m_Mesh->m_Rotation = EntityOrientation;

	// Optimization, avoid the matrix and AABB calcs if nothing has changed
	if(	m_OldTransform_Location	!= m_Mesh->m_Location	||
		m_OldTransform_Rotation	!= m_Mesh->m_Rotation	||
		m_OldTransform_Scale	!= m_Mesh->m_Scale		||
		m_ForceUpdateTransform )
	{
		m_OldTransform_Location	= m_Mesh->m_Location;
		m_OldTransform_Rotation	= m_Mesh->m_Rotation;
		m_OldTransform_Scale	= m_Mesh->m_Scale;
		m_ForceUpdateTransform	= false;

		m_Mesh->RecomputeAABB();

		return true;
	}
	else
	{
		return false;
	}
}

void WBCompRosaMesh::UpdateMesh( const float DeltaTime )
{
	XTRACE_FUNCTION;

	DEVASSERT( m_Mesh );

	bool ShouldUpdateTransform = false;
	if( !m_Mesh->IsAnimated() || !AnimationState::StaticGetStylizedAnim() || m_AnimationTransformTickRate <= 0.0f )
	{
		ShouldUpdateTransform = true;
	}
	else
	{
		m_NextAnimationTransformTime		-= DeltaTime;
		while( m_NextAnimationTransformTime < 0.0f )
		{
			m_NextAnimationTransformTime	+= m_AnimationTransformTickRate;
			ShouldUpdateTransform			= true;
		}
	}
	const bool TransformUpdated = ShouldUpdateTransform && UpdateMeshTransform();

	if( m_Mesh->IsAnimated() )
	{
		m_Mesh->Tick( DeltaTime );

		// Notify other components so that e.g. hitbox can update
		WB_MAKE_EVENT( OnAnimatedMeshTick, GetEntity() );
		WB_DISPATCH_EVENT( GetEventManager(), OnAnimatedMeshTick, GetEntity() );
	}

	// HACK: Optimization: only send event if transform was updated.
	if( TransformUpdated && m_TransformDependentComponents.Size() )
	{
		// HACK: Optimization: don't provide entity for context, clients of OnMeshUpdated don't need it
		WB_MAKE_EVENT( OnMeshUpdated, NULL );

		// HACK: Optimization: bypass event manager and send event directly to dependent components
		FOR_EACH_ARRAY( ComponentIter, m_TransformDependentComponents, WBComponent* )
		{
			WBComponent* const pDependentComponent = ComponentIter.GetValue();
			pDependentComponent->HandleEvent( WB_AUTO_EVENT( OnMeshUpdated ) );
		}
	}

	if( m_AnimationDependentComponents.Size() )
	{
		// Same HACKs as above
		WB_MAKE_EVENT( OnAnimationTick, NULL );
		FOR_EACH_ARRAY( ComponentIter, m_AnimationDependentComponents, WBComponent* )
		{
			WBComponent* const pDependentComponent = ComponentIter.GetValue();
			pDependentComponent->HandleEvent( WB_AUTO_EVENT( OnAnimationTick ) );
		}
	}
}

/*virtual*/ void WBCompRosaMesh::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;
	PROFILE_FUNCTION;

	m_CurrentHighlight.Tick( DeltaTime );

	UpdateMesh( DeltaTime );

	// Add pseudo root motion. Hack from Couriers.
	if( m_Mesh && m_Mesh->IsAnimated() )
	{
		Vector AnimationVelocity;
		Angles AnimationRotationalVelocity;
		GetAnimationVelocity( AnimationVelocity, AnimationRotationalVelocity );

		if( AnimationVelocity.LengthSquared() > 0.0f || !AnimationRotationalVelocity.IsZero() )
		{
			WBCompRosaTransform* const pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
			DEVASSERT( pTransform );

			// Kill velocity in direction of movement.
			if( AnimationVelocity.LengthSquared() > 0.0f )
			{
				Plane MovementPlane( AnimationVelocity.GetNormalized(), 0.0f );
				pTransform->SetVelocity( MovementPlane.ProjectVector( pTransform->GetVelocity() ) );
			}

			pTransform->ApplyImpulse( AnimationVelocity );
			pTransform->ApplyRotationalImpulse( AnimationRotationalVelocity );
		}
	}

	// HACKHACK: Latently populate ignored self shadow casters
	if( m_IsShadowLight && !m_HasAddedIgnoredMeshes )
	{
		m_HasAddedIgnoredMeshes = true;

		FOR_EACH_ARRAY( AttachedMeshIter, m_AttachedMeshes, SAttachedMesh )
		{
			const SAttachedMesh& AttachedMesh = AttachedMeshIter.GetValue();
			if( !AttachedMesh.m_CastsSelfShadows )
			{
				m_Mesh->m_IgnoredMeshes.Insert( AttachedMesh.m_Mesh );
			}
		}
	}

	FOR_EACH_ARRAY( AttachedMeshIter, m_AttachedMeshes, SAttachedMesh )
	{
		SAttachedMesh& AttachedMesh = AttachedMeshIter.GetValue();
		if( AttachedMesh.m_IsShadowLight && !AttachedMesh.m_HasAddedIgnoredMeshes )
		{
			AttachedMesh.m_HasAddedIgnoredMeshes = true;

			if( !m_CastsSelfShadows )
			{
				AttachedMesh.m_Mesh->m_IgnoredMeshes.Insert( m_Mesh );
			}

			FOR_EACH_ARRAY( OtherAttachedMeshIter, m_AttachedMeshes, SAttachedMesh )
			{
				const SAttachedMesh& OtherAttachedMesh = OtherAttachedMeshIter.GetValue();
				if( !OtherAttachedMesh.m_CastsSelfShadows )
				{
					AttachedMesh.m_Mesh->m_IgnoredMeshes.Insert( OtherAttachedMesh.m_Mesh );
				}
			}
		}
	}
}

void WBCompRosaMesh::UpdateAttachedMeshes()
{
	// Need to update bones to attach to the proper place
	DEVASSERT( !m_Mesh->IsAnimated() || m_Mesh->AreBonesUpdated() );

	const AABB&		BaseBounds			= m_Mesh->m_AABB;
	const Vector	BaseLocation		= m_Mesh->m_Location;
	const Matrix	BaseRotationMatrix	= m_Mesh->m_Rotation.ToMatrix();

	FOR_EACH_ARRAY( AttachedMeshIter, m_AttachedMeshes, SAttachedMesh )
	{
		SAttachedMesh& AttachedMesh	= AttachedMeshIter.GetValue();
		DEBUGASSERT( AttachedMesh.m_Mesh );

		const bool		AttachedToRoot		= ( AttachedMesh.m_Bone == HashedString::NullString );
		const Matrix&	BoneMatrix			= AttachedToRoot ? Matrix() : m_Mesh->GetBoneMatrices()[ AttachedMesh.m_BoneIndex ];
		Matrix			BoneRotationMatrix	= BoneMatrix;
		BoneRotationMatrix.ZeroTranslationElements();

		static const Vector skNoScale	= Vector( 1.0f, 1.0f, 1.0f );
		const Vector TranslationOffset	= ( AttachedMesh.m_TranslationOffset * BoneMatrix ) * ( AttachedMesh.m_AbsoluteOffset ? skNoScale : m_Mesh->m_Scale );
		const Angles OrientationOffset	= ( AttachedMesh.m_OrientationOffsetMatrix * BoneRotationMatrix ).ToAngles();

		AttachedMesh.m_Mesh->m_Location = BaseLocation + ( TranslationOffset * BaseRotationMatrix );
		AttachedMesh.m_Mesh->m_Rotation = ( OrientationOffset.ToMatrix() * BaseRotationMatrix ).ToAngles();

		// HACKHACK: Inject the light offset
		if( AttachedMesh.m_IsLight )
		{
			const WBCompRosaLight* const	pLight	= WB_GETCOMP( GetEntity(), RosaLight );
			DEVASSERT( pLight );

			AttachedMesh.m_Mesh->m_Location += pLight->GetTranslation();
		}

		if( AttachedMesh.m_OwnBounds )
		{
			AttachedMesh.m_Mesh->RecomputeAABB();
		}
		else
		{
			// ROSANOTE: Reusing base mesh's AABB for attached meshes; can cause
			// noticeable errors with frustum culling on large attachments like lights
			AttachedMesh.m_Mesh->m_AABB	= BaseBounds;
		}
	}
}

int WBCompRosaMesh::GetBoneIndex( const HashedString& BoneName ) const
{
	DEVASSERT( m_Mesh );
	return m_Mesh->GetBoneIndex( BoneName );
}

Vector WBCompRosaMesh::GetBoneLocation( const int BoneIndex ) const
{
	DEVASSERT( m_Mesh );
	DEVASSERT( m_Mesh->GetBones() );
	DEVASSERT( BoneIndex != INVALID_INDEX );
	DEVASSERT( BoneIndex < m_Mesh->GetBones()->GetNumBones() );

	// Make sure bones have been refreshed this frame.
	// This won't waste any time if it has.
	m_Mesh->UpdateBones();

	const Vector&		BaseLocation		= m_Mesh->m_Location;
	const Matrix		BaseRotationMatrix	= m_Mesh->m_Rotation.ToMatrix();

	const Matrix&		BoneMatrix			= m_Mesh->GetBoneMatrices()[ BoneIndex ];
	const SBoneInfo&	BoneInfo			= m_Mesh->GetBones()->GetBoneInfo( BoneIndex );

	const Vector		BoneOS				= ( BoneInfo.m_BoneStart * BoneMatrix ) * m_Mesh->m_Scale;
	const Vector		BoneWS				= BaseLocation + ( BoneOS * BaseRotationMatrix );

	return BoneWS;
}

void WBCompRosaMesh::GetBoneTransform( const int BoneIndex, Vector& OutTranslation, Angles& OutOrientation ) const
{
	DEVASSERT( m_Mesh );
	DEVASSERT( m_Mesh->GetBones() );
	DEVASSERT( BoneIndex != INVALID_INDEX );
	DEVASSERT( BoneIndex < m_Mesh->GetBones()->GetNumBones() );

	// Make sure bones have been refreshed this frame.
	// This won't waste any time if it has.
	m_Mesh->UpdateBones();

	const Vector&		BaseLocation		= m_Mesh->m_Location;
	const Matrix		BaseRotationMatrix	= m_Mesh->m_Rotation.ToMatrix();

	const Matrix&		BoneMatrix			= m_Mesh->GetBoneMatrices()[ BoneIndex ];
	const SBoneInfo&	BoneInfo			= m_Mesh->GetBones()->GetBoneInfo( BoneIndex );

	const Vector		BoneTranslationOS	= ( BoneInfo.m_BoneStart * BoneMatrix ) * m_Mesh->m_Scale;
	const Vector		BoneTranslationWS	= BaseLocation + ( BoneTranslationOS * BaseRotationMatrix );

	const Matrix		BoneOrientationOS	= BoneInfo.m_Orientation.ToMatrix() * BoneMatrix;
	const Matrix		BoneOrientationWS	= BoneOrientationOS * BaseRotationMatrix;

	OutTranslation	= BoneTranslationWS;
	OutOrientation	= BoneOrientationWS.ToAngles();
}

Segment WBCompRosaMesh::GetBoneSegment( const int BoneIndex ) const
{
	DEVASSERT( m_Mesh );
	DEVASSERT( m_Mesh->GetBones() );
	DEVASSERT( BoneIndex != INVALID_INDEX );
	DEVASSERT( BoneIndex < m_Mesh->GetBones()->GetNumBones() );

	// Make sure bones have been refreshed this frame.
	// This won't waste any time if it has.
	m_Mesh->UpdateBones();

	const Vector&		BaseLocation		= m_Mesh->m_Location;
	const Matrix		BaseRotationMatrix	= m_Mesh->m_Rotation.ToMatrix();

	const Matrix&		BoneMatrix			= m_Mesh->GetBoneMatrices()[ BoneIndex ];
	const SBoneInfo&	BoneInfo			= m_Mesh->GetBones()->GetBoneInfo( BoneIndex );

	const Vector		BoneStartOS			= ( BoneInfo.m_BoneStart * BoneMatrix ) * m_Mesh->m_Scale;
	const Vector		BoneEndOS			= ( BoneInfo.m_BoneEnd * BoneMatrix ) * m_Mesh->m_Scale;

	const Vector		BoneStartWS			= BaseLocation + ( BoneStartOS * BaseRotationMatrix );
	const Vector		BoneEndWS			= BaseLocation + ( BoneEndOS * BaseRotationMatrix );

	const Segment		BoneSegmentWS		= Segment( BoneStartWS, BoneEndWS );

	return BoneSegmentWS;
}

HashedString WBCompRosaMesh::GetBoneName( const int BoneIndex ) const
{
	DEVASSERT( m_Mesh );
	if( !m_Mesh->GetBones() )
	{
		return HashedString::NullString;
	}

	DEVASSERT( BoneIndex != INVALID_INDEX );
	DEVASSERT( BoneIndex < m_Mesh->GetBones()->GetNumBones() );

	const SBoneInfo& BoneInfo = m_Mesh->GetBones()->GetBoneInfo( BoneIndex );
	return BoneInfo.m_Name;
}

HashedString WBCompRosaMesh::GetNearestBoneName( const Vector& Location ) const
{
	DEVASSERT( m_Mesh );
	if( !m_Mesh->GetBones() )
	{
		return HashedString::NullString;
	}

	const uint			NearestBoneIndex	= GetNearestBone( Location );
	const SBoneInfo&	NearestBoneInfo		= m_Mesh->GetBones()->GetBoneInfo( NearestBoneIndex );
	return NearestBoneInfo.m_Name;
}

int WBCompRosaMesh::GetNearestBone( const Vector& Location ) const
{
	DEVASSERT( m_Mesh );
	if( !m_Mesh->GetBones() )
	{
		return INVALID_INDEX;
	}

	// Make sure bones have been refreshed this frame.
	// This won't waste any time if it has.
	m_Mesh->UpdateBones();

	const Vector&	BaseLocation		= m_Mesh->m_Location;
	const Matrix	BaseRotationMatrix	= m_Mesh->m_Rotation.ToMatrix();

	uint			NearestBoneIndex	= 0;
	float			NearestDistSq		= FLT_MAX;
	const uint		NumBones			= m_Mesh->GetBones()->GetNumBones();
	for( uint BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex )
	{
		const Matrix&		BoneMatrix		= m_Mesh->GetBoneMatrices()[ BoneIndex ];
		const SBoneInfo&	BoneInfo		= m_Mesh->GetBones()->GetBoneInfo( BoneIndex );

		const Vector		BoneStartOS		= ( BoneInfo.m_BoneStart * BoneMatrix ) * m_Mesh->m_Scale;
		const Vector		BoneEndOS		= ( BoneInfo.m_BoneEnd * BoneMatrix ) * m_Mesh->m_Scale;
		const Vector		BoneStartWS		= BaseLocation + ( BoneStartOS * BaseRotationMatrix );
		const Vector		BoneEndWS		= BaseLocation + ( BoneEndOS * BaseRotationMatrix );
		const Segment		BoneWS			= Segment( BoneStartWS, BoneEndWS );

		const float			DistSq			= BoneWS.DistanceSqTo( Location );
		if( DistSq < NearestDistSq )
		{
			NearestDistSq		= DistSq;
			NearestBoneIndex	= BoneIndex;
		}
	}

	return NearestBoneIndex;
}

// Scale the alpha (additive emissive) relative to the current scene exposure.
Vector4 WBCompRosaMesh::GetExposureRelativeHighlight() const
{
	Vector4 RetVal = m_CurrentHighlight.GetValue();
	RetVal.a *= GetGame()->GetInvExposure();
	return RetVal;
}

void WBCompRosaMesh::SetSendUpdatedEvent()
{
	// HACKHACK: For optimization, only enable the OnMeshUpdated event on entities that need it.
	WBEntity* const				pEntity		= GetEntity();
	WBCompRosaFrobbable* const	pFrobbable	= WB_GETCOMP( pEntity, RosaFrobbable );
	WBCompRosaCollision* const	pCollision	= WB_GETCOMP( pEntity, RosaCollision );
	WBCompRosaSign* const		pSign		= WB_GETCOMP( pEntity, RosaSign );
	WBCompRosaParticles* const	pParticles	= WB_GETCOMP( pEntity, RosaParticles );
	WBCompRosaSound* const		pSound		= WB_GETCOMP( pEntity, RosaSound );

	if( pFrobbable && pFrobbable->GetUseMeshExtents() )
	{
		m_TransformDependentComponents.PushBack( pFrobbable );
	}

	if( pCollision && pCollision->GetUseMeshExtents() )
	{
		m_TransformDependentComponents.PushBack( pCollision );
	}

	if( pSign && pSign->GetUseMeshExtents() )
	{
		m_TransformDependentComponents.PushBack( pSign );
	}

	if( pParticles && pParticles->HasAttachBone() )
	{
		m_AnimationDependentComponents.PushBack( pParticles );
	}

	if( pSound && pSound->HasAttachBone() )
	{
		m_AnimationDependentComponents.PushBack( pSound );
		pSound->SetAttachBoneIndex( m_Mesh->GetBones() );
	}
}

/*virtual*/ void WBCompRosaMesh::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnInitialized );
	STATIC_HASHED_STRING( OnBecameFrobTarget );
	STATIC_HASHED_STRING( OnUnbecameFrobTarget );
	STATIC_HASHED_STRING( Hide );
	STATIC_HASHED_STRING( Show );
	STATIC_HASHED_STRING( HideMesh );
	STATIC_HASHED_STRING( ShowMesh );
	STATIC_HASHED_STRING( HideBaseMesh );
	STATIC_HASHED_STRING( ShowBaseMesh );
	STATIC_HASHED_STRING( HideMeshAttachment );
	STATIC_HASHED_STRING( ShowMeshAttachment );
	STATIC_HASHED_STRING( SetMeshAttachmentTransform );
	STATIC_HASHED_STRING( PlayAnim );
	STATIC_HASHED_STRING( SetAnim );
	STATIC_HASHED_STRING( SetAnimBlend );
	STATIC_HASHED_STRING( CopyAnimations );
	STATIC_HASHED_STRING( SetMesh );
	STATIC_HASHED_STRING( SetTextures );
	STATIC_HASHED_STRING( OnTeleported );
	STATIC_HASHED_STRING( ForceUpdateMesh );
	STATIC_HASHED_STRING( SetAlbedoMap );
	STATIC_HASHED_STRING( SetMeshOffsetZ );
	STATIC_HASHED_STRING( SetTransientMeshOffsetZ );
	STATIC_HASHED_STRING( OnRenderTargetsUpdated );
	STATIC_HASHED_STRING( OnRagdollTicked );
	STATIC_HASHED_STRING( OnRagdollStopped );
	STATIC_HASHED_STRING( SetAnimationBonesTickRate );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnInitialized )
	{
		SetSendUpdatedEvent();
	}
	else if( EventName == sOnBecameFrobTarget )
	{
		Vector4 TargetHighlight;

		STATIC_HASHED_STRING( HighlightRGB );
		TargetHighlight = Event.GetVector( sHighlightRGB );

		STATIC_HASHED_STRING( HighlightA );
		TargetHighlight.a = Event.GetFloat( sHighlightA );

		m_CurrentHighlight.Reset( Interpolator<Vector4>::EIT_EaseIn, m_CurrentHighlight.GetValue(), TargetHighlight, m_HighlightInterpTime );
	}
	else if( EventName == sOnUnbecameFrobTarget )
	{
		m_CurrentHighlight.Reset( Interpolator<Vector4>::EIT_EaseOut, m_CurrentHighlight.GetValue(), kDefaultHighlight, m_HighlightInterpTime );
	}
	else if( EventName == sHide || EventName == sHideMesh )
	{
		m_Hidden = true;
	}
	else if( EventName == sShow || EventName == sShowMesh )
	{
		m_Hidden = false;
	}
	else if( EventName == sHideBaseMesh )
	{
		m_BaseHidden = true;
	}
	else if( EventName == sShowBaseMesh )
	{
		m_BaseHidden = false;
	}
	else if( EventName == sHideMeshAttachment )
	{
		STATIC_HASHED_STRING( Tag );
		const HashedString Tag = Event.GetHash( sTag );

		SetMeshAttachmentsHidden( Tag, true );
	}
	else if( EventName == sShowMeshAttachment )
	{
		STATIC_HASHED_STRING( Tag );
		const HashedString Tag = Event.GetHash( sTag );

		SetMeshAttachmentsHidden( Tag, false );
	}
	else if( EventName == sSetMeshAttachmentTransform )
	{
		STATIC_HASHED_STRING( Tag );
		const HashedString Tag = Event.GetHash( sTag );

		STATIC_HASHED_STRING( Bone );
		const HashedString Bone = Event.GetHash( sBone );

		STATIC_HASHED_STRING( Offset );
		const Vector Offset = Event.GetVector( sOffset );

		STATIC_HASHED_STRING( OrientationOffset );
		const Angles OrientationOffset = Event.GetAngles( sOrientationOffset );

		SetMeshAttachmentTransform( Tag, Bone, Offset, OrientationOffset );
	}
	else if( EventName == sPlayAnim )
	{
		STATIC_HASHED_STRING( AnimationName );
		const HashedString AnimationName = Event.GetHash( sAnimationName );

		STATIC_HASHED_STRING( Loop );
		const bool Loop = Event.GetBool( sLoop );

		STATIC_HASHED_STRING( IgnoreIfAlreadyPlaying );
		const bool IgnoreIfAlreadyPlaying = Event.GetBool( sIgnoreIfAlreadyPlaying );

		STATIC_HASHED_STRING( PlayRate );
		const float PlayRate = Event.GetFloat( sPlayRate );

		STATIC_HASHED_STRING( BlendTime );
		const float BlendTime = Event.GetFloat( sBlendTime );

		STATIC_HASHED_STRING( Layered );
		const bool Layered = Event.GetBool( sLayered );

		STATIC_HASHED_STRING( Additive );
		const bool Additive = Event.GetBool( sAdditive );

		PlayAnimation( AnimationName, Loop, IgnoreIfAlreadyPlaying, PlayRate, BlendTime, Layered, Additive );
	}
	else if( EventName == sSetAnim )
	{
		STATIC_HASHED_STRING( AnimationIndex );
		const int AnimationIndex = Event.GetInt( sAnimationIndex );

		STATIC_HASHED_STRING( AnimationTime );
		const float AnimationTime = Event.GetFloat( sAnimationTime );

		STATIC_HASHED_STRING( AnimationEndBehavior );
		const int AnimationEndBehavior = Event.GetInt( sAnimationEndBehavior );

		STATIC_HASHED_STRING( AnimationPlayRate );
		const float AnimationPlayRate = Event.GetFloat( sAnimationPlayRate );

		AnimationState::SPlayAnimationParams PlayParams;
		PlayParams.m_EndBehavior = static_cast<AnimationState::EAnimationEndBehavior>( AnimationEndBehavior );

		m_Mesh->SetAnimation( AnimationIndex, PlayParams );
		m_Mesh->SetAnimationTime( AnimationTime );
		m_Mesh->SetAnimationPlayRate( AnimationPlayRate > 0.0f ? AnimationPlayRate : 1.0f );
	}
	else if( EventName == sSetAnimBlend )
	{
		STATIC_HASHED_STRING( AnimationNameA );
		const HashedString AnimationNameA = Event.GetHash( sAnimationNameA );

		STATIC_HASHED_STRING( AnimationNameB );
		const HashedString AnimationNameB = Event.GetHash( sAnimationNameB );

		STATIC_HASHED_STRING( BlendAlpha );
		const float BlendAlpha = Event.GetFloat( sBlendAlpha );

		SetAnimationBlend( AnimationNameA, AnimationNameB, BlendAlpha );
	}
	else if( EventName == sCopyAnimations )
	{
		STATIC_HASHED_STRING( SourceEntity );
		WBEntity* const pSourceEntity = Event.GetEntity( sSourceEntity );

		CopyAnimationsFrom( pSourceEntity );
	}
	else if( EventName == sSetMesh )
	{
		STATIC_HASHED_STRING( Mesh );
		const SimpleString Mesh = Event.GetString( sMesh );

		STATIC_HASHED_STRING( AlbedoMap );
		const SimpleString AlbedoMap = Event.GetString( sAlbedoMap );

		STATIC_HASHED_STRING( NormalMap );
		const SimpleString NormalMap = Event.GetString( sNormalMap );

		STATIC_HASHED_STRING( SpecMap );
		const SimpleString SpecMap = Event.GetString( sSpecMap );

		SetMesh( Mesh );
		SetAlbedoMap( AlbedoMap );
		SetNormalMap( NormalMap );
		SetSpecMap( SpecMap );
		UpdateMesh( 0.0f );
	}
	else if( EventName == sSetTextures )
	{
		STATIC_HASHED_STRING( AlbedoMap );
		const SimpleString AlbedoMap = Event.GetString( sAlbedoMap );

		STATIC_HASHED_STRING( NormalMap );
		const SimpleString NormalMap = Event.GetString( sNormalMap );

		STATIC_HASHED_STRING( SpecMap );
		const SimpleString SpecMap = Event.GetString( sSpecMap );

		SetAlbedoMap( AlbedoMap );
		SetNormalMap( NormalMap );
		SetSpecMap( SpecMap );
	}
	else if( EventName == sOnTeleported || EventName == sForceUpdateMesh )
	{
		UpdateMesh( 0.0f );
	}
	else if( EventName == sSetAlbedoMap )
	{
		STATIC_HASHED_STRING( AlbedoMap );
		const SimpleString AlbedoMap = Event.GetString( sAlbedoMap );

		SetAlbedoMap( AlbedoMap );
	}
	else if( EventName == sSetMeshOffsetZ )
	{
		STATIC_HASHED_STRING( OffsetZ );
		m_Offset.z = Event.GetFloat( sOffsetZ );

		UpdateMesh( 0.0f );
	}
	else if( EventName == sSetTransientMeshOffsetZ )
	{
		STATIC_HASHED_STRING( OffsetZ );
		m_TransientOffset.z = Event.GetFloat( sOffsetZ );

		UpdateMesh( 0.0f );
	}
	else if( EventName == sOnRenderTargetsUpdated )
	{
		// Update the g-buffer textures when resolution changes

		RosaFramework* const		pFramework		= RosaFramework::GetInstance();
		RosaTargetManager* const	pTargetManager	= pFramework->GetTargetManager();
		IRenderTarget* const		pPrimaryRT		= pTargetManager->GetRenderTarget( "Primary" );
		IRenderTarget* const		pGB_Albedo		= pTargetManager->GetRenderTarget( "GB_Albedo" );
		IRenderTarget* const		pGB_Normal		= pTargetManager->GetRenderTarget( "GB_Normal" );
		IRenderTarget* const		pGB_Depth		= pTargetManager->GetRenderTarget( "GB_Depth" );
		IRenderTarget* const		pShadows		= pTargetManager->GetRenderTarget( "ShadowCube" );
		ITexture* const				pAlbedoTexture	= pGB_Albedo->GetColorTextureHandle( 0 );
		ITexture* const				pNormalTexture	= pGB_Normal->GetColorTextureHandle( 0 );
		ITexture* const				pDepthTexture	= pGB_Depth->GetColorTextureHandle( 0 );
		ITexture* const				pShadowCube		= pShadows->GetColorTextureHandle( 0 );

		if( m_IsFogLight )
		{
			m_Mesh->GetMultiPassMaterial( 0 ).SetRenderTarget( pGB_Albedo );
			m_Mesh->GetMultiPassMaterial( 1 ).SetRenderTarget( pPrimaryRT );
			m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pAlbedoTexture );
			m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pDepthTexture );
		}
		else if( m_IsShadowLight )
		{
			m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pShadowCube );
			m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pNormalTexture );
			m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 2, pDepthTexture );
			m_Mesh->GetMultiPassMaterial( 2 ).SetTexture( 0, pAlbedoTexture );
			m_Mesh->GetMultiPassMaterial( 2 ).SetTexture( 1, pNormalTexture );
			m_Mesh->GetMultiPassMaterial( 2 ).SetTexture( 2, pDepthTexture );
		}
		else if( m_IsLight )
		{
			m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pAlbedoTexture );
			m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pNormalTexture );
			m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 2, pDepthTexture );
		}
		else if( m_IsAntiLight )
		{
			m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pNormalTexture );
			m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pDepthTexture );
		}
		else if( m_IsDecal )
		{
			m_Mesh->SetTexture( 3, pDepthTexture );	// 0-2 should be albedo/normal/spec maps
		}
		else if( m_IsFogMesh )
		{
			m_Mesh->GetMultiPassMaterial( 0 ).SetRenderTarget( pGB_Albedo );
			m_Mesh->GetMultiPassMaterial( 1 ).SetRenderTarget( pPrimaryRT );
			m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pAlbedoTexture );
			m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pDepthTexture );
		}

		FOR_EACH_ARRAY( AttachedMeshIter, m_AttachedMeshes, SAttachedMesh )
		{
			const SAttachedMesh& AttachedMesh = AttachedMeshIter.GetValue();
			if( AttachedMesh.m_IsFogLight )
			{
				AttachedMesh.m_Mesh->GetMultiPassMaterial( 0 ).SetRenderTarget( pGB_Albedo );
				AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetRenderTarget( pPrimaryRT );
				AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pAlbedoTexture );
				AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pDepthTexture );
			}
			else if( AttachedMesh.m_IsShadowLight )
			{
				AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pShadowCube );
				AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pNormalTexture );
				AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 2, pDepthTexture );
				AttachedMesh.m_Mesh->GetMultiPassMaterial( 2 ).SetTexture( 0, pAlbedoTexture );
				AttachedMesh.m_Mesh->GetMultiPassMaterial( 2 ).SetTexture( 1, pNormalTexture );
				AttachedMesh.m_Mesh->GetMultiPassMaterial( 2 ).SetTexture( 2, pDepthTexture );
			}
			else if( AttachedMesh.m_IsLight )
			{
				AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pAlbedoTexture );
				AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pNormalTexture );
				AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 2, pDepthTexture );
			}
			else if( AttachedMesh.m_IsAntiLight )
			{
				AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pNormalTexture );
				AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pDepthTexture );
			}
			else if( AttachedMesh.m_IsFogMesh )
			{
				AttachedMesh.m_Mesh->GetMultiPassMaterial( 0 ).SetRenderTarget( pGB_Albedo );
				AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetRenderTarget( pPrimaryRT );
				AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 0, pAlbedoTexture );
				AttachedMesh.m_Mesh->GetMultiPassMaterial( 1 ).SetTexture( 1, pDepthTexture );
			}
		}
	}
	else if( EventName == sOnRagdollTicked )
	{
		if( m_Mesh )
		{
			AABB MassBounds;

			STATIC_HASHED_STRING( OSMassBoundsMin );
			MassBounds.m_Min = Event.GetVector( sOSMassBoundsMin );

			STATIC_HASHED_STRING( OSMassBoundsMax );
			MassBounds.m_Max = Event.GetVector( sOSMassBoundsMax );

			STATIC_HASHED_STRING( BlendAlpha );
			const float BlendAlpha = Event.GetFloat( sBlendAlpha );

			// Grow/shrink to the target size over ragdoll blend time
			MassBounds.m_Min = m_OriginalMeshAABB.m_Min.LERP( BlendAlpha, MassBounds.m_Min );
			MassBounds.m_Max = m_OriginalMeshAABB.m_Max.LERP( BlendAlpha, MassBounds.m_Max );

			m_Mesh->SetAABB( MassBounds );
			m_Mesh->RecomputeAABB();
		}
	}
	else if( EventName == sOnRagdollStopped )
	{
		if( m_Mesh )
		{
			// Restore the true original AABB
			m_Mesh->SetAABB( m_OriginalMeshAABB );
			m_Mesh->RecomputeAABB();
		}
	}
	else if( EventName == sSetAnimationBonesTickRate )
	{
		if( m_Mesh )
		{
			STATIC_HASHED_STRING( TickRate );
			const float TickRate = Event.GetFloat( sTickRate );

			m_Mesh->SetAnimationBonesTickRate( TickRate );
		}
	}
}

bool WBCompRosaMesh::IsWithinCullDistance() const
{
	if( m_AlwaysDraw )
	{
		return true;
	}

	if( m_CullDistanceSq == 0.0f )
	{
		return true;
	}

	const Vector	ViewLocation	= RosaGame::GetPlayerViewLocation();
	const Vector	ViewOffset		= m_Mesh->m_Location - ViewLocation;
	const float		DistanceSq		= ViewOffset.LengthSquared();

	return ( DistanceSq < m_CullDistanceSq );
}

bool WBCompRosaMesh::IntersectsAnyVisibleSector() const
{
	PROFILE_FUNCTION;

	if( m_AlwaysDraw )
	{
		return true;
	}

	RosaWorld* const	pWorld				= GetWorld();
	const uint			NumVisibleSectors	= pWorld->GetNumVisibleSectors();

	for( uint Index = 0; Index < NumVisibleSectors; ++Index )
	{
		const AABB& SectorRenderBound = pWorld->GetVisibleSectorRenderBound( Index );

		if( m_Mesh->m_AABB.Intersects( SectorRenderBound ) )
		{
			return true;
		}

		FOR_EACH_ARRAY( AttachedMeshIter, m_AttachedMeshes, SAttachedMesh )
		{
			const SAttachedMesh&	AttachedMesh	= AttachedMeshIter.GetValue();
			const Mesh*				pMesh			= AttachedMesh.m_Mesh;
			if( AttachedMesh.m_OwnBounds && pMesh->m_AABB.Intersects( SectorRenderBound ) )
			{
				return true;
			}
		}
	}

	return false;
}

bool WBCompRosaMesh::IsMeshVisibleFromAnyVisibleSector( const RosaMesh* const pMesh ) const
{
	PROFILE_FUNCTION;

	DEBUGASSERT( pMesh );

	RosaWorld* const pWorld = GetWorld();
	return pWorld->IsLocationVisibleFromAnyVisibleSector( pMesh->m_Location );
}

// I could compute this as the screen area or something, whatever's cheap and works.
float WBCompRosaMesh::GetLightImportanceScalar( const Mesh* const pMesh ) const
{
	DEVASSERT( pMesh );

	const View*						pMainView	= GetFramework()->GetMainView();
	DEVASSERT( pMainView );

	const WBCompRosaLight* const	pLight		= WB_GETCOMP( GetEntity(), RosaLight );
	DEVASSERT( pLight );

	return pLight->GetImportanceScalar( pMesh, pMainView );
}

float WBCompRosaMesh::GetAntiLightImportanceScalar( const Mesh* const pMesh ) const
{
	DEVASSERT( pMesh );

	const View*							pMainView	= GetFramework()->GetMainView();
	DEVASSERT( pMainView );

	const WBCompRosaAntiLight* const	pAntiLight	= WB_GETCOMP( GetEntity(), RosaAntiLight );
	DEVASSERT( pAntiLight );

	return pAntiLight->GetImportanceScalar( pMesh, pMainView );
}

/*virtual*/ void WBCompRosaMesh::Render()
{
	XTRACE_FUNCTION;

	DEVASSERT( m_Mesh );

	if( m_Hidden )
	{
		return;
	}

	if( !IsWithinCullDistance() )
	{
		return;
	}

	if( !IntersectsAnyVisibleSector() )
	{
		return;
	}

	if( m_Mesh->IsAnimated() )
	{
		m_Mesh->UpdateBones();
	}

	// Latently update attached meshes only when rendering, because attached meshes are
	// purely a visual element and we should never care about their transform otherwise.
	if( m_AttachedMeshes.Size() )
	{
		UpdateAttachedMeshes();
	}

	do
	{
		if( m_BaseHidden )
		{
			continue;
		}

		if( m_IsLight || m_IsFogLight )
		{
			const float LightImportanceScalar = GetLightImportanceScalar( m_Mesh );
			if( LightImportanceScalar <= 0.0f )
			{
				continue;
			}
		}
		else if( m_IsAntiLight )
		{
			const float AntiLightImportanceScalar = GetAntiLightImportanceScalar( m_Mesh );
			if( AntiLightImportanceScalar <= 0.0f )
			{
				continue;
			}
		}

		if( m_IsShadowLight )
		{
			if( !IsMeshVisibleFromAnyVisibleSector( m_Mesh ) )
			{
				continue;
			}
		}

		RenderMesh( m_Mesh );
	}
	while(0);	// This loop is only here so I can continue out of it

	FOR_EACH_ARRAY( AttachedMeshIter, m_AttachedMeshes, SAttachedMesh )
	{
		const SAttachedMesh& AttachedMesh = AttachedMeshIter.GetValue();

		if( AttachedMesh.m_Hidden )
		{
			continue;
		}

		if( AttachedMesh.m_IsLight || AttachedMesh.m_IsFogLight )
		{
			const float LightImportanceScalar = GetLightImportanceScalar( AttachedMesh.m_Mesh );
			if( LightImportanceScalar <= 0.0f )
			{
				continue;
			}
		}
		else if( AttachedMesh.m_IsAntiLight )
		{
			const float AntiLightImportanceScalar = GetAntiLightImportanceScalar( AttachedMesh.m_Mesh );
			if( AntiLightImportanceScalar <= 0.0f )
			{
				continue;
			}
		}

		if( AttachedMesh.m_IsShadowLight )
		{
			if( !IsMeshVisibleFromAnyVisibleSector( AttachedMesh.m_Mesh ) )
			{
				continue;
			}
		}

		RenderMesh( AttachedMesh.m_Mesh );
	}
}

void WBCompRosaMesh::RenderMesh( RosaMesh* const pMesh ) const
{
	GetFramework()->GetRenderer()->AddMesh( pMesh );
}

void WBCompRosaMesh::SetMeshAttachmentsHidden( const HashedString& Tag, const bool Hidden )
{
	FOR_EACH_ARRAY( AttachedMeshIter, m_AttachedMeshes, SAttachedMesh )
	{
		SAttachedMesh& AttachedMesh = AttachedMeshIter.GetValue();

		if( AttachedMesh.m_Tag != Tag )
		{
			continue;
		}

		AttachedMesh.m_Hidden = Hidden;
	}
}

void WBCompRosaMesh::SetMeshAttachmentTransform( const HashedString& Tag, const HashedString& Bone, const Vector& TranslationOffset, const Angles& OrientationOffset )
{
	FOR_EACH_ARRAY( AttachedMeshIter, m_AttachedMeshes, SAttachedMesh )
	{
		SAttachedMesh& AttachedMesh = AttachedMeshIter.GetValue();

		if( AttachedMesh.m_Tag != Tag )
		{
			continue;
		}

		SetAttachmentBone( AttachedMesh, Bone );
		AttachedMesh.m_TranslationOffset		= TranslationOffset;
		SetAttachmentOrientationOffset( AttachedMesh, OrientationOffset );
	}
}

#if BUILD_DEV
/*virtual*/ void WBCompRosaMesh::DebugRender( const bool GroupedRender ) const
{
	Super::DebugRender( GroupedRender );

	WBCompRosaTransform* pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	IRenderer* const pRenderer = GetFramework()->GetRenderer();
	const Vector EntityLocation = pTransform->GetLocation();

	pRenderer->DEBUGDrawCross( EntityLocation + GetMeshOffset(), 0.25f, ARGB_TO_COLOR( 255, 255, 255, 0 ) );
	pRenderer->DEBUGDrawBox( m_Mesh->m_AABB.m_Min, m_Mesh->m_AABB.m_Max, ARGB_TO_COLOR( 255, 255, 0, 255 ) );

	FOR_EACH_ARRAY( AttachedMeshIter, m_AttachedMeshes, SAttachedMesh )
	{
		const SAttachedMesh& AttachedMesh = AttachedMeshIter.GetValue();
		if( AttachedMesh.m_OwnBounds )
		{
			pRenderer->DEBUGDrawBox( AttachedMesh.m_Mesh->m_AABB.m_Min, AttachedMesh.m_Mesh->m_AABB.m_Max, ARGB_TO_COLOR( 255, 255, 0, 255 ) );
		}
	}
}
#endif

void WBCompRosaMesh::SetMeshScale( const Vector& Scale )
{
	DEVASSERT( m_Mesh );
	m_Mesh->m_Scale = Scale;
}

void WBCompRosaMesh::PlayAnimation( const HashedString& AnimationName, const bool Loop, const bool IgnoreIfAlreadyPlaying, const float PlayRate, const float BlendTime, const bool Layered, const bool Additive ) const
{
	DEVASSERT( m_Mesh );

	// HACKHACK: Use the default if BlendTime is zero. Use a negative blend time to force no-blend on a mesh with a non-zero default.
	const float UsingBlendTime = ( BlendTime == 0.0f && !Layered && !Additive ) ? m_DefaultAnimBlendTime : Max( 0.0f, BlendTime );

	AnimationState::SPlayAnimationParams PlayParams;
	PlayParams.m_EndBehavior			= Loop ? AnimationState::EAEB_Loop : AnimationState::EAEB_Stop;
	PlayParams.m_IgnoreIfAlreadyPlaying	= IgnoreIfAlreadyPlaying;
	PlayParams.m_PlayRate				= ( PlayRate > 0.0f ) ? PlayRate : 1.0f;
	PlayParams.m_BlendTime				= UsingBlendTime;
	PlayParams.m_Layered				= Layered;
	PlayParams.m_Additive				= Additive;

	m_Mesh->PlayAnimation( AnimationName, PlayParams );
}

void WBCompRosaMesh::SetAnimationBlend( const HashedString& AnimationNameA, const HashedString& AnimationNameB, const float BlendAlpha ) const
{
	DEVASSERT( m_Mesh );

	m_Mesh->SetAnimationBlend( AnimationNameA, AnimationNameB, BlendAlpha );
}

float WBCompRosaMesh::GetAnimationVelocityScalar() const
{
	WBCompStatMod* const pStatMod = WB_GETCOMP( GetEntity(), StatMod );
	WB_MODIFY_FLOAT_SAFE( AnimationVelocity, 1.0f, pStatMod );
	return WB_MODDED( AnimationVelocity );
}

void WBCompRosaMesh::GetAnimationVelocity( Vector& OutVelocity, Angles& OutRotationalVelocity )
{
	DEVASSERT( m_Mesh );

	WBCompRosaTransform* pTransform = GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	m_Mesh->GetAnimationVelocity( OutVelocity, OutRotationalVelocity );

	const float AnimationVelocityScalar = GetAnimationVelocityScalar();
	OutVelocity				*= AnimationVelocityScalar;
	OutRotationalVelocity	*= AnimationVelocityScalar;	// Also scaling rotation because presumably if we're moving half as fast we should also be turning half as fast

	OutVelocity = OutVelocity.RotateBy( pTransform->GetOrientation() );
}

void WBCompRosaMesh::CopyAnimationsFrom( WBEntity* const pSourceEntity ) const
{
	DEVASSERT( pSourceEntity );
	DEVASSERT( m_Mesh );

	WBCompRosaMesh* const pOtherMesh = WB_GETCOMP( pSourceEntity, RosaMesh );
	DEVASSERT( pOtherMesh );

	m_Mesh->CopyAnimationsFrom( pOtherMesh->GetMesh() );
}

/*static*/ void WBCompRosaMesh::NotifyAnimationFinished( void* pVoid, class Mesh* pMesh, class Animation* pAnimation, bool Interrupted )
{
	WBCompRosaMesh* const pThis = static_cast<WBCompRosaMesh*>( pVoid );
	DEVASSERT( pThis );
	pThis->OnAnimationFinished( pMesh, pAnimation, Interrupted );
}

void WBCompRosaMesh::OnAnimationFinished( class Mesh* pMesh, class Animation* pAnimation, bool Interrupted )
{
	Unused( pMesh );

	DEVASSERT( pAnimation );
	DEVASSERT( pMesh == m_Mesh );

	WB_MAKE_EVENT( OnAnimationFinished, GetEntity() );
	WB_SET_AUTO( OnAnimationFinished, Hash, AnimationName, pAnimation->m_AnimData.m_HashedName );
	WB_SET_AUTO( OnAnimationFinished, Bool, Interrupted, Interrupted );
	WB_DISPATCH_EVENT( GetEventManager(), OnAnimationFinished, GetEntity() );
}

#define VERSION_EMPTY				0
#define VERSION_BASE				1
#define VERSION_ANIMATIONSTATE		2
#define VERSION_BASEHIDDEN			3
#define VERSION_CURRENT				3

uint WBCompRosaMesh::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;													// Version
	Size += IDataStream::SizeForWriteString( m_MeshName );		// m_MeshName
	Size += IDataStream::SizeForWriteString( m_AlbedoMapName );	// m_AlbedoMapName
	Size += IDataStream::SizeForWriteString( m_NormalMapName );	// m_NormalMapName
	Size += IDataStream::SizeForWriteString( m_SpecMapName );	// m_SpecMapName
	Size += sizeof( Vector );									// m_Mesh->m_Scale
	Size += 1;													// m_Hidden
	Size += 1;													// m_BaseHidden
	Size += 16;													// Animation state

	Size += sizeof( Vector );									// m_Offset

	Size += 4;													// m_AttachmentSets.Size()
	FOR_EACH_ARRAY( AttachmentSetIter, m_AttachmentSets, SimpleString )
	{
		const SimpleString& AttachmentSet = AttachmentSetIter.GetValue();
		Size += IDataStream::SizeForWriteString( AttachmentSet );
	}

	Size += 4;													// m_AttachedMeshes.Size()
	Size += m_AttachedMeshes.Size() * 1;						// SAttachedMesh::m_Hidden
	Size += m_AttachedMeshes.Size() * sizeof( HashedString );	// SAttachedMesh::m_Bone
	Size += m_AttachedMeshes.Size() * sizeof( Vector );			// SAttachedMesh::m_TranslationOffset
	Size += m_AttachedMeshes.Size() * sizeof( Angles );			// SAttachedMesh::m_OrientationOffset

	return Size;
}

void WBCompRosaMesh::Save( const IDataStream& Stream )
{
	ASSERT( m_Mesh );

	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteString( m_MeshName );
	Stream.WriteString( m_AlbedoMapName );
	Stream.WriteString( m_NormalMapName );
	Stream.WriteString( m_SpecMapName );

	Stream.Write( sizeof( Vector ), &m_Mesh->m_Scale );

	Stream.WriteBool( m_Hidden );
	Stream.WriteBool( m_BaseHidden );

	const bool IsAnimationOwner = m_Mesh->IsAnimationOwner();
	Stream.WriteBool( IsAnimationOwner );
	if( IsAnimationOwner )
	{
		Stream.WriteInt32( m_Mesh->GetAnimationIndex() );
		Stream.WriteFloat( m_Mesh->GetAnimationTime() );
		Stream.WriteInt32( m_Mesh->GetAnimationEndBehavior() );
		Stream.WriteFloat( m_Mesh->GetAnimationPlayRate() );
	}

	Stream.Write( sizeof( Vector ), &m_Offset );

	Stream.WriteUInt32( m_AttachmentSets.Size() );
	FOR_EACH_ARRAY( AttachmentSetIter, m_AttachmentSets, SimpleString )
	{
		const SimpleString& AttachmentSet = AttachmentSetIter.GetValue();
		Stream.WriteString( AttachmentSet );
	}

	Stream.WriteUInt32( m_AttachedMeshes.Size() );
	FOR_EACH_ARRAY( AttachedMeshIter, m_AttachedMeshes, SAttachedMesh )
	{
		const SAttachedMesh& AttachedMesh = AttachedMeshIter.GetValue();
		Stream.WriteBool( AttachedMesh.m_Hidden );
		Stream.WriteHashedString( AttachedMesh.m_Bone );
		Stream.Write<Vector>( AttachedMesh.m_TranslationOffset );
		Stream.Write<Angles>( AttachedMesh.m_OrientationOffset );
	}
}

void WBCompRosaMesh::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	DEVASSERT( m_Mesh );

	// Remove attachments from initialization; we'll restore serialized stuff below
	RemoveAttachments();
	m_AttachmentSets.Clear();

	// Restore costume properties (WBCompRosaCharacterConfig is serialized first, so this is valid)
	WBCompRosaCharacterConfig* const pCharacterConfig = WB_GETCOMP( GetEntity(), RosaCharacterConfig );
	if( pCharacterConfig && pCharacterConfig->HasCostume() )
	{
		InitializeFromCostume();
	}

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_BASE )
	{
		const SimpleString MeshName = Stream.ReadString();
		if( MeshName != m_MeshName )
		{
			// HACKHACK: Save and restore the section from config
			const HashedString Section = m_Mesh->GetSection();
			SetMesh( MeshName );
			m_Mesh->SetSection( Section );
		}
	}

	if( Version >= VERSION_BASE )
	{
		SetAlbedoMap( Stream.ReadString() );
		SetNormalMap( Stream.ReadString() );
		SetSpecMap( Stream.ReadString() );
	}

	if( Version >= VERSION_BASE )
	{
		Stream.Read( sizeof( Vector ), &m_Mesh->m_Scale );
	}

	if( Version >= VERSION_BASE )
	{
		m_Hidden = Stream.ReadBool();
	}

	if( Version >= VERSION_BASEHIDDEN )
	{
		m_BaseHidden = Stream.ReadBool();
	}

	if( Version >= VERSION_BASE )
	{
		const bool		IsAnimationOwner		= ( Version >= VERSION_ANIMATIONSTATE ) ? Stream.ReadBool() : true;
		if( IsAnimationOwner )
		{
			const int	AnimationIndex			= Stream.ReadInt32();
			const float	AnimationTime			= Stream.ReadFloat();
			const int	AnimationEndBehavior	= Stream.ReadInt32();
			const float	AnimationPlayRate		= Stream.ReadFloat();

			if( m_Mesh->IsAnimated() )
			{
				WB_MAKE_EVENT( SetAnim, GetEntity() );
				WB_SET_AUTO( SetAnim, Int,		AnimationIndex,			AnimationIndex );
				WB_SET_AUTO( SetAnim, Float,	AnimationTime,			AnimationTime );
				WB_SET_AUTO( SetAnim, Int,		AnimationEndBehavior,	AnimationEndBehavior );
				WB_SET_AUTO( SetAnim, Float,	AnimationPlayRate,		AnimationPlayRate );
				WB_QUEUE_EVENT( GetEventManager(), SetAnim, GetEntity() );
			}
		}
	}

	if( Version >= VERSION_BASE )
	{
		Stream.Read( sizeof( Vector ), &m_Offset );
	}

	if( Version >= VERSION_BASE )
	{
		// Restore attachment sets
		const uint NumAttachmentSets = Stream.ReadUInt32();
		for( uint AttachmentSetIndex = 0; AttachmentSetIndex < NumAttachmentSets; ++AttachmentSetIndex )
		{
			const SimpleString AttachmentSet = Stream.ReadString();

			const bool FromSerialization = true;
			AddAttachmentSetFromDefinition( AttachmentSet, FromSerialization );
		}
	}

	if( Version >= VERSION_BASE )
	{
		const uint NumAttachedMeshes = Stream.ReadUInt32();
		for( uint AttachedMeshIndex = 0; AttachedMeshIndex < NumAttachedMeshes; ++AttachedMeshIndex )
		{
			const bool			Hidden				= Stream.ReadBool();
			const HashedString	Bone				= Stream.ReadHashedString();
			const Vector		TranslationOffset	= Stream.Read<Vector>();
			const Angles		OrientationOffset	= Stream.Read<Angles>();

			if( AttachedMeshIndex < m_AttachedMeshes.Size() )
			{
				SAttachedMesh& AttachedMesh = m_AttachedMeshes[ AttachedMeshIndex ];

				AttachedMesh.m_Hidden = Hidden;
				SetAttachmentBone( AttachedMesh, Bone );
				AttachedMesh.m_TranslationOffset = TranslationOffset;
				SetAttachmentOrientationOffset( AttachedMesh, OrientationOffset );
			}
		}
	}
}
