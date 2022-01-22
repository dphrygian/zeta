#ifndef MATHCORE_H
#define MATHCORE_H

#include <math.h>	// For sqrtf, etc.
#include <float.h>	// For FLT_MIN/MAX
#include <limits.h>	// For INT_MIN/MAX, etc.

#define EPSILON 0.0001f
#define SMALLER_EPSILON 1.0e-10f	// ^___________________^;

#define DEGREES_TO_RADIANS( d ) ( ( d ) * 0.01745329252f )
#define RADIANS_TO_DEGREES( r ) ( ( r ) * 57.2957795131f )

#define D2R( d ) DEGREES_TO_RADIANS( d )
#define R2D( r ) RADIANS_TO_DEGREES( r )

#define HALFPI		1.57079632679f
#define PI			3.14159265359f
#define TWOPI		6.28318530718f
#define ONEOVERPI	0.31830988618f

const float kAspect_4_3		= 4.0f / 3.0f;
const float kAspect_16_9	= 16.0f / 9.0f;
const float kAspect_16_10	= 16.0f / 10.0f;

#define MAbs( a )			( ( ( a ) < 0 ) ? -( a ) : ( a ) )
#define MMax( a, b )		( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )
#define MMin( a, b )		( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )
#define MClamp( v, l, h )	( ( ( v ) < ( l ) ) ? ( l ) : ( ( v ) > ( h ) ) ? ( h ) : ( v ) )

inline void Swap( byte& A, byte& B ) { byte T = A; A = B; B = T; }
inline void Swap( char& A, char& B ) { char T = A; A = B; B = T; }

inline uint Min( const uint A, const uint B ) { return ( A < B ) ? A : B; }
inline uint Max( const uint A, const uint B ) { return ( A > B ) ? A : B; }
inline uint Pick( const uint A, const uint B ) { return A != 0 ? A : B; }	// Not a standard operator, just something useful for me
inline uint Pick( const uint A, const uint B, const uint C ) { return A != 0 ? A : Pick( B, C ); }
inline uint Pick( const uint A, const uint B, const uint C, const uint D ) { return A != 0 ? A : Pick( B, C, D ); }
inline uint Pick( const uint A, const uint B, const uint C, const uint D, const uint E ) { return A != 0 ? A : Pick( B, C, D, E ); }
inline uint Clamp( const uint Value, const uint Low, const uint High ) { return ( Value < Low ) ? Low : ( Value > High ) ? High : Value; }
inline void Swap( uint& A, uint& B ) { uint T = A; A = B; B = T; }
inline uint Square( const uint V ) { return V * V; }
inline uint RoundToUInt( const float F ) { return static_cast<uint>( F + 0.5f ); }

inline int Abs( const int A ) { return ( A < 0 ) ? -A : A; }
inline int Min( const int A, const int B ) { return ( A < B ) ? A : B; }
inline int Max( const int A, const int B ) { return ( A > B ) ? A : B; }
inline int Pick( const int A, const int B ) { return A != 0 ? A : B; }	// Not a standard operator, just something useful for me
inline int Pick( const int A, const int B, const int C ) { return A != 0 ? A : Pick( B, C ); }
inline int Pick( const int A, const int B, const int C, const int D ) { return A != 0 ? A : Pick( B, C, D ); }
inline int Pick( const int A, const int B, const int C, const int D, const int E ) { return A != 0 ? A : Pick( B, C, D, E ); }
inline int Clamp( const int Value, const int Low, const int High ) { return ( Value < Low ) ? Low : ( Value > High ) ? High : Value; }
inline void Swap( int& A, int& B ) { int T = A; A = B; B = T; }
inline int Square( const int V ) { return V * V; }
inline int Mod( const int N, const int D ) { int R = N % D; if( R < 0 ) return R + D; return R; } // Handles negative numbers the way I want, i.e., (-7) % 8 == 1
inline int Sign( const int N ) { return ( N > 0 ) ? 1 : ( ( N < 0 ) ? -1 : 0 ); }
inline int RoundToInt( const float F ) { return static_cast<int>( F + 0.5f ); }

