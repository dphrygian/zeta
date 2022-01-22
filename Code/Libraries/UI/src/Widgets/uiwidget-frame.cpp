#include "core.h"
#include "uiwidget-frame.h"
#include "ivertexbuffer.h"
#include "iindexbuffer.h"
#include "ivertexdeclaration.h"
#include "uimanager.h"
#include "mesh.h"
#include "meshfactory.h"
#include "shadermanager.h"
#include "configmanager.h"
#include "irenderer.h"
#include "mathcore.h"
#include "texturemanager.h"

UIWidgetFrame::UIWidgetFrame()
:	m_Texture( NULL )
,	m_Border( 0.0f )
,	m_Mesh( NULL )
,	m_Material()
{
}

UIWidgetFrame::~UIWidgetFrame()
{
	SafeDelete( m_Mesh );
}

Mesh* UIWidgetFrame::CreateMesh() const
{
	static const uint kNumVertices	= 16;
	static const uint kNumIndices	= 54;

	const Vector TopLeft		= Vector( m_Location.x, 0.0f, m_Location.y );
	const Vector BottomRight	= TopLeft + Vector( m_Extents.x, 0.0f, m_Extents.y );

	Array<Vector> Positions;
	Positions.Reserve( kNumVertices );

	{
		Array<float> PositionsX;
		PositionsX.Reserve( 4 );

		PositionsX.PushBack( TopLeft.x );
		PositionsX.PushBack( TopLeft.x + m_Border );
		PositionsX.PushBack( BottomRight.x - m_Border );
		PositionsX.PushBack( BottomRight.x );

		Array<float> PositionsY;
		PositionsY.Reserve( 4 );

		PositionsY.PushBack( TopLeft.z );
		PositionsY.PushBack( TopLeft.z + m_Border );
		PositionsY.PushBack( BottomRight.z - m_Border );
		PositionsY.PushBack( BottomRight.z );

		for( uint Y = 0; Y < 4; ++Y )
		{
			for( uint X = 0; X < 4; ++X )
			{
				Positions.PushBack( Vector( PositionsX[ X ], 0.0f, PositionsY[ Y ] ) );
			}
		}
	}

	Array<Vector2> UVs;
	UVs.Reserve( kNumVertices );

	{
		Array<float> UVValues;
		UVValues.Reserve( 4 );

		UVValues.PushBack( 0.0f );
		UVValues.PushBack( 0.25f );
		UVValues.PushBack( 0.75f );
		UVValues.PushBack( 1.0f );

		for( uint V = 0; V < 4; ++V )
		{
			for( uint U = 0; U < 4; ++U )
			{
				UVs.PushBack( Vector2( UVValues[ U ], UVValues[ V ] ) );
			}
		}
	}

	Array<uint> Colors;
	Colors.Reserve( kNumVertices );

	{
		for( uint I = 0; I < kNumVertices; ++I )
		{
			Colors.PushBack( 0xffffffff );
		}
	}

	Array<index_t> Indices;
	Indices.Reserve( kNumIndices );

	for( index_t Y = 0; Y < 3; ++Y )
	{
		for( index_t X = 0; X < 3; ++X )
		{
			const index_t Base = X + Y * 4;
			Indices.PushBack( Base + 0 );
			Indices.PushBack( Base + 4 );
			Indices.PushBack( Base + 1 );
			Indices.PushBack( Base + 4 );
			Indices.PushBack( Base + 5 );
			Indices.PushBack( Base + 1 );
		}
	}

	IRenderer* const			pRenderer			= m_UIManager->GetRenderer();
	IVertexBuffer* const		VertexBuffer		= pRenderer->CreateVertexBuffer();
	IVertexDeclaration* const	VertexDeclaration	= pRenderer->GetVertexDeclaration( VD_POSITIONS | VD_UVS | VD_COLORS );
	IIndexBuffer* const			IndexBuffer			= pRenderer->CreateIndexBuffer();

	IVertexBuffer::SInit InitStruct;
	InitStruct.NumVertices	= kNumVertices;
	InitStruct.Positions	= Positions.GetData();
	InitStruct.UVs			= UVs.GetData();
	InitStruct.Colors		= Colors.GetData();

	VertexBuffer->Init( InitStruct );
	IndexBuffer->Init( kNumIndices, Indices.GetData() );
	IndexBuffer->SetPrimitiveType( EPT_TRIANGLELIST );

	Mesh* const pMesh = new Mesh( VertexBuffer, VertexDeclaration, IndexBuffer );

	pMesh->SetAABB( AABB( TopLeft, BottomRight ) );

#if BUILD_DEBUG
	pMesh->m_DEBUG_Name = "UI Frame";
#endif

	return pMesh;
}

void UIWidgetFrame::UpdateRender()
{
	DEBUGASSERT( m_UIManager->DEBUGIsSafeToUpdateRender( m_LastRenderedTime ) );

	SafeDelete( m_Mesh );

	IRenderer* const pRenderer = m_UIManager->GetRenderer();
	m_Mesh = CreateMesh();
	m_Mesh->SetMaterialDefinition( m_Material, pRenderer );
	m_Mesh->SetTexture( 0, m_Texture );
	m_Mesh->SetMaterialFlags( m_RenderInWorld ? MAT_INWORLDHUD : MAT_HUD );
}

void UIWidgetFrame::Render( bool HasFocus )
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

void UIWidgetFrame::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	UIWidget::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( DisplayWidth );
	const float DisplayWidth	= ConfigManager::GetFloat( sDisplayWidth );
	const float ParentWidth		= m_ParentWidget ? m_ParentWidget->GetWidth() : DisplayWidth;

	STATICHASH( DisplayHeight );
	const float DisplayHeight	= ConfigManager::GetFloat( sDisplayHeight );
	const float ParentHeight	= m_ParentWidget ? m_ParentWidget->GetHeight() : DisplayHeight;

	STATICHASH( PixelBorder );
	STATICHASH( ParentHBorder );
	STATICHASH( ScreenHBorder );
	STATICHASH( ParentWBorder );
	STATICHASH( ScreenWBorder );
	m_Border = Pick(
		ConfigManager::GetInheritedFloat( sPixelBorder, 0.0f, sDefinitionName ),
		ParentHeight	* ConfigManager::GetInheritedFloat( sParentHBorder, 0.0f, sDefinitionName ),
		DisplayHeight	* ConfigManager::GetInheritedFloat( sScreenHBorder, 0.0f, sDefinitionName ),
		ParentWidth		* ConfigManager::GetInheritedFloat( sParentWBorder, 0.0f, sDefinitionName ),
		DisplayWidth	* ConfigManager::GetInheritedFloat( sScreenWBorder, 0.0f, sDefinitionName ) );

	ASSERT( m_Extents.x > m_Border * 2.0f );
	ASSERT( m_Extents.y > m_Border * 2.0f );

	STATICHASH( Image );
	const char* const Filename = ConfigManager::GetInheritedString( sImage, DEFAULT_TEXTURE, sDefinitionName );
	m_Texture = m_UIManager->GetRenderer()->GetTextureManager()->GetTexture( Filename, TextureManager::ETL_Permanent );

	STATICHASH( Material );
	const SimpleString DefaultMaterial = m_RenderInWorld ? "Material_HUDInWorld" : "Material_HUD";
	m_Material = ConfigManager::GetInheritedString( sMaterial, DefaultMaterial.CStr(), sDefinitionName );

	UpdateRender();
}
