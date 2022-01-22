#ifndef VIEW_H
#define VIEW_H

#include "matrix.h"
#include "vector.h"
#include "angles.h"
#include "3d.h"

class Frustum;
class Vector4;

// A view describes the view and projection matrices for
// a 3D scene--could be called a camera, but that name
// is better reserved for a gameplay device.

class View
{
public:
	View();

	// Perspective mode constructor
	View( const Vector& Location, const Angles& Rotation, float VerticalFOV, float AspectRatio, float NearClip, float FarClip, const float VanishingPointY, const bool MirrorX, const bool OpenGL );

	// Ortho mode constructors
	View( const Vector& Location, const Angles& Rotation, const SRect& Bounds, float NearClip, float FarClip, const bool OpenGL );

	const Vector&	GetLocation() const { return m_Location; }
	void			SetLocation( const Vector& Location );

	const Angles&	GetRotation() const { return m_Rotation; }
	void			SetRotation( const Angles& Rotation );

	float			GetNearClip() const { return m_NearClip; }
	void			SetNearClip( const float NearClip );

	float			GetFarClip() const { return m_FarClip; }
	void			SetFarClip( const float FarClip );

	float			GetFOV() const { return m_VerticalFOV; }
	void			SetFOV( const float FOV );

	float			GetVanishingPointY() const { return m_VanishingPointY; }
	void			SetVanishingPointY( const float VanishingPointY );

	bool			GetMirrorX() const { return m_MirrorX; }
	void			SetMirrorX( const bool MirrorX );

	float			GetAspectRatio() const { return m_AspectRatio; }
	void			SetAspectRatio( const float AspectRatio );

	void			SetOrthoBounds( const SRect& Bounds );

	void			SetClipPlanes( float NearClip, float FarClip );

	void			ApplyToFrustum( Frustum& f ) const;

	const float		GetInvRange() const;

	const Matrix&	GetViewMatrix() const;
	const Matrix&	GetProjectionMatrix() const;
	const Matrix&	GetViewProjectionMatrix() const;

	Vector4	Project4( const Vector& WorldPos ) const;
	Vector2	ProjectAndClipToScreen( const Vector& WorldPos ) const;	// Returns X,Y in range [0,1]

	Vector	Project( const Vector& WorldPos ) const;
	Vector	Unproject( const Vector& ScreenPos ) const;

private:
	void	DirtyInvRange()				{ m_DirtyInvRange = true; }
	void	DirtyViewMatrix()			{ m_DirtyViewMatrix = true; m_DirtyViewProjectionMatrix = true; }
	void	DirtyProjectionMatrix()		{ m_DirtyProjectionMatrix = true; m_DirtyViewProjectionMatrix = true; }

	Vector	m_Location;
	Angles	m_Rotation;
	float	m_NearClip;
	float	m_FarClip;

	// Perspective mode
	float	m_VerticalFOV;	// Degrees, not radians!
	float	m_AspectRatio;
	float	m_VanishingPointY;	// Equivalent to the y offset for the reticle on UI
	bool	m_MirrorX;

	// Ortho mode
	bool	m_OrthoMode;
	SRect	m_Bounds;

	// HACKHACK
	bool	m_OpenGL;

	// Cached stuff for perf I guess
	mutable bool	m_DirtyInvRange;
	mutable float	m_CACHED_InvRange;	// 1/(far-near)
	mutable bool	m_DirtyViewMatrix;
	mutable Matrix	m_CACHED_ViewMatrix;
	mutable bool	m_DirtyProjectionMatrix;
	mutable Matrix	m_CACHED_ProjectionMatrix;
	mutable bool	m_DirtyViewProjectionMatrix;
	mutable Matrix	m_CACHED_ViewProjectionMatrix;
};

#endif // VIEW_H
