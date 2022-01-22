#include "core.h"
#include "rosatargetmanager.h"
#include "3d.h"
#include "irenderer.h"
#include "irendertarget.h"
#include "rosagame.h"
#include "configmanager.h"
#include "rosaframework.h"
#include "Common/uimanagercommon.h"
#include "uiscreen.h"
#include "uiwidget.h"

RosaTargetManager::RosaTargetManager( IRenderer* const pRenderer )
:	TargetManager( pRenderer )
{
}

RosaTargetManager::~RosaTargetManager()
{
}

void RosaTargetManager::CreateTargets( const uint DisplayWidth, const uint DisplayHeight )
{
	TargetManager::CreateTargets( DisplayWidth, DisplayHeight );

	// ROSANOTE: G-buffer layout:
	// NormalX | NormalY | Smoothness* | Reflectance**
	// AlbedoR | AlbedoG | AlbedoB     | Emissive***
	// Depth----------------------------------------
	// * Smoothness is [0,1] and maps to a reasonable range of exponents for specular term.
	// * Smoothness also has NormalZ sign baked in.
	// ** Reflectance is [0,1] and controls lerp between diffuse and specular+reflective terms.
	// ** Reflectance also has Metallic bit baked in.
	// *** Emissive is [0,1] but scaled by a control in the fog params (because why not).
	// *** Emissive also has Skin bit baked in.
	// */**/*** Reflectance, smoothness, emissive, and metallic are authored as RGBA channels in the "spec map".

	// GB_Albedo is also repurposed after light combine for fog light depth (changed from GB_Normal so I still have that for outlines)
	// GB_Albedo is *also* repurposed after fog lights to store edges for post (might be a cheaper way than switching RTs for that?)
	// GB_Normal is also repurposed after edges for fog light depth, again, masking edges

	// ROSANOTE: LAccum is NOT part of g-buffer, so it can be a different bit depth than other buffers.
	IRenderTarget* const pLAccum = m_Renderer->CreateRenderTarget(
		SRenderTargetParams( DisplayWidth, DisplayHeight, ERTF_A16B16G16R16F, ERTF_D24S8 ) );
	m_RenderTargets.Insert( "GB_LAccum", pLAccum );

	IRenderTarget* const pNormal = m_Renderer->CreateRenderTarget(
		SRenderTargetParams( DisplayWidth, DisplayHeight, ERTF_A8R8G8B8, ERTF_None ) );
	m_RenderTargets.Insert( "GB_Normal", pNormal );

	IRenderTarget* const pAlbedo = m_Renderer->CreateRenderTarget(
		SRenderTargetParams( DisplayWidth, DisplayHeight, ERTF_A8R8G8B8, ERTF_None ) );
	m_RenderTargets.Insert( "GB_Albedo", pAlbedo );

	IRenderTarget* const pDepth = m_Renderer->CreateRenderTarget(
		SRenderTargetParams( DisplayWidth, DisplayHeight, ERTF_R32F, ERTF_None ) );
	m_RenderTargets.Insert( "GB_Depth", pDepth );

	IRenderTarget* const pGBuffer = m_Renderer->CreateRenderTarget();
	pGBuffer->AttachColorFrom( pNormal, 0 );
	pGBuffer->AttachColorFrom( pAlbedo, 0 );
	pGBuffer->AttachColorFrom( pDepth, 0 );
	pGBuffer->AttachDepthStencilFrom( pLAccum );
	pGBuffer->FinishAttach();
	m_RenderTargets.Insert( "GBuffer", pGBuffer );

	// Make a decal buffer that renders into normal/albedo only (so it can read depth)
	IRenderTarget* const pDecals = m_Renderer->CreateRenderTarget();
	pDecals->AttachColorFrom( pNormal, 0 );
	pDecals->AttachColorFrom( pAlbedo, 0 );
	pDecals->AttachDepthStencilFrom( pLAccum );	// We don't care about depth but we do want to stencil out foreground
	pDecals->FinishAttach();
	m_RenderTargets.Insert( "Decals", pDecals );

	// Reuse any RGBA g-buffer surface for post compositing
	IRenderTarget* const pPost = m_Renderer->CreateRenderTarget();
	pPost->AttachColorFrom( pNormal, 0 );
	pPost->FinishAttach();
	m_RenderTargets.Insert( "Post", pPost );

	// Reuse any RGBA g-buffer surface for UI compositing, if we can't just do it in the backbuffer
	// (This is only used for fullscreen upscaling, but there's no real overhead to create it.)
	IRenderTarget* const pUI = m_Renderer->CreateRenderTarget();
	pUI->AttachColorFrom( pAlbedo, 0 );
	pUI->FinishAttach();
	m_RenderTargets.Insert( "UI", pUI );

	// Primary RT, where scene is composited (light and materials blended from G-buffer)
	IRenderTarget* const pPrimary = m_Renderer->CreateRenderTarget(
		SRenderTargetParams( DisplayWidth, DisplayHeight, ERTF_A16B16G16R16F, ERTF_None ) );
	m_RenderTargets.Insert( "Primary", pPrimary );

	// Version of primary RT with depth from LAccum attached.
	IRenderTarget* const pPrimaryDepth = m_Renderer->CreateRenderTarget();
	pPrimaryDepth->AttachColorFrom( pPrimary, 0 );
	pPrimaryDepth->AttachDepthStencilFrom( pLAccum );
	pPrimaryDepth->FinishAttach();
	m_RenderTargets.Insert( "PrimaryDepth", pPrimaryDepth );

	// NOTE: If I change the dimensions here, also change in RosaGame::CreateBloomQuads and RosaFramework::CreateHUDView
	m_RenderTargets.Insert( "BloomAH", m_Renderer->CreateRenderTarget(
		SRenderTargetParams( DisplayWidth / ROSA_BLOOMA_SCALE, DisplayHeight / ROSA_BLOOMA_SCALE, ERTF_A16B16G16R16F, ERTF_None ) ) );
	m_RenderTargets.Insert( "BloomAV", m_Renderer->CreateRenderTarget(
		SRenderTargetParams( DisplayWidth / ROSA_BLOOMA_SCALE, DisplayHeight / ROSA_BLOOMA_SCALE, ERTF_A16B16G16R16F, ERTF_None ) ) );
	m_RenderTargets.Insert( "BloomBH", m_Renderer->CreateRenderTarget(
		SRenderTargetParams( DisplayWidth / ROSA_BLOOMB_SCALE, DisplayHeight / ROSA_BLOOMB_SCALE, ERTF_A16B16G16R16F, ERTF_None ) ) );
	m_RenderTargets.Insert( "BloomBV", m_Renderer->CreateRenderTarget(
		SRenderTargetParams( DisplayWidth / ROSA_BLOOMB_SCALE, DisplayHeight / ROSA_BLOOMB_SCALE, ERTF_A16B16G16R16F, ERTF_None ) ) );
	m_RenderTargets.Insert( "BloomCH", m_Renderer->CreateRenderTarget(
		SRenderTargetParams( DisplayWidth / ROSA_BLOOMC_SCALE, DisplayHeight / ROSA_BLOOMC_SCALE, ERTF_A16B16G16R16F, ERTF_None ) ) );
	m_RenderTargets.Insert( "BloomCV", m_Renderer->CreateRenderTarget(
		SRenderTargetParams( DisplayWidth / ROSA_BLOOMC_SCALE, DisplayHeight / ROSA_BLOOMC_SCALE, ERTF_A16B16G16R16F, ERTF_None ) ) );

	UIManagerCommon* const	pUIManager		= RosaFramework::GetInstance()->GetUIManager();
	STATIC_HASHED_STRING( HUD );
	UIScreen* const			pHUD			= pUIManager->GetScreen( sHUD );
	STATIC_HASHED_STRING( Minimap );
	UIWidget* const			pMinimap		= pHUD->GetWidget( sMinimap );
	const uint				MinimapRTWidth	= pMinimap ? static_cast<uint>( pMinimap->GetWidth() ) : 64;	// We need this target even if we don't have a minimap UI element
	const uint				MinimapRTHeight	= pMinimap ? static_cast<uint>( pMinimap->GetHeight() ) : 64;	// We need this target even if we don't have a minimap UI element
	m_RenderTargets.Insert( "MinimapA", m_Renderer->CreateRenderTarget(
		SRenderTargetParams( MinimapRTWidth, MinimapRTHeight, ERTF_A16B16G16R16F, ERTF_D24S8 ) ) );
	m_RenderTargets.Insert( "MinimapB", m_Renderer->CreateRenderTarget(
		SRenderTargetParams( MinimapRTWidth, MinimapRTHeight, ERTF_A16B16G16R16F, ERTF_None ) ) );

#if ROSA_USE_MAXIMAP
	STATIC_HASHED_STRING( Maximap );
	UIWidget* const			pMaximap		= pHUD->GetWidget( sMaximap );
	const uint				MaximapRTWidth	= pMaximap ? static_cast<uint>( pMaximap->GetWidth() ) : 64;	// We need this target even if we don't have a maximap UI element
	const uint				MaximapRTHeight	= pMaximap ? static_cast<uint>( pMaximap->GetHeight() ) : 64;	// We need this target even if we don't have a maximap UI element
	m_RenderTargets.Insert( "MaximapA", m_Renderer->CreateRenderTarget(
		SRenderTargetParams( MaximapRTWidth, MaximapRTHeight, ERTF_A16B16G16R16F, ERTF_D24S8 ) ) );
	m_RenderTargets.Insert( "MaximapB", m_Renderer->CreateRenderTarget(
		SRenderTargetParams( MaximapRTWidth, MaximapRTHeight, ERTF_A16B16G16R16F, ERTF_None ) ) );
#endif

	// TODO: Configurate shadow map size in case this is a problem for lower end hardware
	// (A 512*512*6 cube is about 75% of another 1080p G-buffer layer, for example.)
	static const uint skShadowMapSize = 512;
	m_RenderTargets.Insert( "ShadowCube", m_Renderer->CreateCubeRenderTarget(
		SRenderTargetParams( skShadowMapSize, skShadowMapSize, ERTF_R32F, ERTF_D24S8 ) ) );
}
