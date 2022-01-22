#include "core.h"
#include "3d.h"
#include "uiwidget-image.h"
#include "uimanager.h"
#include "mesh.h"
#include "meshfactory.h"
#include "shadermanager.h"
#include "configmanager.h"
#include "irenderer.h"
#include "mathcore.h"
#include "texturemanager.h"

UIWidgetImage::UIWidgetImage()
:	m_Textures()
,	m_Mesh( NULL )
,	m_Material()
,	m_CircleSides( 0 )
{
}

UIWidgetImage::UIWidgetImage( const SimpleString& DefinitionName )
:	m_Textures()
,	m_Mesh( NULL )
,	m_Material()
,	m_CircleSides( 0 )
{
	InitializeFromDefinition( DefinitionName );
}

UIWidgetImage::~UIWidgetImage()
{
	SafeDelete( m_Mesh );
}

void UIWidgetImage::UpdateRender()
{
	// This will fail if UpdateRender is called between submitting UI meshes to the renderer and actually
	// rendering the scene. It may actually be safe if this widget was not submitted to the renderer.
	DEBUGASSERT( m_UIManager->DEBUGIsSafeToUpdateRender( m_LastRenderedTime ) );

	SafeDelete( m_Mesh );

	IRenderer* pRenderer = m_UIManager->GetRenderer();
	if( m_CircleSides > 0 )
	{
		m_Mesh = pRenderer->GetMeshFactory()->CreateCircleSprite( m_CircleSides );
	}
	else
	{
		m_Mesh = pRenderer->GetMeshFactory()->CreateSprite();
	}
	m_Mesh->SetMaterialDefinition( m_Material, pRenderer );
	FOR_EACH_ARRAY( TextureIter, m_Textures, ITexture* )
	{
		m_Mesh->SetTexture( TextureIter.GetIndex(), TextureIter.GetValue() );
	}
	m_Mesh->SetMaterialFlags( m_RenderInWorld ? MAT_INWORLDHUD : MAT_HUD );
	m_Mesh->m_Location	= Vector( m_Location.x + m_Extents.x * 0.5f, 0.0f, m_Location.y + m_Extents.y * 0.5f );
	m_Mesh->m_Scale		= Vector( m_Extents.x, 1.0f, m_Extents.y );
}

// HACKHACK: For Neon title sequence
/*virtual*/ void UIWidgetImage::Tick( const float DeltaTime )
{
	m_Mesh->Tick( DeltaTime );
}

void UIWidgetImage::Render( bool HasFocus )
{
	XTRACE_FUNCTION;

	UIWidget::Render( HasFocus );

	if( IsDisabled() )
	{
		m_Mesh->m_ConstantColor = m_DisabledColor;
	}
	else if( HasFocus )
	{
		m_Mesh->m_ConstantColor = GetHighlightColor();
	}
	else
	{
		m_Mesh->m_ConstantColor = m_Color;
	}

	STATIC_HASHED_STRING( ScreenColor );
	m_Mesh->SetShaderConstant( sScreenColor, m_ScreenColor );

	// Don't draw mesh if it's going to be invisible
	if( m_Mesh->m_ConstantColor.a > 0.0f )
	{
		m_UIManager->GetRenderer()->AddMesh( m_Mesh );
	}
}

void UIWidgetImage::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	UIWidget::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	// If LoadImage is false, we're expecting to dynamically set the texture in code somewhere
	STATICHASH( LoadImage );
	if( ConfigManager::GetInheritedBool( sLoadImage, true, sDefinitionName ) )
	{
		STATICHASH( NumTextures );
		const uint NumTextures = ConfigManager::GetInheritedInt( sNumTextures, 0, sDefinitionName );
		if( NumTextures > 0 )
		{
			m_Textures.Resize( NumTextures );

			for( uint TextureIndex = 0; TextureIndex < NumTextures; ++TextureIndex )
			{
				const SimpleString Texture = ConfigManager::GetInheritedSequenceString( "Texture%d", TextureIndex, DEFAULT_TEXTURE, sDefinitionName );
				SetTexture( Texture.CStr(), TextureIndex );
			}
		}
		else
		{
			m_Textures.Resize( 1 );

			STATICHASH( Image );
			SetTexture( ConfigManager::GetInheritedString( sImage, DEFAULT_TEXTURE, sDefinitionName ), 0 );
		}
	}
	else
	{
		STATICHASH( NumTextures );
		const uint NumTextures = ConfigManager::GetInheritedInt( sNumTextures, 1, sDefinitionName );
		m_Textures.Resize( NumTextures );
		// NOTE: *Don't* ResizeZero this; because widgets can be reinitialized, that
		// would wipe out textures that have been set, and I don't want to do that.
	}

	STATICHASH( Material );
	const SimpleString DefaultMaterial = m_RenderInWorld ? "Material_HUDInWorld" : "Material_HUD";
	m_Material = ConfigManager::GetInheritedString( sMaterial, DefaultMaterial.CStr(), sDefinitionName );

	STATICHASH( CircleSides );
	m_CircleSides = ConfigManager::GetInheritedInt( sCircleSides, 0, sDefinitionName );

	UpdateRender();
}

void UIWidgetImage::SetTexture( const char* Filename, const uint Index )
{
	DEVASSERT( Filename );
	DEVASSERT( m_Textures.Size() > Index );

	m_Textures[ Index ] = m_UIManager->GetRenderer()->GetTextureManager()->GetTexture( Filename, TextureManager::ETL_Permanent );
}

// HACK: Supporting Eldritch character screen.
// Note that we call UpdateRender here since image widgets don't normally refresh their textures.
void UIWidgetImage::SetTexture( ITexture* const pTexture, const uint Index )
{
	DEVASSERT( m_Textures.Size() > Index );

	m_Textures[ Index ] = pTexture;
	UpdateRender();
}