inline float Abs( const float A ) { return ( A < 0.0f ) ? -A : A; }
inline float Min( const float A, const float B ) { return ( A < B ) ? A : B; }
inline float Max( const float A, const float B ) { return ( A > B ) ? A : B; }
inline void MinMax( const float A, const float B, float& OutMin, float& OutMax ) { if( A < B ) { OutMin = A; OutMax = B; } else { OutMin = B; OutMax = A; } }
inline float Pick( const float A, const float B ) { return A != 0.0f ? A : B; }	// Not a standard operator, just something useful for me
inline float Pick( const float A, const float B, const float C ) { return A != 0.0f ? A : Pick( B, C ); }
inline float Pick( const float A, const float B, const float C, const float D ) { return A != 0.0f ? A : Pick( B, C, D ); }
inline float Pick( const float A, const float B, const float C, const float D, const float E ) { return A != 0.0f ? A : Pick( B, C, D, E ); }
inline float Clamp( const float Value, const float Low, const float High ) { return ( Value < Low ) ? Low : ( Value > High ) ? High : Value; }
inline bool Equal( const float A, const float B, const float E = EPSILON ) { return Abs( A - B ) < E; }
inline void Swap( float& A, float& B ) { float T = A; A = B; B = T; }
inline void MinMax( float& A, float& B ) { if ( A > B ) { Swap( A, B ); } }
inline float SqRt( const float V ) { return sqrtf( V ); }
inline float Square( const float V ) { return V * V; }
inline float Pow( const float V, const float E ) { return powf( V, E ); }
inline float SignedSquare( const float V ) { return ( V > 0.0f ) ? V * V : -V * V; }
inline float SignedPow( const float V, const float E ) { return Pow( Abs( V ), E ) * ( ( V > 0.0f ) ? 1.0f : -1.0f ); }
inline float Floor( const float F ) { return floorf( F ); }
inline float Ceiling( const float F ) { return ceilf( F ); }
inline float Mod( const float N, const float D ) { return fmodf( N, D ); }
inline float Split( const float N, float& IntegerPart ) { return modff( N, &IntegerPart ); }	// Splits float into integer and fraction
inline float Frac( const float N ) { return N - Floor( N ); }
inline float Round( const float F ) { return floorf( F + 0.5f ); }
inline float Log( const float F ) { return logf( F ); }
inline float Log10( const float F ) { return log10f( F ); }
inline float LogBase( const float F, const float Base ) { return Log( F ) / Log( Base ); }
inline float Log2( const float F ) { return LogBase( F, 2.0f ); }
inline float Lerp( const float A, const float B, const float T ) { return ( A + ( T * ( B - A ) ) ); }
inline float InvLerp( const float F, const float A, const float B ) { return ( B == A ) ? 0.0f : ( ( F - A ) / ( B - A ) ); }
inline float Hermite( const float A, const float B, const float T ) { return ( A + ( ( T * T * ( 3.0f - 2.0f * T ) ) * ( B - A ) ) ); }
inline float Sign( const float F ) { return ( F > 0.0f ) ? 1.0f : ( ( F < 0.0f ) ? -1.0f : 0.0f ); }
inline float NonZeroSign( const float F ) { return ( F >= 0.0f ) ? 1.0f : -1.0f; }
inline float Saturate( const float F ) { return Clamp( F, 0.0f, 1.0f ); }
inline float QuantizeWithin( const float F, const float E = EPSILON ) { const float R = Round( F ); return ( Abs( R - F ) < E ) ? R : F; }

inline float SRGBToLinear( const float F ) { return Pow( F, 2.2f ); }
inline float LinearToSRGB( const float F ) { return Pow( F, 1.0f / 2.2f ); }

// Attenuation functions:
// Attenuate (linear) is inversely proportional to distance. (Not used much anymore; this *may* be physically correct for sound, but sources are unclear and it's just not good.)
// AttenuateQuad is inversely proportional to distance squared, sort of. (Now used for sound and AI sound sensors; even if it's not physically correct, it's what I expect.)
// AttenuateExp is inversely proportional to 2^distance. (Not used anywhere? Not correct for anything?)
// --->																																		d: 0  r    2r   3r    4r    5r    6r    7r
inline float Attenuate( const float Distance, const float RadiusHalf ) { return RadiusHalf / ( RadiusHalf + Distance ); }					// 1  1/2  1/3  1/4   1/5   1/6   1/7   1/8
inline float AttenuateQuad( const float Distance, const float RadiusHalf ) { return 1.0f / ( 1.0f + Square( Distance / RadiusHalf ) ); }	// 1  1/2  1/5  1/10  1/17  1/26  1/37  1/50
inline float AttenuateExp( const float Distance, const float RadiusHalf ) { return 1.0f / Pow( 2.0f, Distance / RadiusHalf ); }				// 1  1/2  1/4  1/8   1/16  1/32  1/64  1/128

inline void CheckFloatingPointControlWord()
{
#if BUILD_WINDOWS
	uint control_word;
	_controlfp_s( &control_word, 0, 0 );
	//_controlfp_s(&control_word, _CW_DEFAULT, MCW_PC);	// If I ever need it, this resets the FPCW to default
	PRINTF( "FPCW: 0x%08X\n", control_word );
#endif
}

inline float FastInvSqRt( float F )
{
	float Half = 0.5f * F;
	int FAsI = *( int* )&F;
	FAsI = 0x5f3759df - ( FAsI >> 1 );
	F = *( float* )&FAsI;
	F = F * ( 1.5f - Half * F * F );
	//F = F * ( 1.5f - Half * F * F ); // Another iteration of Newton's method can be done to get more precision.
	return F;
}

inline float VolumeToDB( const float V ) { return 20.0f * Log10( V ); }
inline float DBToVolume( const float V ) { return Pow( 10.0f, V / 20.0f ); }
inline float VolumeToMB( const float V ) { return 100.0f * VolumeToDB( V ); }
inline float MBToVolume( const float V ) { return DBToVolume( V / 100.0f ); }

inline float Sin( const float F ) { return sinf( F ); }
inline float ASin( const float F ) { return asinf( F ); }	// F in [-1,1], returns in [-pi/2,pi/2]
inline float Cos( const float F ) { return cosf( F ); }
inline float ACos( const float F ) { return acosf( F ); }	// F in [-1,1], returns in [0,pi]
inline float Tan( const float F ) { return tanf( F ); }
inline float ATan( const float F ) { return atanf( F ); }	// Returns in [-pi/2,pi/2]
inline float ATan2( const float Y, const float X ) { return atan2f( Y, X ); }	// Returns in [-pi,pi]

#if BUILD_DEV
inline bool FIsANumber( const float F ) { return ( F == F ); }
inline bool FIsFinite( const float F ) { return ( F >= -FLT_MAX && F <= FLT_MAX ); }
inline bool FIsValid( const float F ) { return FIsANumber( F ) && FIsFinite( F ); }
#endif

template<class C> C Lerp( const C& A, const C& B, const float t ) { return A + ( t * ( B - A ) ); }

inline uint CountBits( uint Value )
{
	uint Count = 0;
	for( ; Value; Count++)
	{
		Value &= Value - 1;
	}
	return Count;
}

#endif // MATHCORE_H
