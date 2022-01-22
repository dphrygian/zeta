#include "core.h"
#include "vector4.h"
#include "vector.h"
#include "vector2.h"
#include "matrix.h"
#include "mathcore.h"
#include "simplestring.h"

Vector4::Vector4() : x( 0.0f ), y( 0.0f ), z( 0.0f ), w( 0.0f ) {}
Vector4::Vector4( float _x, float _y, float _z, float _w ) : x(_x), y(_y), z(_z), w(_w) {}
Vector4::Vector4( const Vector4& V ) : x(V.x), y(V.y), z(V.z), w(V.w) {}

// Note that w will default to 1 when converting a Vector, because
// this is how Vectors are implicitly treated (see matrix-vector multiply)
Vector4::Vector4( const Vector& V, float _w /*= 1.0f*/ )
:	x( V.x )
,	y( V.y )
,	z( V.z )
,	w( _w )
{
}

Vector4::Vector4( const Vector2& V, float _z /*= 0.0f*/, float _w /*= 0.0f*/ )
:	x( V.x )
,	y( V.y )
,	z( _z )
,	w( _w )
{
}

// Ordered RGBA
Vector4::Vector4( c_uint32 Color )
{
	x = static_cast<float>( R_FROM_COLOR( Color ) ) / 255.0f;
	y = static_cast<float>( G_FROM_COLOR( Color ) ) / 255.0f;
	z = static_cast<float>( B_FROM_COLOR( Color ) ) / 255.0f;
	w = static_cast<float>( A_FROM_COLOR( Color ) ) / 255.0f;
}

Vector4 Vector4::operator*( const Matrix& m ) const
{
	return m * *this;
}

Vector4& Vector4::operator*=( const Matrix& m )
{
	*this = m * *this;
	return *this;
}

Vector4& Vector4::operator=( const Vector4& V )
{
	x = V.x;
	y = V.y;
	z = V.z;
	w = V.w;
	return *this;
}

Vector4 Vector4::operator-() const
{
	return Vector4( -x, -y, -z, -w );
}

Vector4 Vector4::operator+( const Vector4& V ) const
{
	return Vector4( x + V.x, y + V.y, z + V.z, w + V.w );
}

Vector4 Vector4::operator-( const Vector4& V ) const
{
	return Vector4( x - V.x, y - V.y, z - V.z, w - V.w );
}

Vector4 Vector4::operator*( float f ) const
{
	return Vector4( x * f, y * f, z * f, w * f );
}

Vector4 Vector4::operator/( float f ) const
{
	float recF = 1.f / f;
	return Vector4( x * recF, y * recF, z * recF, w * recF );
}

Vector4& Vector4::operator+=( const Vector4& V )
{
	x += V.x;
	y += V.y;
	z += V.z;
	w += V.w;
	return *this;
}

Vector4& Vector4::operator-=( const Vector4& V )
{
	x -= V.x;
	y -= V.y;
	z -= V.z;
	w -= V.w;
	return *this;
}

Vector4& Vector4::operator*=( float f )
{
	x *= f;
	y *= f;
	z *= f;
	w *= f;
	return *this;
}

Vector4& Vector4::operator/=( float f )
{
	float recF = 1.f / f;
	x *= recF;
	y *= recF;
	z *= recF;
	w *= recF;
	return *this;
}

bool Vector4::operator==( const Vector4& V ) const
{
	return
		( x == V.x ) &&
		( y == V.y ) &&
		( z == V.z ) &&
		( w == V.w );
}

bool Vector4::operator!=( const Vector4& V ) const
{
	return
		( x != V.x ) ||
		( y != V.y ) ||
		( z != V.z ) ||
		( w != V.w );
}

Vector4 Vector4::operator*( const Vector4& V ) const
{
	return Vector4( x * V.x, y * V.y, z * V.z, w * V.w );
}

Vector4 Vector4::operator/( const Vector4& V ) const
{
	return Vector4( x / V.x, y / V.y, z / V.z, w / V.w );
}

Vector4& Vector4::operator*=( const Vector4& V )
{
	x *= V.x;
	y *= V.y;
	z *= V.z;
	w *= V.w;
	return *this;
}

Vector4& Vector4::operator/=( const Vector4& V )
{
	x /= V.x;
	y /= V.y;
	z /= V.z;
	w /= V.w;
	return *this;
}

