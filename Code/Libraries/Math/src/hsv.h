#ifndef HSV_H
#define HSV_H

class Vector;
class Vector4;

namespace HSV
{
	Vector	RGBToHSV( const Vector& ColorRGB );
	Vector	HSVToRGB( const Vector& ColorHSV );

	Vector4	RGBToHSV_AlphaPass( const Vector4& ColorRGB );
	Vector4	HSVToRGB_AlphaPass( const Vector4& ColorHSV );

	Vector	GetConfigHSV( const SimpleString& NameBase, const HashedString& Context, const Vector& DefaultHSV );
	Vector4	GetConfigHSVA( const SimpleString& NameBase, const HashedString& Context, const Vector4& DefaultHSVA );
	Vector	GetConfigRGB( const SimpleString& NameBase, const HashedString& Context, const Vector& DefaultRGB );
	Vector4	GetConfigRGBA( const SimpleString& NameBase, const HashedString& Context, const Vector4& DefaultRGBA );
}

#endif // HSV
