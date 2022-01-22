#include "core.h"
#include "rosaworldtexture.h"
#include "idatastream.h"

RosaWorldTexture::RosaWorldTexture()
:	m_Data()
{
}

RosaWorldTexture::~RosaWorldTexture()
{
}

/*virtual*/ void RosaWorldTexture::LoadDDS( const IDataStream& Stream, STextureData& OutTextureData )
{
	Unused( Stream );
	Unused( OutTextureData );

	WARNDESC( "Compressed files not supported for RosaWorldTexture. Use an RGB format!" );
}

/*virtual*/ void RosaWorldTexture::CreateTexture( const STextureData& TextureData )
{
	// Deep copy
	m_Data.m_Width	= TextureData.m_Width;
	m_Data.m_Height	= TextureData.m_Height;
	m_Data.m_Format	= TextureData.m_Format;
	FOR_EACH_ARRAY( MipIter, TextureData.m_MipChain, TTextureMip )
	{
		m_Data.m_MipChain.PushBack( MipIter.GetValue() );
	}
}