bool Vector4::Equals( const Vector4& V, const float Epsilon ) const
{
	return
		( Abs( x - V.x ) <= Epsilon ) &&
		( Abs( y - V.y ) <= Epsilon ) &&
		( Abs( z - V.z ) <= Epsilon ) &&
		( Abs( w - V.w ) <= Epsilon );
}

//Vector4 Vector4::Cross( const Vector4& V ) const
//{
//	// TODO: This
//	WARN;
//	return Vector4();
//}

float Vector4::Dot(const Vector4& V ) const
{
	return ( x * V.x ) + ( y * V.y ) + ( z * V.z ) + ( w * V.w );
}

void Vector4::Negate()
{
	x = -x;
	y = -y;
	z = -z;
	w = -w;
}

void Vector4::Normalize()
{
	float Len = Length();
	if ( Abs( Len ) < EPSILON )
	{
		return;
	}
	float recLen = 1.f / Len;
	x *= recLen;
	y *= recLen;
	z *= recLen;
	w *= recLen;
}

void Vector4::FastNormalize()
{
	// No error checking, plus fast inverse square root
	float recLen = FastInvSqRt( LengthSquared() );
	x *= recLen;
	y *= recLen;
	z *= recLen;
	w *= recLen;
}

Vector4 Vector4::GetNormalized() const
{
	float Len = Length();
	if ( Abs( Len ) < EPSILON )
	{
		return Vector4();
	}
	float recLen = 1.f / Len;
	return Vector4( x * recLen, y * recLen, z * recLen, w * recLen );
}

Vector4 Vector4::GetFastNormalized() const
{
	// No error checking, plus fast inverse square root
	float recLen = FastInvSqRt( LengthSquared() );
	return Vector4( x * recLen, y * recLen, z * recLen, w * recLen );
}

float Vector4::Length() const
{
	return SqRt( ( x * x ) + ( y * y ) + ( z * z ) + ( w * w ) );
}

float Vector4::LengthSquared() const
{
	return ( x * x ) + ( y * y ) + ( z * z ) + ( w * w );
}

Vector4 Vector4::ProjectionOnto( const Vector4& V ) const
{
	return ( Dot( V ) / V.LengthSquared() ) * V;	// I assume this is still valid for 4D vectors
}

Vector4 Vector4::LERP( float t, const Vector4& V ) const
{
	return *this + ( V - *this ) * t;
}

void Vector4::Zero()
{
	x = 0.f;
	y = 0.f;
	z = 0.f;
	w = 0.f;
}

void Vector4::Set( float _x, float _y, float _z, float _w )
{
	x = _x;
	y = _y;
	z = _z;
	w = _w;
}

// Assumes RGBA order, mainly because Vector is RGB
uint Vector4::ToColor() const
{
	float R = Saturate( x );
	float G = Saturate( y );
	float B = Saturate( z );
	float A = Saturate( w );

	return ARGB_TO_COLOR( (byte)( A * 255.0f ), (byte)( R * 255.0f ), (byte)( G * 255.0f ), (byte)( B * 255.0f ) );
}

Vector4 operator*( float f, const Vector4& V )
{
	return Vector4( V.x * f, V.y * f, V.z * f, V.w * f );
}

Vector4 operator/( float f, const Vector4& V )
{
	float recF = 1.f / f;
	return Vector4( V.x * recF, V.y * recF, V.z * recF, V.w * recF );
}

/*static*/ void Vector4::MinMax( const Vector4& V1, const Vector4& V2, Vector4& OutMin, Vector4& OutMax )
{
	::MinMax( V1.x, V2.x, OutMin.x, OutMax.x );
	::MinMax( V1.y, V2.y, OutMin.y, OutMax.y );
	::MinMax( V1.z, V2.z, OutMin.z, OutMax.z );
	::MinMax( V1.w, V2.w, OutMin.w, OutMax.w );
}

/*static*/ void Vector4::MinMax( Vector4& V1, Vector4& V2 )
{
	::MinMax( V1.x, V2.x );
	::MinMax( V1.y, V2.y );
	::MinMax( V1.z, V2.z );
	::MinMax( V1.w, V2.w );
}

SimpleString Vector4::GetString() const
{
	return SimpleString::PrintF( "(%.02f, %.02f, %.02f, %.02f)", x, y, z, w );
}

#if BUILD_DEV
bool Vector4::IsValid() const
{
	return
		FIsValid( x ) &&
		FIsValid( y ) &&
		FIsValid( z ) &&
		FIsValid( w );
}
#endif
