#include "core.h"
#include "texturecommon.h"
#include "packstream.h"
#include "bmp-format.h"

struct TGAHeader
{
	byte		m_SizeOfIDField;
	byte		m_ColorMapType;
	byte		m_ImageType;		// I'll be concerned with 2 and 10 only

	// Broken up into bytes because of word alignment issues
	byte		m_ColorMapOriginLo;
	byte		m_ColorMapOriginHi;
	byte		m_ColorMapLengthLo;
	byte		m_ColorMapLengthHi;
	byte		m_ColorMapBitDepth;

	c_uint16	m_OriginX;
	c_uint16	m_OriginY;
	c_uint16	m_Width;
	c_uint16	m_Height;
	byte		m_BitDepth;
	byte		m_ImageDescriptor;	// See TGA documentation
};

TextureCommon::TextureCommon()
{
}

TextureCommon::~TextureCommon()
{
}

void TextureCommon::Initialize( const SimpleString& Filename, const bool NoMips )
{
	STextureData TextureData;

	if( Filename.EndsWith( "bmp" ) )
	{
		StaticLoadBMP( PackStream( Filename.CStr() ), TextureData, NoMips );
	}
	else if( Filename.EndsWith( "tga" ) )
	{
		StaticLoadTGA( PackStream( Filename.CStr() ), TextureData, NoMips );
	}
	else if( Filename.EndsWith( "dds" ) )
	{
		LoadDDS( PackStream( Filename.CStr() ), TextureData );
	}
	else
	{
		DEVWARNDESC( "TextureCommon::Initialize: Unknown file extension." );
	}

	CreateTexture( TextureData );
}

/*static*/ uint TextureCommon::StaticCountMipLevels( const uint Width, const uint Height )
{
	uint MipWidth	= Width;
	uint MipHeight	= Height;
	uint MipLevels	= 0;

	while( MipWidth && MipHeight )
	{
		MipWidth	>>= 1;
		MipHeight	>>= 1;
		++MipLevels;
	}

	return MipLevels;
}

/*static*/ void TextureCommon::StaticMakeMip( STextureData& OutTextureData, const uint MipLevel )
{
	DEVASSERT( MipLevel > 0 );

	const uint HiMipLevel	= MipLevel - 1;
	const uint HiWidth		= OutTextureData.m_Width >> HiMipLevel;
	const uint LoWidth		= OutTextureData.m_Width >> MipLevel;
	const uint LoHeight		= OutTextureData.m_Height >> MipLevel;

	// If this case ever happens, just set the dimension to 1
	DEVASSERT( LoWidth && LoHeight );

	DEVASSERT( MipLevel < OutTextureData.m_MipChain.Size() );
	TTextureMip& HiMip = OutTextureData.m_MipChain[ HiMipLevel ];
	TTextureMip& LoMip = OutTextureData.m_MipChain[ MipLevel ];

	DEVASSERT( HiMip.Size() > 0 );
	LoMip.Resize( 4 * LoWidth * LoHeight );

	const byte* const	SrcPixels		= HiMip.GetData();
	byte*				DestPixels		= LoMip.GetData();
	c_uint16			SrcSum;
	const byte*			SrcBoxTopLeft;
	const byte*			SrcBoxTopRight;
	const byte*			SrcBoxBottomLeft;
	const byte*			SrcBoxBottomRight;
	uint				SrcRowOffset	= 4 * HiWidth;
	uint				SrcColOffset	= 4;

	for( uint Y = 0; Y < LoHeight; ++Y )
	{
		for( uint X = 0; X < LoWidth; ++X )
		{
			SrcBoxBottomLeft	= SrcPixels + ( 4 * ( ( X << 1 ) + ( ( Y << 1 ) * HiWidth ) ) );
			SrcBoxBottomRight	= SrcBoxBottomLeft	+ SrcColOffset;
			SrcBoxTopLeft		= SrcBoxBottomLeft	+ SrcRowOffset;
			SrcBoxTopRight		= SrcBoxTopLeft		+ SrcColOffset;

			for( uint C = 0; C < 4; ++C )
			{
				SrcSum = *SrcBoxBottomLeft + *SrcBoxBottomRight + *SrcBoxTopLeft + *SrcBoxTopRight;
				*DestPixels++ = static_cast<byte>( SrcSum >> 2 );

				++SrcBoxBottomLeft;
				++SrcBoxBottomRight;
				++SrcBoxTopLeft;
				++SrcBoxTopRight;
			}
		}
	}
}

