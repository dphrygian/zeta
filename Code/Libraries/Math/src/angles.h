#ifndef ANGLES_H
#define ANGLES_H

// Very simple Euler angles--there's intentionally
// very little functionality here because Euler
// angles have limited usefulness and matrices
// or quaternions are generally more appropriate.

// In general,
// 0 <= Pitch < 2*PI
// 0 <= Roll < PI
// 0 <= Yaw < 2*PI

class Quat;
class Matrix;
class Vector;

class Angles
{
public:
	Angles();
	Angles( float _Pitch, float _Roll, float _Yaw );

	Angles& operator=( const Angles& a );
	Angles operator-();
	Angles operator+( const Angles& a ) const;
	Angles operator-( const Angles& a ) const;
	Angles operator*( float f ) const;
	Angles operator/( float f ) const;
	Angles& operator+=( const Angles& a );
	Angles& operator-=( const Angles& a );
	Angles& operator*=( float f );
	Angles& operator/=( float f );
	Angles operator*( const Angles& a ) const;	// Component-wise multiplication
	Angles operator/( const Angles& a ) const;	// Component-wise division
	Angles& operator*=( const Angles& a );		// Component-wise multiplication
	Angles& operator/=( const Angles& a );		// Component-wise division
	bool operator==( const Angles& v ) const;
	bool operator!=( const Angles& v ) const;

	void	Zero();
	bool	IsZero() const;

	Vector	GetX() const;	// I.e., right vector
	Vector	GetY() const;	// I.e., forward vector
	Vector	GetZ() const;	// I.e., up vector
	void	GetAxes( Vector& X, Vector& Y, Vector& Z ) const;

	Angles	Get2D() const { return Angles( 0.0f, 0.0f, Yaw ); }
	Angles	GetZeroRoll() const { return Angles( Pitch, 0.0f, Yaw ); }

	Angles	GetShortestRotation() const;

	Vector	ToVector() const;	// Same as GetY
	Vector	ToVector2D() const;	// Only regards Yaw
	Matrix	ToMatrix() const;	// Gets the rotation matrix for this orientation
	Quat	ToQuaternion() const;

	SimpleString	GetString() const;

	// In radians
	float	Pitch;
	float	Roll;
	float	Yaw;
};

Angles operator*( float f, const Angles& a );
Angles operator/( float f, const Angles& a );

#endif // ANGLES_H
