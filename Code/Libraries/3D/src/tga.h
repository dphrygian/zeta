#ifndef TGA_H
#define TGA_H

// Image types:
// 0  -  No image data included.
// 1  -  Uncompressed, color-mapped images.
// 2  -  Uncompressed, RGB images.
// 3  -  Uncompressed, black and white images.
// 9  -  Runlength encoded color-mapped images.
// 10  -  Runlength encoded RGB images.
// 11  -  Compressed, black and white images.
// 32  -  Compressed color-mapped data, using Huffman, Delta, and runlength encoding.
// 33  -  Compressed color-mapped data, using Huffman, Delta, and runlength encoding.  4-pass quadtree-type process.

class IDataStream;

namespace TGA
{
	enum EImageType
	{
		EIT_None = 0,
		EIT_Unc_Map = 1,
		EIT_Unc_RGB = 2,
		EIT_Unc_BW = 3,
		EIT_RLE_Map = 9,
		EIT_RLE_RBG = 10,
		EIT_Cmp_BW = 11,
		EIT_Cmp_Map = 32,
		EIT_Cmp_Map4 = 33,
	};

	struct SHeader
	{
		SHeader()
		:	m_IDLength( 0 )
		,	m_ColorMapType( 0 )
		,	m_ImageType( 0 )
		,	m_ColorMapIndex( 0 )
		,	m_ColorMapNum( 0 )
		,	m_ColorMapDepth( 0 )
		,	m_OriginX( 0 )
		,	m_OriginY( 0 )
		,	m_Width( 0 )
		,	m_Height( 0 )
		,	m_BPP( 0 )
		,	m_Flags( 0 )
		{
		}

		c_int8	m_IDLength;
		c_int8	m_ColorMapType;
		c_int8	m_ImageType;		// One of EImageType
		c_int16	m_ColorMapIndex;
		c_int16	m_ColorMapNum;
		c_int8	m_ColorMapDepth;
		c_int16	m_OriginX;
		c_int16	m_OriginY;
		c_int16	m_Width;
		c_int16	m_Height;
		c_int8	m_BPP;
		c_int8	m_Flags;
	};

	void Load( const IDataStream& Stream, SHeader& InOutHeader );
}

#endif // TGA_H