/*static*/ void TextureCommon::StaticLoadBMP( const IDataStream& Stream, STextureData& OutTextureData, const bool NoMips )
{
	XTRACE_FUNCTION;

	SBitmapFileHeader	BMPFileHeader;
	SBitmapInfoHeader	BMPInfoHeader;

	Stream.Read( sizeof( SBitmapFileHeader ), &BMPFileHeader );
	Stream.Read( sizeof( SBitmapInfoHeader ), &BMPInfoHeader );

	OutTextureData.m_Format	= EIF_ARGB8;
	OutTextureData.m_Width	= BMPInfoHeader.m_Width;
	OutTextureData.m_Height	= BMPInfoHeader.m_Height;

	const uint NumMips	= NoMips ? 1 : StaticCountMipLevels( OutTextureData.m_Width, OutTextureData.m_Height );
	ASSERT( NumMips > 0 );

	// Resize doesn't construct! So PushBack instead
	for( uint MipLevel = 0; MipLevel < NumMips; ++MipLevel )
	{
		OutTextureData.m_MipChain.PushBack();
	}

	const uint Stride	= ( ( OutTextureData.m_Width + 1 ) * 3 ) & 0xffffffc;

	TTextureMip& BaseMip = OutTextureData.m_MipChain[0];
	BaseMip.Resize( Stride * OutTextureData.m_Height );
	c_uint8* DestPixels	= BaseMip.GetData() + ( Stride * ( OutTextureData.m_Height - 1 ) );

	// Flip image (stored bottom-to-top)
	for( uint Y = 0; Y < OutTextureData.m_Height; ++Y )
	{
		for( uint X = 0; X < OutTextureData.m_Width; ++X )
		{
			Stream.Read( 3, DestPixels );
			DestPixels += 3;
			*DestPixels++ = 0xff;	// Set the alpha byte (instead of loading a 24-bit image and then converting like for BMPs)
		}
		DestPixels -= Stride * 2;
	}

	// Build mip chain as needed
	for( uint MipLevel = 1; MipLevel < NumMips; ++MipLevel )
	{
		StaticMakeMip( OutTextureData, MipLevel );
	}
}

