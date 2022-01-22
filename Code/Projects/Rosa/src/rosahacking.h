#ifndef ROSAHACKING_H
#define ROSAHACKING_H

// ROSATODO: Remove this class.
// Hacking minigame
// Paddle/ball/brick-breaking game
// Board covers (0,0)-(1,1) space, with (0,0) in the upper left corner (to match ortho view space in my engine).

#include "iwbeventobserver.h"
#include "box2d.h"
#include "array.h"
#include "material.h"
#include "wbeventmanager.h"
#include "vector4.h"

class IRenderer;
class InputSystem;
class Mesh;
class ITexture;
class Segment2D;
class WBAction;

class RosaHacking : public IWBEventObserver
{
public:
	RosaHacking();
	~RosaHacking();

	// IWBEventObserver
	virtual void	HandleEvent( const WBEvent& Event );

	void			RegisterForEvents();

	void			InitializeFromDefinition( const SimpleString& DefinitionName );
	void			InitializeBoardFromDefinition( const SimpleString& BoardDef );

	void			Tick( const float DeltaTime );
	void			Render() const;

	bool			IsHacking() const { return m_IsHacking; }

private:
	void			BeginHacking();
	void			EndHacking();

	void			SucceedHack();
	void			FailHack();

	IRenderer*		GetRenderer() const;
	InputSystem*	GetInputSystem() const;

	void			TickMinigame( const float DeltaTime );

	void			MoveBall( const Vector2& StartLocation, Vector2& InOutMovement );
	bool			SweepBall( const Segment2D& SweepSegment, CollisionInfo2D& Info );
	bool			SweepBallAgainst( const Segment2D& SweepSegment, const Box2D& SweepBounds, const Box2D& EntityBox, const void* const pEntity, const HashedString& Desc, CollisionInfo2D& Info );

	void			PlayPaddleSound();
	void			PlayBrickSound();

	Mesh*			CreateQuad( const uint AtlasIndex ) const;

	struct SBrick
	{
		SBrick();
		~SBrick();

		Box2D	m_Box;
		bool	m_Active;
		bool	m_IsBarrier;
		Mesh*	m_Mesh;
	};

	struct SBall
	{
		SBall();
		~SBall();

		bool	m_Launched;
		bool	m_Active;
		Vector2	m_Location;
		Vector2	m_Velocity;
		Vector2	m_Extents;
		Mesh*	m_Mesh;
	};

	struct SPaddle
	{
		SPaddle();
		~SPaddle();

		Vector2	m_Location;
		Vector2	m_Velocity;
		Vector2	m_Extents;
		Mesh*	m_Mesh;
	};

	SBall			m_Ball;
	SPaddle			m_Paddle;
	Array<SBrick>	m_Bricks;

	// Config
	float			m_PaddleSpeed;
	float			m_BallSpeed;
	float			m_BallOffset;	// Float above paddle
	float			m_PaddleOrigin;	// For reflecting ball
	float			m_GameEndDelay;	// Delay before board is closed when succeeding or failing
	Vector4			m_BrickColor;
	Vector4			m_BarrierColor;
	Vector4			m_PaddleColor;
	Vector4			m_BallColor;

	bool			m_IsHacking;
	HashedString	m_InputContext;
	Material		m_Material;
	ITexture*		m_Texture;
	Vector2			m_ScreenDimensions;
	float			m_UVAdjustment;
	SimpleString	m_PaddleSound;
	SimpleString	m_BrickSound;

	Array<WBAction*>	m_SuccessActions;
	Array<WBAction*>	m_FailureActions;

	TEventUID		m_GameEndEventUID;
};

#endif // ROSAHACKING_H
