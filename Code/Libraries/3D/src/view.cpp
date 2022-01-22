#include "core.h"
#include "3d.h"
#include "view.h"
#include "matrix.h"
#include "frustum.h"
#include "mathcore.h"
#include "vector4.h"
#include "vector2.h"
#include "box2d.h"
#include "segment2d.h"
#include "collisioninfo2d.h"
#include "mathcore.h"

View::View()
:	m_Location( 0.f, 0.f, 0.f )
,	m_Rotation( 0.f, 0.f, 0.f )
,	m_NearClip( .01f )
,	m_FarClip( 3000.f )
,	m_VerticalFOV( 90.f )
,	m_AspectRatio( kAspect_4_3 )
,	m_VanishingPointY( 0.0f )
,	m_MirrorX( false )
,	m_OrthoMode( false )
,	m_Bounds( SRect( 0, 0, 640, -480 ) )
,	m_OpenGL( false )
,	m_DirtyInvRange( true )
,	m_CACHED_InvRange( 0.0f )
,	m_DirtyViewMatrix( true )
,	m_CACHED_ViewMatrix()
,	m_DirtyProjectionMatrix( true )
,	m_CACHED_ProjectionMatrix()
,	m_DirtyViewProjectionMatrix( true )
,	m_CACHED_ViewProjectionMatrix()
{
}

View::View( const Vector& Location, const Angles& Rotation, float VerticalFOV, float AspectRatio, float NearClip, float FarClip, const float VanishingPointY, const bool MirrorX, const bool OpenGL )
:	m_Location( Location )
,	m_Rotation( Rotation )
,	m_NearClip( NearClip )
,	m_FarClip( FarClip )
,	m_VerticalFOV( VerticalFOV )
,	m_AspectRatio( AspectRatio )
,	m_VanishingPointY( VanishingPointY )
,	m_MirrorX( MirrorX )
,	m_OrthoMode( false )
,	m_Bounds( SRect( 0, 0, 640, -480 ) )
,	m_OpenGL( OpenGL )
,	m_DirtyInvRange( true )
,	m_CACHED_InvRange( 0.0f )
,	m_DirtyViewMatrix( true )
,	m_CACHED_ViewMatrix()
,	m_DirtyProjectionMatrix( true )
,	m_CACHED_ProjectionMatrix()
,	m_DirtyViewProjectionMatrix( true )
,	m_CACHED_ViewProjectionMatrix()
{
}

View::View( const Vector& Location, const Angles& Rotation, const SRect& Bounds, float NearClip, float FarClip, const bool OpenGL )
:	m_Location( Location )
,	m_Rotation( Rotation )
,	m_NearClip( NearClip )
,	m_FarClip( FarClip )
,	m_VerticalFOV( 90.f )
,	m_AspectRatio( 0.0f )
,	m_VanishingPointY( 0.0f )
,	m_MirrorX( false )
,	m_OrthoMode( true )
,	m_Bounds( Bounds )
,	m_OpenGL( OpenGL )
,	m_DirtyInvRange( true )
,	m_CACHED_InvRange( 0.0f )
,	m_DirtyViewMatrix( true )
,	m_CACHED_ViewMatrix()
,	m_DirtyProjectionMatrix( true )
,	m_CACHED_ProjectionMatrix()
,	m_DirtyViewProjectionMatrix( true )
,	m_CACHED_ViewProjectionMatrix()
{
	SetAspectRatio( ( Bounds.m_Right - Bounds.m_Left ) / ( Bounds.m_Bottom - Bounds.m_Top ) );
}

void View::SetLocation( const Vector& Location )
{
	if( Location != m_Location )
	{
		m_Location = Location;
		DirtyViewMatrix();
	}
}

void View::SetRotation( const Angles& Rotation )
{
	if( Rotation != m_Rotation )
	{
		m_Rotation = Rotation;
		DirtyViewMatrix();
	}
}

void View::SetNearClip( const float NearClip )
{
	if( NearClip != m_NearClip )
	{
		m_NearClip = NearClip;
		DirtyInvRange();
	}
}

void View::SetFarClip( const float FarClip )
{
	if( FarClip != m_FarClip )
	{
		m_FarClip = FarClip;
		DirtyInvRange();
	}
}

void View::SetFOV( const float FOV )
{
	if( FOV != m_VerticalFOV )
	{
		m_VerticalFOV = FOV;
		DirtyProjectionMatrix();
	}
}

void View::SetVanishingPointY( const float VanishingPointY )
{
	if( VanishingPointY != m_VanishingPointY )
	{
		m_VanishingPointY = VanishingPointY;
		DirtyProjectionMatrix();
	}
}

void View::SetMirrorX( const bool MirrorX )
{
	if( MirrorX != m_MirrorX )
	{
		m_MirrorX = MirrorX;
		DirtyProjectionMatrix();
	}
}

void View::SetAspectRatio( const float AspectRatio )
{
	if( AspectRatio != m_AspectRatio )
	{
		m_AspectRatio = AspectRatio;
		DirtyProjectionMatrix();
	}
}

void View::SetOrthoBounds( const SRect& Bounds )
{
	if( Bounds != m_Bounds )
	{
		m_Bounds = Bounds;
		DirtyProjectionMatrix();
	}
}