/*static*/ void TextureCommon::StaticLoadTGA( const IDataStream& Stream, STextureData& OutTextureData, const bool NoMips )
{
	XTRACE_FUNCTION;

	TGAHeader Header;

	Stream.Read( sizeof( TGAHeader ), &Header );
	Stream.SetPos( Stream.GetPos() + Header.m_SizeOfIDField );

	if( Header.m_ColorMapType )
	{
		c_uint16 Length = Header.m_ColorMapLengthLo + ( Header.m_ColorMapLengthHi << 8 );
		Stream.SetPos( Stream.GetPos() + ( Length * Header.m_ColorMapBitDepth ) );
	}

	OutTextureData.m_Format	= EIF_ARGB8;
	OutTextureData.m_Width	= Header.m_Width;
	OutTextureData.m_Height	= Header.m_Height;

	const uint NumMips	= NoMips ? 1 : StaticCountMipLevels( OutTextureData.m_Width, OutTextureData.m_Height );
	ASSERT( NumMips > 0 );

	// Resize doesn't construct! So PushBack instead
	for( uint MipLevel = 0; MipLevel < NumMips; ++MipLevel )
	{
		OutTextureData.m_MipChain.PushBack();
	}

	const uint Stride	= OutTextureData.m_Width * 4;

	TTextureMip& BaseMip = OutTextureData.m_MipChain[0];
	BaseMip.Resize( Stride * OutTextureData.m_Height );

	DEVASSERT( Header.m_ImageType == 2 || Header.m_ImageType == 10 );
	if( Header.m_ImageType == 2 )
	{
		// Initialize read pointer to the last row, since TGAs are ordered bottom-to-top
		c_uint8* DestPixels	= BaseMip.GetData() + ( Stride * ( OutTextureData.m_Height - 1 ) );

		if( Header.m_BitDepth == 24 )
		{
			for( uint Y = 0; Y < OutTextureData.m_Height; ++Y )
			{
				for( uint X = 0; X < OutTextureData.m_Width; ++X )
				{
					Stream.Read( 3, DestPixels );
					DestPixels += 3;
					*DestPixels++ = 0xff;	// Set the alpha byte
				}
				DestPixels -= Stride * 2;
			}
		}
		else if( Header.m_BitDepth == 32 )
		{
			for( uint Y = 0; Y < OutTextureData.m_Height; ++Y )
			{
				Stream.Read( Stride, DestPixels );
				DestPixels -= Stride;
			}
		}
		else
		{
			WARNDESC( "TextureCommon::LoadTGA: Unsupported bit depth." );
		}
	}
	else if( Header.m_ImageType == 10 )
	{
		{
			PROFILE_SCOPE( TGA_RLE_Unpack );

			// Initialize read pointer to the first row, read upside down, then flip
			byte* DestPixels = BaseMip.GetData();

			if( Header.m_BitDepth == 24 )
			{
				const uint NumPixels = OutTextureData.m_Width * OutTextureData.m_Height;
				uint ReadPixels = 0;

				while( ReadPixels < NumPixels )
				{
					byte PacketHeader = Stream.ReadUInt8();
					uint PacketSize = static_cast<uint>( PacketHeader & 0x7f ) + 1;

					if( PacketHeader & 0x80 )	// RLE packet
					{
						uint RLEValue = 0xffffffff;
						Stream.Read( 3, &RLEValue );
						const byte* const EndDestPixels = DestPixels + PacketSize * 4;
						for( ; DestPixels < EndDestPixels; DestPixels += 4 )
						{
							uint* const DestPixels32 = reinterpret_cast<uint*>( DestPixels );
							*DestPixels32 = RLEValue;
						}
					}
					else						// Raw packet
					{
						const byte* const EndDestPixels = DestPixels + PacketSize * 4;
						for( ; DestPixels < EndDestPixels; )
						{
							Stream.Read( 3, DestPixels );
							DestPixels += 3;
							*DestPixels++ = 0xff;	// Set the alpha byte
						}
					}

					ReadPixels += PacketSize;
				}
			}
			else if( Header.m_BitDepth == 32 )
			{
				const uint NumPixels = OutTextureData.m_Width * OutTextureData.m_Height;
				uint ReadPixels = 0;

				while( ReadPixels < NumPixels )
				{
					byte PacketHeader = Stream.ReadUInt8();
					uint PacketSize = static_cast<uint>( PacketHeader & 0x7f ) + 1;

					if( PacketHeader & 0x80 )	// RLE packet
					{
						uint RLEValue = Stream.ReadUInt32();
						const byte* const EndDestPixels = DestPixels + PacketSize * 4;
						for( ; DestPixels < EndDestPixels; DestPixels += 4 )
						{
							uint* const DestPixels32 = reinterpret_cast<uint*>( DestPixels );
							*DestPixels32 = RLEValue;
						}
					}
					else						// Raw packet
					{
						Stream.Read( PacketSize * 4, DestPixels );
						DestPixels += PacketSize * 4;
					}

					ReadPixels += PacketSize;
				}
			}
			else
			{
				WARNDESC( "TextureCommon::LoadTGA: Unsupported bit depth." );
			}
		}

		{
			PROFILE_SCOPE( TGA_RLE_Flip );

			// Flip image vertically
			byte* RowT = new byte[ Stride ];
			const int HalfHeight = OutTextureData.m_Height / 2;
			for( int Row = 0; Row < HalfHeight; ++Row )
			{
				byte* const RowA = BaseMip.GetData() + Stride * Row;
				byte* const RowB = BaseMip.GetData() + Stride * ( OutTextureData.m_Height - Row - 1 );
				memcpy( RowT, RowA, Stride );
				memcpy( RowA, RowB, Stride );
				memcpy( RowB, RowT, Stride );
			}
			SafeDeleteArray( RowT );
		}
	}
	else
	{
		WARNDESC( "TextureCommon::LoadTGA: Unsupported image type." );
	}

	// Build mip chain as needed
	for( uint MipLevel = 1; MipLevel < NumMips; ++MipLevel )
	{
		StaticMakeMip( OutTextureData, MipLevel );
	}
}

/*static*/ void TextureCommon::StaticSaveTGA( const IDataStream& Stream, const STextureData& TextureData )
{
	TGAHeader	Header;
	memset( &Header, 0, sizeof( TGAHeader ) );

	Header.m_ImageType			= 2;
	Header.m_Width				= static_cast<c_uint16>( TextureData.m_Width );
	Header.m_Height				= static_cast<c_uint16>( TextureData.m_Height );
	Header.m_BitDepth			= 32;
	Header.m_ImageDescriptor	= 8;

	Stream.Write( sizeof( TGAHeader ), &Header );

	// Only write top level
	const TTextureMip&	Mip		= TextureData.m_MipChain[0];
	const uint			Stride	= TextureData.m_Width * 4;
	for( int Row = TextureData.m_Height - 1; Row >= 0; --Row )
	{
		const uint		Index	= Row * Stride;
		Stream.Write( Stride, &Mip[ Index ] );
	}
}
