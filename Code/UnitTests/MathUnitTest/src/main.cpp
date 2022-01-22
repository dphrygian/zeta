#include "core.h"
#include "vector.h"
#include "vector4.h"
#include "matrix.h"
#include "quat.h"
#include "angles.h"
#include "plane.h"
#include "mathcore.h"
#include "test.h"
#include "frustum.h"
#include "aabb.h"
#include "sphere.h"
#include "line.h"
#include "hashedstring.h"
#include "random.h"
#include "filestream.h"
#include "triangle.h"
#include "segment.h"
#include "ray.h"
#include "collisioninfo.h"
#include "cylinder.h"
#include "ellipsoid.h"
#include "crypto.h"
#include "mathfunc.h"
#include "hsv.h"
#include "allocator.h"
#include "convexhull.h"

#include <Windows.h>
#include <crtdbg.h>
#include <math.h>

// Bresenham callback
void PrintPoint( int x, int y, bool& Break, void* pContext )
{
	Unused( Break );
	Unused( pContext );
	PRINTF( "%d %d\n", x, y );
}

// Bresenham3 callback
void PrintPoint3( int x, int y, int z, bool& Break, void* pContext )
{
	Unused( Break );
	Unused( pContext );
	PRINTF( "%d %d %d\n", x, y, z );
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	Unused( hInstance );
	Unused( hPrevInstance );
	Unused( lpCmdLine );
	Unused( nCmdShow );

	Allocator::GetDefault().Initialize( 65536 );
	Math::SeedGenerator();

	STARTTESTS( math );
	SUPPRESSTESTSUCCESSES;

	// Math core tests
	TEST( 5 == Abs( 5 ) );
	TEST( 5 == Abs( -5 ) );
	TEST( 0 == Abs( 0 ) );
	TEST( 0 == Abs( -0 ) );
	TEST( 5 == Max( 2, 5 ) );
	TEST( -2 == Max( -2, -5 ) );
	TEST( 0 == Max( 0, 0 ) );
	TEST( 2 == Min( 2, 5 ) );
	TEST( -5 == Min( -2, -5 ) );
	TEST( 0 == Min( 0, 0 ) );
	TEST( 4 == Clamp( 4, 3, 5 ) );
	TEST( 3 == Clamp( -2, 3, 5 ) );
	TEST( 5 == Clamp( 6, 3, 5 ) );

	PRAGMA( warning( push ) );
	PRAGMA( warning( disable: 4127 ) );
	TEST( 5 == MAbs( 5 ) );
	TEST( 5 == MAbs( -5 ) );
	TEST( 0 == MAbs( 0 ) );
	TEST( 0 == MAbs( -0 ) );
	TEST( 5 == MMax( 2, 5 ) );
	TEST( -2 == MMax( -2, -5 ) );
	TEST( 0 == MMax( 0, 0 ) );
	TEST( 2 == MMin( 2, 5 ) );
	TEST( -5 == MMin( -2, -5 ) );
	TEST( 0 == MMin( 0, 0 ) );
	TEST( 4 == MClamp( 4, 3, 5 ) );
	TEST( 3 == MClamp( -2, 3, 5 ) );
	TEST( 5 == MClamp( 6, 3, 5 ) );
	PRAGMA( warning( pop ) );

	TEST( 5.0f == Abs( 5.0f ) );
	TEST( 5.0f == Abs( -5.0f ) );
	TEST( 0.0f == Abs( 0.0f ) );
	TEST( 0.0f == Abs( -0.0f ) );
	TEST( 5.0f == Max( 2.0f, 5.0f ) );
	TEST( -2.0f == Max( -2.0f, -5.0f ) );
	TEST( 0.0f == Max( 0.0f, 0.0f ) );
	TEST( 2.0f == Min( 2.0f, 5.0f ) );
	TEST( -5.0f == Min( -2.0f, -5.0f ) );
	TEST( 0.0f == Min( 0.0f, 0.0f ) );
	TEST( 4.0f == Clamp( 4.0f, 3.0f, 5.0f ) );
	TEST( 3.0f == Clamp( -2.0f, 3.0f, 5.0f ) );
	TEST( 5.0f == Clamp( 6.0f, 3.0f, 5.0f ) );

	PRAGMA( warning( push ) );
	PRAGMA( warning( disable: 4127 ) );
	TEST( 5.0f == MAbs( 5.0f ) );
	TEST( 5.0f == MAbs( -5.0f ) );
	TEST( 0.0f == MAbs( 0.0f ) );
	TEST( 0.0f == MAbs( -0.0f ) );
	TEST( 5.0f == MMax( 2.0f, 5.0f ) );
	TEST( -2.0f == MMax( -2.0f, -5.0f ) );
	TEST( 0.0f == MMax( 0.0f, 0.0f ) );
	TEST( 2.0f == MMin( 2.0f, 5.0f ) );
	TEST( -5.0f == MMin( -2.0f, -5.0f ) );
	TEST( 0.0f == MMin( 0.0f, 0.0f ) );
	TEST( 4.0f == MClamp( 4.0f, 3.0f, 5.0f ) );
	TEST( 3.0f == MClamp( -2.0f, 3.0f, 5.0f ) );
	TEST( 5.0f == MClamp( 6.0f, 3.0f, 5.0f ) );
	PRAGMA( warning( pop ) );

	FOR_EACH_INDEX_REVERSE( TestIndex, 0 )
	{
		TEST( TestIndex < 5 );
	}

	FOR_EACH_INDEX_REVERSE( TestIndex, 5 )
	{
		TEST( TestIndex < 5 );
	}

	// Vector unit tests
	Vector a, b;
	a = b = Vector(3,3,3);
	a = b *= 3.f;
	TEST( a == Vector(9,9,9) );
	TEST( Vector(5,5,5) * 2 == 2 * Vector(5,5,5) );
	TEST( Vector(5,5,5) * .5f == Vector(5,5,5) / 2 );
	TEST( Vector(5,5,5) + Vector(1,2,3) == Vector(6,7,8) );
	TEST( Vector(1,1,1) != Vector(1.1f,1.1f,1.1f) );
	TEST( Vector(2,0,0).Cross(Vector(0,2,0)) == Vector(0,0,4) );
	TEST( Vector(2,3,4).Dot(Vector(7,6,5)) == 52.f );
	a.Negate();
	TEST( a == Vector(-9,-9,-9) );
	TEST( Vector(5,0,0).GetNormalized() == Vector(1,0,0) );
	a.Normalize();
	TEST( a.Equals( Vector(-1,-1,-1) / 1.732f, EPSILON ) );
	TEST( Abs( Vector(5,5,5).Length() - 8.66f ) < .001f );
	TEST( Abs( Vector(5,5,5).LengthSquared() - 75.f ) < .001f );
	TEST( Vector(2,2,2).ProjectionOnto( Vector(1,0,0) ) == Vector(2,0,0) );
	TEST( Vector(0,0,0).LERP( .3f, Vector(2,2,2) ) == Vector(.6f,.6f,.6f) );

	Vector av( -.5f, .5f, .5f );
	Vector bv;
	Angles t = av.ToAngles();
	bv = t.ToVector();
	av.Normalize();
	TEST( Abs( t.Pitch - .615479f ) < .001f );
	TEST( Abs( t.Roll ) < .001f );
	TEST( Abs( t.Yaw - .785398f ) < .001f );
	TEST( ( bv - av ).LengthSquared() < .001f );
	Vector aX, aY, aZ;
	t.GetAxes( aX, aY, aZ );
	TEST( aX == Vector( 1, 1, 0 ).GetNormalized() );
	TEST( aY == Vector( -1, 1, 1 ).GetNormalized() );
	TEST( aZ.Equals( Vector( 1, -1, 2 ).GetNormalized(), EPSILON ) );

	TEST( ( Vector( 1, 0, 0 ) * Matrix::CreateRotationAboutZ( PI * .5f ) ).Equals( Vector( 0, 1, 0 ), EPSILON ) );

	TEST( Vector( 1.0f, 1.0f, -1.0f ) * Matrix::CreateReflection( Vector( 0.0f, 0.0f, 1.0f ) ) == Vector( 1.0f, 1.0f, 1.0f ) );
	TEST( Vector( 1.0f, 1.0f, -1.0f ) * Matrix::CreateReflection( Vector( -1.0f, 0.0f, 0.0f ) ) == Vector( -1.0f, 1.0f, -1.0f ) );
	TEST( Vector( 1.0f, 1.0f, -1.0f ) * Matrix::CreateReflection( Vector( 0.0f, -1.0f, 0.0f ) ) == Vector( 1.0f, -1.0f, -1.0f ) );

	Matrix am = Matrix::CreateRotationAboutX( .785398f ) * Matrix::CreateRotationAboutZ( .785398f );
	av = Vector( 0, 1, 0 ) * am;
	t = am.ToAngles();
	bv = t.ToVector();
	TEST( Abs( t.Pitch - .785398f ) < .001f );
	TEST( Abs( t.Roll ) < .001f );
	TEST( Abs( t.Yaw - .785398f ) < .001f );
	TEST( ( av - bv ).LengthSquared() < .001f );

	TEST( Vector4( Vector( 1, 1, 1 ) ) == Vector4( 1, 1, 1, 1 ) );

	Matrix c;
	c.GetCofactorAt( 1, 1 );
	c.GetCofactorAt( 2, 3 );

	Matrix translate = Matrix::CreateTranslation( Vector( 1, 1, 5 ) );
	Matrix rotate = Matrix::CreateRotationAboutX( PI / 2.f );
	Matrix scale = Matrix::CreateScale( Vector( 3.f, 3.f, 3.f ) );
	Vector v( -1, -1, -1 );
	v *= scale;
	v *= scale.GetInverse();
	v *= translate;
	v *= rotate;

	Matrix view = Matrix::CreateViewMatrixLookAt( Vector(5,5,5), Vector(0,0,0) );
	Matrix proj = Matrix::CreateProjectionMatrix( 90.f, .1f, 1000.f, 4.f/3.f, false /*OpenGL*/ );
	v *= view;
	v *= proj;

	Quat q(5,5,5,5), r(5,5,5,5);
	q.Invert();
	Quat IdentityQuat;
	TEST( ( r * q ).Equals( IdentityQuat, EPSILON ) );
	TEST( ( IdentityQuat / q ).Equals( r, EPSILON ) );	// Does quat division reverse multiplication? Apparently yes!

	Angles ConversionAngles( 0.5f, 0.5f, 0.5f );
	Quat ConversionQuatViaMatrix = ConversionAngles.ToMatrix().ToQuaternion();
	Quat ConversionQuat = ConversionAngles.ToQuaternion();
	Angles ConversonAnglesFromQuatViaMatrix = ConversionQuat.ToMatrix().ToAngles();
	Angles ConversonAnglesFromQuat = ConversionQuat.ToAngles();
	TEST( ConversionQuat == ConversionQuatViaMatrix );
	TEST( ConversonAnglesFromQuatViaMatrix == ConversionAngles );
	TEST( ConversonAnglesFromQuatViaMatrix == ConversonAnglesFromQuat );

	Plane plane( Vector(0,0,1), -1 );	// -1 means the plane is above the XY-plane... seems backwards, but ok
	TEST( plane.ProjectPoint( Vector(1,1,2) ) == Vector(1,1,1) );
	TEST( plane.ProjectVector( Vector(1,1,1) ) == Vector(1,1,0) );
	TEST( !plane.Intersects( Plane( Vector( 0,0,-1 ), -1 ) ) );
	TEST( plane.Intersects( Plane( Vector( 1,0,0 ), -1 ) ) );
	TEST( plane.DistanceTo( Vector(0,0,2) ) == 1.0f );

	Plane plane2( Vector(1,0,0), -1 );
	Line line = plane.GetIntersection( plane2 );
	TEST( line.m_Direction == Vector( 0, 1, 0 ) );
	Line line1(Vector(0,0,0), Vector(-1,0,0));
	Line line2(Vector(0,0,0), Vector(1,0,0));
	Line line3(Vector(2,0,0), Vector(1,0,0));
	Line line4(Vector(2,0,0), Vector(-1,0,0));
	TEST( plane2.Intersects( line2 ) );
	TEST( plane2.GetIntersection( line1 ) == Vector( 1, 0, 0 ) );
	TEST( plane2.GetIntersection( line2 ) == Vector( 1, 0, 0 ) );
	TEST( plane2.GetIntersection( line3 ) == Vector( 1, 0, 0 ) );
	TEST( plane2.GetIntersection( line4 ) == Vector( 1, 0, 0 ) );

	Line anotherline( Vector(1,1,1), Vector(1,1,1) );
	Vector v_yes(2,2,2);
	Vector v_no(2,2,3);
	Sphere s_yes( Vector(3,3,2), 2 );
	Sphere s_no( Vector(0,0,3), 1 );
	TEST( anotherline.Intersects( v_yes ) );
	TEST( !anotherline.Intersects( v_no ) );
	TEST( anotherline.Intersects( s_yes ) );
	TEST( !anotherline.Intersects( s_no ) );

	Frustum frustum;	// Unit frustum
	Frustum viewfrustum( Matrix::CreateViewMatrixDirection( Vector(0,-1,0), Vector(0,1,0) ) * Matrix::CreateProjectionMatrix( 90.f, .01f, 3000.f, 4.f/3.f, false /*OpenGL*/ ) );
	Frustum viewfrustum2( Matrix::CreateViewMatrixDirection( Vector(0,5,0), Vector(0,1,0) ) * Matrix::CreateProjectionMatrix( 90.f, 1, 2, 4.f/3.f, false /*OpenGL*/ ) );
	TEST( frustum.Intersects( Vector(0,0,0) ) );
	TEST( frustum.Intersects( AABB( Vector(-.5f,-.5f,-.5f), Vector(.5f,.5f,.5f) ) ) );
	TEST( frustum.Intersects( viewfrustum ) );
	TEST( viewfrustum.Intersects( frustum ) );
	TEST( !frustum.Intersects( viewfrustum2 ) );
	TEST( !viewfrustum2.Intersects( frustum ) );

	AABB aabb_a( Vector(0,0,0), Vector(2,2,2) );
	AABB aabb_b( Vector(1,1,1), Vector(3,3,3) );
	AABB aabb_c( Vector(3,3,3), Vector(4,4,4) );
	TEST( aabb_a.Intersects( aabb_b ) );
	TEST( !aabb_a.Intersects( aabb_c ) );
	TEST( aabb_b.Intersects( aabb_c ) );

	Sphere sp_s;
	Sphere sp_t( Vector(3,3,3), 1 );
	Sphere sp_u( Vector(5,5,5), 3 );
	Sphere sp_v( Vector(0,5,0), 1 );
	TEST( sp_s.Intersects( frustum ) );
	TEST( !sp_t.Intersects( frustum ) );
	TEST( !sp_v.Intersects( frustum ) );
	TEST( sp_t.Intersects( sp_u ) );
	TEST( !sp_t.Intersects( sp_s ) );

	Random RNG;
	RNG.Seed();
	TEST( RNG.Get( 100 ) < 100 );
	TEST( RNG.Get( 0xffffffff ) < 0xffffffff );
	int Test[10];
	memset( Test, 0, sizeof( Test ) );
	for( int i = 0; i < 1000; ++i )
	{
		++Test[ RNG.Get( 10 ) ];	// Test how well random numbers are distributed
	}

	Triangle Triangle1( Vector( 1.0f, 1.0f, 1.0f ), Vector( 2.0f, 2.0f, 1.0f ), Vector( 1.5f, 1.5f, 3.0f ) );
	Line Line1( Vector( 2.0f, 1.0f, 2.0f ), Vector( -1.0f, 1.0f, 0.0f ) );
	Line Line2( Vector( 1.0f, 2.0f, 2.0f ), Vector( 1.0f, -1.0f, 0.0f ) );
	Segment Segment1( Vector( 2.0f, 1.0f, 2.0f ), Vector( 1.0f, 2.0f, 2.0f ) );
	Segment Segment2( Vector( 1.0f, 2.0f, 2.0f ), Vector( 2.0f, 1.0f, 2.0f ) );

	TEST( Line1.Intersects( Triangle1, false ) );
	TEST( Line1.Intersects( Triangle1, true ) );
	TEST( !Line2.Intersects( Triangle1, false ) );
	TEST( Line2.Intersects( Triangle1, true ) );
	TEST( Segment1.Intersects( Triangle1 ) );
	TEST( !Segment2.Intersects( Triangle1 ) );

	Segment PlaneSegmentTestSegment1( Vector( -3.0f, 0.0f, 0.0f ), Vector( -5.0f, 0.0f, 0.0f ) );
	Segment PlaneSegmentTestSegment1a( Vector( -5.0f, 0.0f, 0.0f ), Vector( -3.0f, 0.0f, 0.0f ) );
	Segment PlaneSegmentTestSegment2( Vector( -3.0f, 0.0f, 0.0f ), Vector( -1.0f, 0.0f, 0.0f ) );
	Segment PlaneSegmentTestSegment3( Vector( -1.0f, 0.0f, 0.0f ), Vector( -3.0f, 0.0f, 0.0f ) );
	Segment PlaneSegmentTestSegment4( Vector( -1.0f, 0.0f, 0.0f ), Vector( 1.0f, 0.0f, 0.0f ) );
	Segment PlaneSegmentTestSegment5( Vector( 3.0f, 0.0f, 0.0f ), Vector( 1.0f, 0.0f, 0.0f ) );
	Segment PlaneSegmentTestSegment5a( Vector( 1.0f, 0.0f, 0.0f ), Vector( 3.0f, 0.0f, 0.0f ) );
	Segment PlaneSegmentTestSegment6( Vector( 3.0f, 0.0f, 0.0f ), Vector( 5.0f, 0.0f, 0.0f ) );
	Segment PlaneSegmentTestSegment7( Vector( 1.0f, 0.0f, 0.0f ), Vector( -1.0f, 0.0f, 0.0f ) );
	Segment PlaneSegmentTestSegment8( Vector( 1.0f, 0.0f, 0.0f ), Vector( 3.0f, 0.0f, 0.0f ) );
	Plane PlaneSegmentTestPlane1( Vector( 1.0f, 0.0f, 0.0f ), -2.0f );	// Right side of origin, facing right
	Plane PlaneSegmentTestPlane2( Vector( 1.0f, 0.0f, 0.0f ), 2.0f );	// Left side of origin, facing right
	Plane PlaneSegmentTestPlane3( Vector( -1.0f, 0.0f, 0.0f ), 2.0f );	// Right side of origin, facing left
	Plane PlaneSegmentTestPlane4( Vector( -1.0f, 0.0f, 0.0f ), -2.0f );	// Left side of origin, facing left

	TEST( !PlaneSegmentTestSegment1.Intersects( PlaneSegmentTestPlane1 ) );
	TEST( !PlaneSegmentTestSegment1a.Intersects( PlaneSegmentTestPlane1 ) );
	TEST( !PlaneSegmentTestSegment2.Intersects( PlaneSegmentTestPlane1 ) );
	TEST( !PlaneSegmentTestSegment3.Intersects( PlaneSegmentTestPlane1 ) );
	TEST( !PlaneSegmentTestSegment4.Intersects( PlaneSegmentTestPlane1 ) );
	TEST( PlaneSegmentTestSegment5.Intersects( PlaneSegmentTestPlane1 ) );
	TEST( PlaneSegmentTestSegment5a.Intersects( PlaneSegmentTestPlane1 ) );
	TEST( !PlaneSegmentTestSegment6.Intersects( PlaneSegmentTestPlane1 ) );
	TEST( !PlaneSegmentTestSegment7.Intersects( PlaneSegmentTestPlane1 ) );
	TEST( PlaneSegmentTestSegment8.Intersects( PlaneSegmentTestPlane1 ) );

	TEST( !PlaneSegmentTestSegment1.Intersects( PlaneSegmentTestPlane2 ) );
	TEST( PlaneSegmentTestSegment2.Intersects( PlaneSegmentTestPlane2 ) );
	TEST( PlaneSegmentTestSegment3.Intersects( PlaneSegmentTestPlane2 ) );
	TEST( !PlaneSegmentTestSegment4.Intersects( PlaneSegmentTestPlane2 ) );
	TEST( !PlaneSegmentTestSegment5.Intersects( PlaneSegmentTestPlane2 ) );
	TEST( !PlaneSegmentTestSegment6.Intersects( PlaneSegmentTestPlane2 ) );
	TEST( !PlaneSegmentTestSegment7.Intersects( PlaneSegmentTestPlane2 ) );
	TEST( !PlaneSegmentTestSegment8.Intersects( PlaneSegmentTestPlane2 ) );

	TEST( !PlaneSegmentTestSegment1.Intersects( PlaneSegmentTestPlane3 ) );
	TEST( !PlaneSegmentTestSegment2.Intersects( PlaneSegmentTestPlane3 ) );
	TEST( !PlaneSegmentTestSegment3.Intersects( PlaneSegmentTestPlane3 ) );
	TEST( !PlaneSegmentTestSegment4.Intersects( PlaneSegmentTestPlane3 ) );
	TEST( PlaneSegmentTestSegment5.Intersects( PlaneSegmentTestPlane3 ) );
	TEST( !PlaneSegmentTestSegment6.Intersects( PlaneSegmentTestPlane3 ) );
	TEST( !PlaneSegmentTestSegment7.Intersects( PlaneSegmentTestPlane3 ) );
	TEST( PlaneSegmentTestSegment8.Intersects( PlaneSegmentTestPlane3 ) );

	TEST( !PlaneSegmentTestSegment1.Intersects( PlaneSegmentTestPlane4 ) );
	TEST( PlaneSegmentTestSegment2.Intersects( PlaneSegmentTestPlane4 ) );
	TEST( PlaneSegmentTestSegment3.Intersects( PlaneSegmentTestPlane4 ) );
	TEST( !PlaneSegmentTestSegment4.Intersects( PlaneSegmentTestPlane4 ) );
	TEST( !PlaneSegmentTestSegment5.Intersects( PlaneSegmentTestPlane4 ) );
	TEST( !PlaneSegmentTestSegment6.Intersects( PlaneSegmentTestPlane4 ) );
	TEST( !PlaneSegmentTestSegment7.Intersects( PlaneSegmentTestPlane4 ) );
	TEST( !PlaneSegmentTestSegment8.Intersects( PlaneSegmentTestPlane4 ) );

	AABB AABB1( Vector( 2.0f, 2.0f, 2.0f ), Vector( 5.0f, 5.0f, 5.0f ) );
	Segment Segment3( Vector( 4.0f, 4.0f, 1.0f ), Vector( 1.0f, 1.0f, 4.0f ) );
	Segment Segment4( Vector( 5.0f, 5.0f, 1.0f ), Vector( 7.0f, 7.0f, 3.0f ) );
	Ray Ray1( Vector( 4.0f, 4.0f, 4.0f ), Vector( -1.0f, -1.0f, 1.0f ) );
	Ray Ray2( Vector( 5.0f, 5.0f, 1.0f ), Vector( 1.0f, 1.0f, -1.0f ) );
	Plane Plane1( Vector( 0.0f, 0.0f, 1.0f ), -1.0f );
	Plane Plane2( Vector( 0.0f, 0.0f, 1.0f ), -3.0f );
	TEST( Segment3.Intersects( AABB1 ) );
	TEST( !Segment4.Intersects( AABB1 ) );
	TEST( Ray1.Intersects( AABB1 ) );
	TEST( !Ray2.Intersects( AABB1 ) );
	TEST( !Plane1.Intersects( AABB1 ) );
	TEST( Plane2.Intersects( AABB1 ) );

	Ray Ray3( Vector( -3.0f, 0.0f, 0.0f ), Vector( 1.0f, 0.0f, 0.0f ) );
	Ray Ray4( Vector( -3.0f, 0.0f, 0.0f ), Vector( -1.0f, 0.0f, 0.0f ) );
	Ray Ray5( Vector( -1.0f, 0.0f, 0.0f ), Vector( 1.0f, 0.0f, 0.0f ) );
	Ray Ray6( Vector( -1.0f, 0.0f, 0.0f ), Vector( -1.0f, 0.0f, 0.0f ) );
	Ray Ray7( Vector( 3.0f, 0.0f, 0.0f ), Vector( 1.0f, 0.0f, 0.0f ) );
	Ray Ray8( Vector( 3.0f, 0.0f, 0.0f ), Vector( -1.0f, 0.0f, 0.0f ) );
	Ray Ray9( Vector( 1.0f, 0.0f, 0.0f ), Vector( 1.0f, 0.0f, 0.0f ) );
	Ray Ray10( Vector( 1.0f, 0.0f, 0.0f ), Vector( -1.0f, 0.0f, 0.0f ) );
	Plane Plane3( Vector( 1.0f, 0.0f, 0.0f ), -2.0f );	// Right side of origin, facing right
	Plane Plane4( Vector( 1.0f, 0.0f, 0.0f ), 2.0f );	// Left side of origin, facing right
	Plane Plane5( Vector( -1.0f, 0.0f, 0.0f ), 2.0f );	// Right side of origin, facing left
	Plane Plane6( Vector( -1.0f, 0.0f, 0.0f ), -2.0f );	// Left side of origin, facing left

	TEST( Plane3.Intersects( Ray3 ) );
	TEST( !Plane3.Intersects( Ray4 ) );
	TEST( Plane3.Intersects( Ray5 ) );
	TEST( !Plane3.Intersects( Ray6 ) );
	TEST( !Plane3.Intersects( Ray7 ) );
	TEST( Plane3.Intersects( Ray8 ) );
	TEST( Plane3.Intersects( Ray9 ) );
	TEST( !Plane3.Intersects( Ray10 ) );
	TEST( Plane4.Intersects( Ray3 ) );
	TEST( !Plane4.Intersects( Ray4 ) );
	TEST( !Plane4.Intersects( Ray5 ) );
	TEST( Plane4.Intersects( Ray6 ) );
	TEST( !Plane4.Intersects( Ray7 ) );
	TEST( Plane4.Intersects( Ray8 ) );
	TEST( !Plane4.Intersects( Ray9 ) );
	TEST( Plane4.Intersects( Ray10 ) );
	TEST( Plane5.Intersects( Ray3 ) );
	TEST( !Plane5.Intersects( Ray4 ) );
	TEST( Plane5.Intersects( Ray5 ) );
	TEST( !Plane5.Intersects( Ray6 ) );
	TEST( !Plane5.Intersects( Ray7 ) );
	TEST( Plane5.Intersects( Ray8 ) );
	TEST( Plane5.Intersects( Ray9 ) );
	TEST( !Plane5.Intersects( Ray10 ) );
	TEST( Plane6.Intersects( Ray3 ) );
	TEST( !Plane6.Intersects( Ray4 ) );
	TEST( !Plane6.Intersects( Ray5 ) );
	TEST( Plane6.Intersects( Ray6 ) );
	TEST( !Plane6.Intersects( Ray7 ) );
	TEST( Plane6.Intersects( Ray8 ) );
	TEST( !Plane6.Intersects( Ray9 ) );
	TEST( Plane6.Intersects( Ray10 ) );

	CollisionInfo Info1, Info2, Info3, Info4, Info5, Info6, Info7, Info8;
	Segment Segment5( Vector( 2.0f, 0.0f, 1.0f ), Vector( 2.0f, 2.0f, 1.0f ) );
	Cylinder Cylinder1( Vector( 2.0f, 1.0f, 1.0f ), Vector( 1.0f, 2.0f, 1.0f ), 0.5f );
	Segment5.Intersects( Cylinder1, &Info3 );
	TEST( !Info3.m_Out_Collision );	// Shouldn't be colliding with endcaps

	Triangle Tri1( Vector( 2.0f, 1.0f, 1.0f ), Vector( 1.5f, 1.5f, 3.0f ), Vector( 1.0f, 2.0f, 1.0f ) );
	Triangle Tri2( Vector( -1.0f, 2.0f, 0.0f ), Vector( -1.0f, 2.0f, 2.0f ), Vector( -3.0f, 2.0f, 0.0f ) );
	Triangle Tri3( Vector( 1.0f, 0.0f, 1.0f ), Vector( 1.0f, 0.0f, 0.0f ), Vector( -1.0f, 0.0f, 0.0f ) );
	Triangle Tri4( Vector( 0.0f, 0.0f, -1.0f ), Vector( 0.0f, 0.0f, 1.0f ), Vector( -1.0f, 0.0f, 0.0f ) );
	Sphere Sphere1( Vector( 1.0f, 1.0f, 1.0f ), 0.5f );
	Sphere Sphere2( Vector( 2.0f, 0.0f, 1.0f ), 0.5f );
	Sphere Sphere3( Vector( -2.0f, 0.5f, 0.0f ), 0.8f );
	Sphere Sphere4( Vector( 1.0f, 0.0f, 0.0f ), 0.5f );
	Sphere Sphere5( Vector( 1.0f, 0.0f, 1.0f ), 0.5f );
	Sphere Sphere6( Vector( -0.5f, 1.0f, 0.0f ), 0.5f );
	Sphere Sphere7( Vector( 0.01f, -0.5001f, 0.0f ), 0.5f );
	Vector Point1( -2.0f, 0.0f, 0.0f );
	Vector Velocity1( 1.0f, 1.0f, 1.0f );
	Vector Velocity2( 0.0f, 2.0f, 0.0f );
	Vector Velocity3( 1.0f, 1.0f, 0.0f );
	Vector Velocity4( -2.0f, 0.0f, 0.0f );
	Vector Velocity5( 0.0f, -2.0f, 0.0f );
	Vector Velocity6( 0.0f, 0.0004f, 0.0f );
	Sphere1.SweepAgainst( Tri1, Velocity1, &Info1 );	// Collides with face
	Sphere2.SweepAgainst( Tri1, Velocity2, &Info2 );	// Collides with vert
	Sphere3.SweepAgainst( Tri2, Velocity3, &Info4 );	// Collides with face
	Sphere4.SweepAgainst( Tri4, Velocity4, &Info5 );	// Collides with edge
	Sphere5.SweepAgainst( Tri4, Velocity4, &Info6 );	// Collides with edge
	Sphere6.SweepAgainst( Tri4, Velocity5, &Info7 );	// Collides with edge
	Sphere7.SweepAgainst( Tri4, Velocity6, &Info8 );	// Collides with edge
	TEST( Info1.m_Out_Collision );
	TEST( Info1.m_Out_Plane.m_Normal == Vector( -1.0f, -1.0f, 0.0f ).GetNormalized() );
	TEST( Info2.m_Out_Collision );
	TEST( Info2.m_Out_Intersection == Vector( 2.0f, 1.0f, 1.0f ) );
	TEST( Info2.m_Out_Plane.m_Normal == Vector( 0.0f, -1.0f, 0.0f ) );
	TEST( Info4.m_Out_Collision );
	TEST( Info4.m_Out_Intersection == Vector( -1.3f, 2.0f, 0.0f ) );
	TEST( Info4.m_Out_Plane.m_Normal == Vector( 0.0f, -1.0f, 0.0f ) );
	TEST( !Tri3.Contains( Point1 ) );
	TEST( Info5.m_Out_Intersection == Vector( 0.0f, 0.0f, 0.0f ) );
	TEST( Info5.m_Out_HitT == 0.25f );
	TEST( Info6.m_Out_Intersection == Vector( 0.0f, 0.0f, 1.0f ) );
	TEST( Info6.m_Out_HitT == 0.25f );
	TEST( Info7.m_Out_Intersection == Vector( -0.5f, 0.0f, 0.0f ) );
	TEST( Info7.m_Out_HitT == 0.25f );
	TEST( Info8.m_Out_Intersection == Vector( 0.0f, 0.0f, 0.0f ) );
	TEST( Triangle( Vector( 5, 0, 0 ), Vector( -5, 0, 0 ), Vector( 0, 5, 0 ) ).Contains( Vector( 0, 2, 0 ) ) );
	TEST( !Triangle( Vector( 5, 0, 0 ), Vector( -5, 0, 0 ), Vector( 0, 5, 0 ) ).Contains( Vector( 0, 2, -1 ) ) );

	Plane NormalPointPlane( Vector( 0.0f, 0.0f, 1.0f ), Vector( 2.0f, 2.0f, 2.0f ) );
	TEST( NormalPointPlane.m_Distance == -2.0f );

	Ellipsoid e1( Vector( 0.0f, 0.0f, 0.0f ), Vector( 0.5f, 0.5f, 2.0f ) );
	Ellipsoid e2( Vector( 0.0f, -2.5f, 1.0f ), Vector( 2.0f, 2.0f, 0.5f ) );
	Vector ev( 0.0f, 1.0f, 0.0f );
	CollisionInfo ei;
	e2.SweepAgainst( e1, ev, &ei );

	Vector X( 1.0f, 0.0f, 0.0f );
	Vector Y( 0.0f, 1.0f, 0.0f );
	Vector Z( 0.0f, 0.0f, 1.0f );
	Vector XY( 0.707107f, 0.707107f, 0.0f );
	Vector XZ( 0.707107f, 0.0f, 0.707107f );
	Vector nXZ( -0.707107f, 0.0f, 0.707107f );
	Vector YZ( 0.0f, 0.707107f, 0.707107f );
	Vector XYZ( 0.577350f, 0.577350f, 0.577350f );
	TEST( ( X * Matrix::CreateRotationAboutZ( PI * 0.5f ) ).Equals( Y, EPSILON ) );
	TEST( ( X * Matrix::CreateRotation( Z, PI * 0.5f ) ).Equals( Y, EPSILON ) );
	TEST( ( Y * Matrix::CreateRotationAboutX( PI * 0.5f ) ).Equals( Z, EPSILON ) );
	TEST( ( Y * Matrix::CreateRotation( X, PI * 0.5f ) ).Equals( Z, EPSILON ) );
	TEST( ( Z * Matrix::CreateRotationAboutY( PI * 0.5f ) ).Equals( X, EPSILON ) );
	TEST( ( Z * Matrix::CreateRotation( Y, PI * 0.5f ) ).Equals( X, EPSILON ) );
	TEST( ( Z * Matrix::CreateRotation( Y, PI * 0.5f ) ).Equals( X, EPSILON ) );
	TEST( ( X * Matrix::CreateRotation( XY, PI ) ).Equals( Y, EPSILON ) );
	TEST( ( Y * Matrix::CreateRotation( XY, PI ) ).Equals( X, EPSILON ) );
	TEST( ( X * Matrix::CreateRotation( XZ, PI ) ).Equals( Z, EPSILON ) );
	TEST( ( Z * Matrix::CreateRotation( XZ, PI ) ).Equals( X, EPSILON ) );
	TEST( ( Y * Matrix::CreateRotation( YZ, PI ) ).Equals( Z, EPSILON ) );
	TEST( ( Z * Matrix::CreateRotation( YZ, PI ) ).Equals( Y, EPSILON ) );

	TEST( Equal( 0.0f, XY.GetPerpendicular().Dot( XY ) ) );
	TEST( Equal( 0.0f, XZ.GetPerpendicular().Dot( XZ ) ) );
	TEST( Equal( 0.0f, YZ.GetPerpendicular().Dot( YZ ) ) );
	TEST( Equal( 0.0f, XYZ.GetPerpendicular().Dot( XYZ ) ) );

	TEST( X * Matrix::CreateCoordinate( Y, -X, Z ) == Y );
	TEST( XY * Matrix::CreateCoordinate( Z, -X, -Y ) == nXZ );
	TEST( Matrix::CreateCoordinate( Y, -X, Z ).ToAngles() == Angles( 0.0f, 0.0f, PI * 0.5f ) );
	TEST( Matrix::CreateCoordinate( -Y, X, Z ).ToAngles() == Angles( 0.0f, 0.0f, -PI * 0.5f ) );

	TEST( Mod( 3, 8 ) == 3 );
	TEST( Mod( 13, 8 ) == 5 );
	TEST( Mod( -1, 8 ) == 7 );
	TEST( Mod( -13, 8 ) == 3 );

	TEST( Equal( FastInvSqRt( 1.0f ), 1.0f, 0.01f ) );
	TEST( Equal( FastInvSqRt( 0.25f ), 2.0f, 0.01f ) );
	TEST( Equal( FastInvSqRt( 4.0f ), 0.5f, 0.01f ) );
	TEST( Equal( FastInvSqRt( 25.0f ), 0.2f, 0.01f ) );

	static const float kColorEpsilon = ( 0.5f / 255.0f );
	Vector ColorHSV = HSV::RGBToHSV( Vector( 1.0f, 0.0f, 0.0f ) );
	TEST( Equal( ColorHSV.x, 0.0f, kColorEpsilon ) );
	TEST( Equal( ColorHSV.y, 1.0f, kColorEpsilon ) );
	TEST( Equal( ColorHSV.z, 1.0f, kColorEpsilon ) );

	ColorHSV = HSV::RGBToHSV( Vector( 1.0f, 0.5f, 0.25f ) );
	TEST( Equal( ColorHSV.x, 20.0f / 360.0f, kColorEpsilon ) );
	TEST( Equal( ColorHSV.y, 0.75f, kColorEpsilon ) );
	TEST( Equal( ColorHSV.z, 1.0f, kColorEpsilon ) );

	ColorHSV = HSV::RGBToHSV( Vector( 1.0f, 0.25f, 0.5f ) );
	TEST( Equal( ColorHSV.x, 340.0f / 360.0f, kColorEpsilon ) );
	TEST( Equal( ColorHSV.y, 0.75f, kColorEpsilon ) );
	TEST( Equal( ColorHSV.z, 1.0f, kColorEpsilon ) );

	Vector ColorRGB = HSV::HSVToRGB( Vector( 0.0f, 1.0f, 1.0f ) );
	TEST( Equal( ColorRGB.r, 1.0f, kColorEpsilon ) );
	TEST( Equal( ColorRGB.g, 0.0f, kColorEpsilon ) );
	TEST( Equal( ColorRGB.b, 0.0f, kColorEpsilon ) );

	ColorRGB = HSV::HSVToRGB( Vector( 20.0f / 360.0f, 0.75f, 1.0f ) );
	TEST( Equal( ColorRGB.r, 1.0f, kColorEpsilon ) );
	TEST( Equal( ColorRGB.g, 0.5f, kColorEpsilon ) );
	TEST( Equal( ColorRGB.b, 0.25f, kColorEpsilon ) );

	ColorRGB = HSV::HSVToRGB( Vector( 340.0f / 360.0f, 0.75f, 1.0f ) );
	TEST( Equal( ColorRGB.r, 1.0f, kColorEpsilon ) );
	TEST( Equal( ColorRGB.g, 0.25f, kColorEpsilon ) );
	TEST( Equal( ColorRGB.b, 0.5f, kColorEpsilon ) );

	ColorRGB = HSV::HSVToRGB( Vector( 0.0f, 0.0f, 0.5f ) );
	TEST( Equal( ColorRGB.r, 0.5f, kColorEpsilon ) );
	TEST( Equal( ColorRGB.g, 0.5f, kColorEpsilon ) );
	TEST( Equal( ColorRGB.b, 0.5f, kColorEpsilon ) );

	// Crypto tests
	{
		SimpleString Plaintext( "ABCDEFGHIJKLMNOPQRSTUVWXYZ" );
		SimpleString Key( "FOO" );
		Array< char > PlaintextArray;
		Array< char > KeyArray;
		Array< char > CiphertextArray;
		Array< char > OutPlaintextArray;
		Plaintext.FillArray( PlaintextArray );
		Key.FillArray( KeyArray );
		Crypto::Encrypt( PlaintextArray, KeyArray, CiphertextArray );
		Crypto::Decrypt( CiphertextArray, KeyArray, OutPlaintextArray );
		OutPlaintextArray.PushBack( '\0' );
		SimpleString OutPlaintext( OutPlaintextArray );
		TEST( Plaintext == OutPlaintext );
	}

	// Bresenham tests
	PRINTF( "Bresenham 2D 1\n" );
	Math::Bresenham( 0, 0, 4, 2, PrintPoint, NULL );
	PRINTF( "Bresenham 2D 2\n" );
	Math::Bresenham( 0, 0, 2, 4, PrintPoint, NULL );
	PRINTF( "Bresenham 2D 3\n" );
	Math::Bresenham( 0, 0, -2, -4, PrintPoint, NULL );
	PRINTF( "Bresenham 2D 4\n" );
	Math::Bresenham( 0, 0, -4, -2, PrintPoint, NULL );
	PRINTF( "Bresenham 3D 1\n" );
	Math::Bresenham3( 0, 0, 0, 4, 2, 0, PrintPoint3, NULL );
	PRINTF( "Bresenham 3D 2\n" );
	Math::Bresenham3( 0, 0, 0, 2, 4, 8, PrintPoint3, NULL );
	PRINTF( "Bresenham 3D 3\n" );
	Math::Bresenham3( 0, 0, 0, 4, 2, 8, PrintPoint3, NULL );
	PRINTF( "Bresenham 3D 4\n" );
	Math::Bresenham3( 0, 0, 0, 2, 8, 4, PrintPoint3, NULL );
	PRINTF( "Bresenham 3D 5\n" );
	Math::Bresenham3( 0, 0, 0, 4, 8, 2, PrintPoint3, NULL );
	PRINTF( "Bresenham 3D 6\n" );
	Math::Bresenham3( 0, 0, 0, 8, 2, 4, PrintPoint3, NULL );
	PRINTF( "Bresenham 3D 7\n" );
	Math::Bresenham3( 0, 0, 0, 8, 4, 2, PrintPoint3, NULL );
	PRINTF( "Bresenham 3D 8\n" );
	Math::Bresenham3( 0, 0, 0, 2, -4, 8, PrintPoint3, NULL );
	PRINTF( "Bresenham 3D 9\n" );
	Math::Bresenham3( 0, 0, 0, -4, 2, 8, PrintPoint3, NULL );
	PRINTF( "Bresenham 3D 10\n" );
	Math::Bresenham3( 0, 0, 0, 2, 8, -4, PrintPoint3, NULL );
	PRINTF( "Bresenham 3D 11\n" );
	Math::Bresenham3( 0, 0, 0, -4, 8, 2, PrintPoint3, NULL );
	PRINTF( "Bresenham 3D 12\n" );
	Math::Bresenham3( 0, 0, 0, 8, 2, -4, PrintPoint3, NULL );
	PRINTF( "Bresenham 3D 13\n" );
	Math::Bresenham3( 0, 0, 0, 8, -4, 2, PrintPoint3, NULL );

	// Convex volume tests
	{
		{
			ConvexHull CubeVolume;
			CubeVolume.TryAddPlane( Plane( Vector( 0.0f, 0.0f, 1.0f ), -1.0f ), EPSILON );
			CubeVolume.TryAddPlane( Plane( Vector( 0.0f, 0.0f, 1.0f ), -1.0f ), EPSILON );	// Make sure this doesn't get added twice
			CubeVolume.TryAddPlane( Plane( Vector( 0.0f, 0.0f, -1.0f ), -1.0f ), EPSILON );
			CubeVolume.TryAddPlane( Plane( Vector( 1.0f, 0.0f, 0.0f ), -1.0f ), EPSILON );
			CubeVolume.TryAddPlane( Plane( Vector( -1.0f, 0.0f, 0.0f ), -1.0f ), EPSILON );
			CubeVolume.TryAddPlane( Plane( Vector( 0.0f, 1.0f, 0.0f ), -1.0f ), EPSILON );
			CubeVolume.TryAddPlane( Plane( Vector( 0.0f, -1.0f, 0.0f ), -1.0f ), EPSILON );
			TEST( CubeVolume.GetNumPlanes() == 6 );

			{
				const AABB TestBox = AABB::CreateFromCenterAndExtents( Vector( 1.5f, 1.5f, 2.5f ), Vector( 1.0f, 1.0f, 1.0f ) );
				const Vector Travel = Vector( 0.0f, 0.0f, -1.0f );

				CollisionInfo Info;
				const bool Collision = CubeVolume.Sweep( TestBox, Travel, &Info );

				TEST( Collision == true );
				TEST( Abs( Info.m_Out_HitT - 0.5f ) < EPSILON );
			}

			{
				const AABB TestBox = AABB::CreateFromCenterAndExtents( Vector( 2.5f, 1.5f, 2.5f ), Vector( 1.0f, 1.0f, 1.0f ) );
				const Vector Travel = Vector( 0.0f, 0.0f, -1.0f );

				CollisionInfo Info;
				const bool Collision = CubeVolume.Sweep( TestBox, Travel, &Info );

				TEST( Collision == false );
			}

			{
				const AABB TestBox = AABB::CreateFromCenterAndExtents( Vector( 1.5f, 1.5f, 1.0f ), Vector( 1.0f, 1.0f, 1.0f ) );
				const Vector Travel = Vector( 0.0f, 0.0f, -1.0f );

				CollisionInfo Info;
				const bool Collision = CubeVolume.Sweep( TestBox, Travel, &Info );

				TEST( Collision == true );
				TEST( Info.m_Out_HitT == 0.0f );	// Started inside collision
			}

			{
				const AABB TestPoint = AABB::CreateFromCenterAndExtents( Vector( 0.5f, 0.5f, 0.5f ), Vector() );

				CollisionInfo Info;
				const bool Collision = CubeVolume.Sweep( TestPoint, Vector(), &Info );

				TEST( Collision == true );
				TEST( Info.m_Out_HitT == 0.0f );	// Point inside collision
			}

			{
				const AABB TestPoint = AABB::CreateFromCenterAndExtents( Vector( 1.5f, 1.5f, 1.5f ), Vector() );

				CollisionInfo Info;
				const bool Collision = CubeVolume.Sweep( TestPoint, Vector(), &Info );

				TEST( Collision == false );		// Point outside collision
			}

			{
				const AABB TestPoint = AABB::CreateFromCenterAndExtents( Vector( 1.0f, 1.0f, 1.0f ), Vector() );

				CollisionInfo Info;
				const bool Collision = CubeVolume.Sweep( TestPoint, Vector(), &Info );

				TEST( Collision == true );
				TEST( Info.m_Out_HitT == 0.0f );	// Point on collision
			}
		}

		{
			ConvexHull PyramidVolume;
			PyramidVolume.TryAddPlane( Plane( Vector( 0.0f, 0.0f, -1.0f ), 0.0f ), EPSILON );
			PyramidVolume.TryAddPlane( Plane( Vector( 1.0f, 0.0f, 1.0f ).GetNormalized(), -0.707107f ), EPSILON );
			PyramidVolume.TryAddPlane( Plane( Vector( -1.0f, 0.0f, 1.0f ).GetNormalized(), -0.707107f ), EPSILON );
			PyramidVolume.TryAddPlane( Plane( Vector( 0.0f, 1.0f, 1.0f ).GetNormalized(), -0.707107f ), EPSILON );
			PyramidVolume.TryAddPlane( Plane( Vector( 0.0f, -1.0f, 1.0f ).GetNormalized(), -0.707107f ), EPSILON );

			// Don't forget to add the bounding box planes!
			// I'm not going to try to make an automatic way to build the convex hull's AABB
			// and bounding planes from its given planes. That would be a tedious exercise
			// to rebuild the original vertices. Instead, I need to be sure to compute that
			// when I *have* the original vertices.
			PyramidVolume.TryAddPlane( Plane( Vector( 0.0f, 0.0f, 1.0f ), -1.0f ), EPSILON );
			PyramidVolume.TryAddPlane( Plane( Vector( 1.0f, 0.0f, 0.0f ), -1.0f ), EPSILON );
			PyramidVolume.TryAddPlane( Plane( Vector( -1.0f, 0.0f, 0.0f ), -1.0f ), EPSILON );
			PyramidVolume.TryAddPlane( Plane( Vector( 0.0f, 1.0f, 0.0f ), -1.0f ), EPSILON );
			PyramidVolume.TryAddPlane( Plane( Vector( 0.0f, -1.0f, 0.0f ), -1.0f ), EPSILON );

			{
				const AABB TestBox = AABB::CreateFromCenterAndExtents( Vector( 1.0f, 0.0f, 1.5f ), Vector( 0.5f, 0.5f, 0.5f ) );
				const Vector Travel = Vector( 0.0f, 0.0f, -2.0f );

				CollisionInfo Info;
				const bool Collision = PyramidVolume.Sweep( TestBox, Travel, &Info );

				TEST( Collision == true );
				TEST( Abs( Info.m_Out_HitT - 0.25f ) < EPSILON );
			}

			{
				const AABB TestBox = AABB::CreateFromCenterAndExtents( Vector( 1.75f, 0.0f, 1.5f ), Vector( 0.5f, 0.5f, 0.5f ) );
				const Vector Travel = Vector( 0.0f, 0.0f, -2.0f );

				CollisionInfo Info;
				const bool Collision = PyramidVolume.Sweep( TestBox, Travel, &Info );

				TEST( Collision == false );
			}
		}
	}

	{
		const Line		Line3		= Line( Vector( 0.0f, 0.0f, 0.0f ), Vector( 0.0f, 0.0f, 1.0f ) );
		const Line		Line4		= Line( Vector( 1.0f, 0.0f, 0.0f ), Vector( 0.0f, 1.0f, 0.0f ) );
		const Vector	Nearest34	= Line3.NearestPointTo( Line4 );
		TEST( ( Nearest34 - Vector() ).Length() < EPSILON );
	}

	int NumFailed = TESTSFAILED;
	ENDTESTS;

	PrintManager::DeleteInstance();

	Allocator::GetDefault().Report( FileStream( "memory_exit_report.txt", FileStream::EFM_Write ) );
	Allocator::GetDefault().ShutDown();

	DEBUGASSERT( _CrtCheckMemory() );
	DEBUGASSERT( !_CrtDumpMemoryLeaks() );

	return NumFailed;
}
