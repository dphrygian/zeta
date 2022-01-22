#include "core.h"
#include "hsv.h"
#include "vector.h"
#include "vector4.h"
#include "mathcore.h"
#include "configmanager.h"

// NOTE: For HSV, x is hue, y is saturation, and z is value.
// Many sources encode hue in [0, 360], but I'm using [0, 1].

// NOTE: This actually seems to work ok for RGB values greater than 1.0,
// but I may just have gotten lucky so far. It seems like it would depend
// on the chroma (max - min) not being greater than 1.0.
Vector HSV::RGBToHSV( const Vector& ColorRGB )
{
	Vector ColorHSV;

	const float MaxValue	= Max( ColorRGB.r, Max( ColorRGB.g, ColorRGB.b ) );

	// Value is the greatest component of RGB
	ColorHSV.z = MaxValue;

	if( MaxValue < EPSILON )
	{
		// Without value, hue is undefined and saturation is 0.
		ColorHSV.x = 0.0f;
		ColorHSV.y = 0.0f;
		return ColorHSV;
	}

	const float MinValue	= Min( ColorRGB.r, Min( ColorRGB.g, ColorRGB.b ) );
	const float Chroma		= MaxValue - MinValue;	// Roughly, relative colorfulness of components

	// Saturation is normalized chroma
	ColorHSV.y = Chroma / MaxValue;

	if( Chroma < EPSILON )
	{
		// Without chroma, hue is undefined.
		ColorHSV.x = 0.0f;
		return ColorHSV;
	}

	// Do in 0,360 first, then divide. Figure out where I'm going wrong.
	static const float kHueScalar	= 1.0f / 6.0f;
	static const float kGreenShift	= 1.0f / 3.0f;
	static const float kBlueShift	= 2.0f / 3.0f;
	if( MaxValue == ColorRGB.r )
	{
		ColorHSV.x = ( ( ColorRGB.g - ColorRGB.b ) / Chroma ) * kHueScalar;
		if( ColorHSV.x < 0.0f )
		{
			ColorHSV.x += 1.0f;
		}
	}
	else if( MaxValue == ColorRGB.g )
	{
		ColorHSV.x = kGreenShift + ( ( ( ColorRGB.b - ColorRGB.r ) / Chroma ) * kHueScalar );
	}
	else
	{
		ColorHSV.x = kBlueShift + ( ( ( ColorRGB.r - ColorRGB.g ) / Chroma ) * kHueScalar );
	}

	return ColorHSV;
}

inline float HSVMod( float F )
{
	while( F < 0.0f )
	{
		F += 1.0f;
	}
	while( F > 1.0f )
	{
		F -= 1.0f;
	}
	return F;
}

Vector HSV::HSVToRGB( const Vector& ColorHSV )
{
	Vector ColorRGB;

	const float Hue			= HSVMod( ColorHSV.x );
	const float Chroma		= ColorHSV.z * ColorHSV.y;
	const float MinValue	= ColorHSV.z - Chroma;
	const float MidValue	= MinValue + Chroma * ( 1.0f - Abs( Mod( Hue * 6.0f, 2.0f ) - 1.0f ) );	// wat
	const float MaxValue	= ColorHSV.z;

	if( Hue < ( 1.0f / 6.0f ) )
	{
		ColorRGB.r = MaxValue;
		ColorRGB.g = MidValue;
		ColorRGB.b = MinValue;
	}
	else if( Hue < ( 2.0f / 6.0f ) )
	{
		ColorRGB.r = MidValue;
		ColorRGB.g = MaxValue;
		ColorRGB.b = MinValue;
	}
	else if( Hue < ( 3.0f / 6.0f ) )
	{
		ColorRGB.r = MinValue;
		ColorRGB.g = MaxValue;
		ColorRGB.b = MidValue;
	}
	else if( Hue < ( 4.0f / 6.0f ) )
	{
		ColorRGB.r = MinValue;
		ColorRGB.g = MidValue;
		ColorRGB.b = MaxValue;
	}
	else if( Hue < ( 5.0f / 6.0f ) )
	{
		ColorRGB.r = MidValue;
		ColorRGB.g = MinValue;
		ColorRGB.b = MaxValue;
	}
	else
	{
		ColorRGB.r = MaxValue;
		ColorRGB.g = MinValue;
		ColorRGB.b = MidValue;
	}

	return ColorRGB;
}

