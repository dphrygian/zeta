#include "core.h"
#include "rosaworldcubemap.h"
#include "idatastream.h"

RosaWorldCubemap::RosaWorldCubemap()
:	m_Data()
,	m_Irradiance()
,	m_APICubemap( NULL )
{
}

RosaWorldCubemap::~RosaWorldCubemap()
{
	SafeDelete( m_APICubemap );
}

void RosaWorldCubemap::SetAPICubemap( ITexture* const pCubemap )
{
	SafeDelete( m_APICubemap );
	m_APICubemap = pCubemap;
}

/*virtual*/ void RosaWorldCubemap::LoadDDS( const IDataStream& Stream, STextureData& OutTextureData )
{
	Unused( Stream );
	Unused( OutTextureData );

	WARNDESC( "Compressed files not supported for RosaWorldCubemap. Use an RGB format!" );
}

/*virtual*/ void RosaWorldCubemap::CreateCubemap( const SCubemapData& CubemapData )
{
	// Deep copy
	for( uint Face = 0; Face < 6; ++Face )
	{
		m_Data.m_Textures[ Face ].m_Width	= CubemapData.m_Textures[ Face ].m_Width;
		m_Data.m_Textures[ Face ].m_Height	= CubemapData.m_Textures[ Face ].m_Height;
		m_Data.m_Textures[ Face ].m_Format	= CubemapData.m_Textures[ Face ].m_Format;
		FOR_EACH_ARRAY( MipIter, CubemapData.m_Textures[ Face ].m_MipChain, TTextureMip )
		{
			m_Data.m_Textures[ Face ].m_MipChain.PushBack( MipIter.GetValue() );
		}
	}
}