void View::SetClipPlanes( float NearClip, float FarClip )
{
	SetNearClip( NearClip );
	SetFarClip( FarClip );
}

void View::ApplyToFrustum( Frustum& f ) const
{
	f.InitWith( GetViewProjectionMatrix() );
}

const float View::GetInvRange() const
{
	if( m_DirtyInvRange )
	{
		m_DirtyInvRange = false;
		m_CACHED_InvRange = 1.0f / ( m_FarClip - m_NearClip );
	}

	return m_CACHED_InvRange;
}

const Matrix& View::GetViewMatrix() const
{
	if( m_DirtyViewMatrix )
	{
		m_DirtyViewMatrix = false;
		Vector X, Y, Z;
		m_Rotation.GetAxes( X, Y, Z );
		m_CACHED_ViewMatrix = Matrix::CreateViewMatrixCoords( m_Location, X, Y, Z );
	}

	return m_CACHED_ViewMatrix;
}

const Matrix& View::GetProjectionMatrix() const
{
	if( m_DirtyProjectionMatrix )
	{
		m_DirtyProjectionMatrix = false;

		if( m_OrthoMode )
		{
			m_CACHED_ProjectionMatrix = Matrix::CreateOrthoProjectionMatrix( m_Bounds.m_Left, m_Bounds.m_Top, m_Bounds.m_Right, m_Bounds.m_Bottom, m_NearClip, m_FarClip, GetInvRange(), m_OpenGL );
		}
		else
		{
			m_CACHED_ProjectionMatrix = Matrix::CreateProjectionMatrix( m_VerticalFOV, m_NearClip, m_FarClip, GetInvRange(), m_AspectRatio, m_VanishingPointY, m_MirrorX, m_OpenGL );
		}
	}

	return m_CACHED_ProjectionMatrix;
}

const Matrix& View::GetViewProjectionMatrix() const
{
	if( m_DirtyViewProjectionMatrix )
	{
		m_DirtyViewProjectionMatrix = false;
		m_CACHED_ViewProjectionMatrix = GetViewMatrix() * GetProjectionMatrix();
	}

	return m_CACHED_ViewProjectionMatrix;
}

Vector View::Project( const Vector& WorldPos ) const
{
	const Matrix&	VP				= GetViewProjectionMatrix();
	const Vector4	UnprojectedPos	= WorldPos;
	const Vector4	ProjectedPos	= UnprojectedPos * VP;
	const Vector	ScreenPos		= ProjectedPos / ProjectedPos.w;
	return ScreenPos;
}

Vector View::Unproject( const Vector& ScreenPos ) const
{
	const Matrix	InvVP			= GetViewProjectionMatrix().GetInverse();
	const Vector4	ProjectedPos	= ScreenPos;
	const Vector4	UnprojectedPos	= ProjectedPos * InvVP;
	const Vector	WorldPos		= UnprojectedPos / UnprojectedPos.w;
	return WorldPos;
}

Vector4 View::Project4( const Vector& WorldPos ) const
{
	const Matrix&	VP				= GetViewProjectionMatrix();
	const Vector4	UnprojectedPos	= WorldPos;
	const Vector4	ProjectedPos	= UnprojectedPos * VP;

	return ProjectedPos;
}

Vector2 View::ProjectAndClipToScreen( const Vector& WorldPos ) const
{
	const Vector4			ProjectedLocation4D	= Project4( WorldPos );
	Vector2					ProjectedLocation2D;

	if( Abs( ProjectedLocation4D.w ) < EPSILON )
	{
		ProjectedLocation2D = ProjectedLocation4D;
	}
	else
	{
		ProjectedLocation2D = ProjectedLocation4D / ProjectedLocation4D.w;

		if( ProjectedLocation4D.w < 0.0f )
		{
			// Project beyond [-1,1] if location is behind view.
			ProjectedLocation2D = ProjectedLocation2D.GetNormalized() * -2.0f;
		}
	}

	static const Vector2	kScreenScale		= Vector2( 0.5f, -0.5f );
	static const Vector2	kScreenOffset		= Vector2( 0.5f, 0.5f );
	static const Vector2	kScreenMin			= Vector2( 0.0f, 0.0f );
	static const Vector2	kScreenMid			= Vector2( 0.5f, 0.5f );
	static const Vector2	kScreenMax			= Vector2( 1.0f, 1.0f );
	static const Box2D		kScreenBox			= Box2D( kScreenMin, kScreenMax );
	Vector2					ScreenLocation		= ( ProjectedLocation2D * kScreenScale ) + kScreenOffset;
	const Segment2D			ScreenSegment		= Segment2D( ScreenLocation, kScreenMid );

	CollisionInfo2D			Info;
	if( ScreenSegment.Intersects( kScreenBox, &Info ) )
	{
		ScreenLocation = Info.m_Intersection;
	}

	ScreenLocation.x = Clamp( ScreenLocation.x, 0.0f, 1.0f );
	ScreenLocation.y = Clamp( ScreenLocation.y, 0.0f, 1.0f );

	return ScreenLocation;
}
