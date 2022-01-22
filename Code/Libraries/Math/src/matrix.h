#ifndef MATRIX_H
#define MATRIX_H

class Quat;
class Vector;
class Vector2;
class Vector4;
class Angles;

class Matrix {
public:
	Matrix();
	Matrix( float m00, float m01, float m02, float m03,
			float m10, float m11, float m12, float m13,
			float m20, float m21, float m22, float m23,
			float m30, float m31, float m32, float m33 );
	Matrix( float p[4][4] );
	Matrix( const Matrix& n );
	Matrix( const Vector4& Row0, const Vector4& Row1, const Vector4& Row2, const Vector4& Row3 );

	Matrix& operator=( const Matrix& n );
	Matrix operator+( const Matrix& n ) const;
	Matrix operator-( const Matrix& n ) const;
	Matrix operator*( const Matrix& n ) const;
	Matrix& operator+=( const Matrix& n );
	Matrix& operator-=( const Matrix& n );
	Matrix& operator*=( const Matrix& n );

	Matrix operator*( float f ) const;
	Matrix operator/( float f ) const;
	Matrix& operator*=( float f );
	Matrix& operator/=( float f );

	Vector operator*( const Vector& V ) const;
	Vector4 operator*( const Vector4& V ) const;
	Vector2 operator*( const Vector2& V ) const;

	// Equivalence tests
	bool operator==( const Matrix& n ) const;
	bool operator!=( const Matrix& n ) const;
	bool Equals( const Matrix& n, const float Epsilon ) const;

	// Functions
	void	Transpose();									// Transpose this matrix
	Matrix	GetTransposed() const;							// Return the transpose of this matrix
	void	Invert();										// Invert this matrix
	Matrix	GetInverse() const;								// Return the inverted matrix
	float	Determinant() const;							// Get determinant
	Matrix	GetCofactor() const;							// Get cofactor matrix
	float	GetCofactorAt(const int i, const int j) const;	// Get cofactor at (i, j)
	void	ZeroTranslationElements();
	Vector	GetTranslationElements() const;
	void	SetTranslationElements( const Vector& V );

	Vector4	GetRow( const uint Row ) const;
	void	SetRow( const uint Row, const Vector4& Value );

	Quat	ToQuaternion() const;
	Angles	ToAngles() const;

	const float*	GetArray() const { return &m[0][0]; }

	// Static functions to create transformation matrices
	// TODO: Maybe make scale and translation take 3 floats instead of a vector,
	// because that's how I tend to use them
	static Matrix CreateCoordinate( const Vector& X, const Vector& Y, const Vector& Z );
	static Matrix CreateScale( const Vector& s );
	static Matrix CreateSkewXY( float x, float y );
	static Matrix CreateSkewXZ( float x, float z );
	static Matrix CreateSkewYZ( float y, float z );
	static Matrix CreateRotationAboutX( float theta );
	static Matrix CreateRotationAboutY( float theta );
	static Matrix CreateRotationAboutZ( float theta );
	static Matrix CreateRotation( const Vector& axis, float theta );
	static Matrix CreateTranslation( const Vector& t );
	static Matrix CreateProjectionOnto( const Vector& n );
	static Matrix CreateReflection( const Vector& n );	// Given normal is the normal to the plane to reflect about

	static Matrix CreateViewMatrixDirection( const Vector& eye, const Vector& dir );
	static Matrix CreateViewMatrixLookAt( const Vector& eye, const Vector& at );
	static Matrix CreateViewMatrixCoords( const Vector& eye, const Vector& rt, const Vector& fd, const Vector& up );

	static Matrix CreateProjectionMatrix( const float fov, const float nearC, const float farC, const float invR, const float aRatio, const float VanishingPointOffset, const bool MirrorX, const bool OpenGL );
	static Matrix CreateProjectionMatrix( const float fov, const float nearC, const float farC, const float aRatio, const bool OpenGL );

	// Top should be a greater value than Bottom!
	static Matrix CreateOrthoProjectionMatrix( const float Left, const float Top, const float Right, const float Bottom, const float NearZ, const float FarZ, const float InvRange, const bool OpenGL );

	// Member data
	union
	{
		float m[4][4];
		float v[16];
	};

	static const Matrix Zero;
	static const Matrix Identity;
};

Matrix operator*( float f, const Matrix& n );
Matrix operator/( float f, const Matrix& n );

#endif