Vector4 HSV::RGBToHSV_AlphaPass( const Vector4& ColorRGB )
{
	return Vector4( RGBToHSV( Vector( ColorRGB ) ), ColorRGB.a );
}

Vector4 HSV::HSVToRGB_AlphaPass( const Vector4& ColorHSV )
{
	return Vector4( HSVToRGB( Vector( ColorHSV ) ), ColorHSV.a );
}

Vector HSV::GetConfigHSV( const SimpleString& NameBase, const HashedString& Context, const Vector& DefaultHSV )
{
	Vector OutHSV;

	MAKEHASHFROM( NameHD, NameBase + "HD" );
	OutHSV.x = ConfigManager::GetInheritedFloat( sNameHD, DefaultHSV.x * 360.0f, Context ) / 360.0f;

	MAKEHASHFROM( NameH, NameBase + "H" );
	OutHSV.x = ConfigManager::GetInheritedFloat( sNameH, OutHSV.x, Context );

	MAKEHASHFROM( NameS, NameBase + "S" );
	OutHSV.y = ConfigManager::GetInheritedFloat( sNameS, DefaultHSV.y, Context );

	MAKEHASHFROM( NameV, NameBase + "V" );
	OutHSV.z = ConfigManager::GetInheritedFloat( sNameV, DefaultHSV.z, Context );

	return OutHSV;
}

Vector4 HSV::GetConfigHSVA( const SimpleString& NameBase, const HashedString& Context, const Vector4& DefaultHSVA )
{
	Vector4 OutHSVA;

	MAKEHASHFROM( NameHD, NameBase + "HD" );
	OutHSVA.x = ConfigManager::GetInheritedFloat( sNameHD, DefaultHSVA.x * 360.0f, Context ) / 360.0f;

	MAKEHASHFROM( NameH, NameBase + "H" );
	OutHSVA.x = ConfigManager::GetInheritedFloat( sNameH, OutHSVA.x, Context );

	MAKEHASHFROM( NameS, NameBase + "S" );
	OutHSVA.y = ConfigManager::GetInheritedFloat( sNameS, DefaultHSVA.y, Context );

	MAKEHASHFROM( NameV, NameBase + "V" );
	OutHSVA.z = ConfigManager::GetInheritedFloat( sNameV, DefaultHSVA.z, Context );

	MAKEHASHFROM( NameA, NameBase + "A" );
	OutHSVA.w = ConfigManager::GetInheritedFloat( sNameA, DefaultHSVA.w, Context );

	return OutHSVA;
}

Vector HSV::GetConfigRGB( const SimpleString& NameBase, const HashedString& Context, const Vector& DefaultRGB )
{
	Vector OutRGB;

	MAKEHASHFROM( NameR, NameBase + "R" );
	OutRGB.x = ConfigManager::GetInheritedFloat( sNameR, DefaultRGB.x, Context );

	MAKEHASHFROM( NameG, NameBase + "G" );
	OutRGB.y = ConfigManager::GetInheritedFloat( sNameG, DefaultRGB.y, Context );

	MAKEHASHFROM( NameB, NameBase + "B" );
	OutRGB.z = ConfigManager::GetInheritedFloat( sNameB, DefaultRGB.z, Context );

	return OutRGB;
}

Vector4 HSV::GetConfigRGBA( const SimpleString& NameBase, const HashedString& Context, const Vector4& DefaultRGBA )
{
	Vector4 OutRGBA;

	MAKEHASHFROM( NameR, NameBase + "R" );
	OutRGBA.x = ConfigManager::GetInheritedFloat( sNameR, DefaultRGBA.x, Context );

	MAKEHASHFROM( NameG, NameBase + "G" );
	OutRGBA.y = ConfigManager::GetInheritedFloat( sNameG, DefaultRGBA.y, Context );

	MAKEHASHFROM( NameB, NameBase + "B" );
	OutRGBA.z = ConfigManager::GetInheritedFloat( sNameB, DefaultRGBA.z, Context );

	MAKEHASHFROM( NameA, NameBase + "A" );
	OutRGBA.w = ConfigManager::GetInheritedFloat( sNameA, DefaultRGBA.w, Context );

	return OutRGBA;
}
