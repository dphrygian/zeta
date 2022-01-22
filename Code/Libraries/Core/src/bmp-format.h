#ifndef BMPFORMAT_H
#define BMPFORMAT_H

// Mirrors RGBQUAD
struct SRGBQuad
{
	byte	m_Blue;
	byte	m_Green;
	byte	m_Red;
	byte	m_Padding;
};

// Mirrors BITMAPINFOHEADER
struct SBitmapInfoHeader
{
	uint		m_Size;
	int			m_Width;
	int			m_Height;
	c_uint16	m_Planes;
	c_uint16	m_BitCount;
	uint		m_Compression;
	uint		m_SizeImage;
	int			m_PixelsPerMeterX;
	int			m_PixelsPerMeterY;
	uint		m_ColorUsed;
	uint		m_ColorImportant;
};

// Mirrors the additional data in e.g. BITMAPV5HEADER
struct SBitmapInfoHeaderExt
{
	uint	m_RedMask;
	uint	m_GreenMask;
	uint	m_BlueMask;
	uint	m_AlphaMask;
	uint	m_ColorSpaceType;
	int		m_EndpointsX;
	int		m_EndpointsY;
	int		m_EndpointsZ;
	uint	m_GammaRed;
	uint	m_GammaGreen;
	uint	m_GammaBlue;
	uint	m_Intent;
	uint	m_ProfileData;
	uint	m_ProfileSize;
	uint	m_Reserved;
};

// Mirrors BITMAPINFO
struct SBitmapInfo
{
	SBitmapInfoHeader	m_Header;
	SRGBQuad			m_Colors;
};

// Mirrors BITMAPFILEHEADER
// 2-byte aligned
#pragma pack(push, 2)
struct SBitmapFileHeader
{
	c_uint16	m_Type;
	uint		m_Size;
	c_uint16	m_Reserved1;
	c_uint16	m_Reserved2;
	uint		m_OffsetBits;
};
#pragma pack(pop)

#endif // BMPFORMAT_H
