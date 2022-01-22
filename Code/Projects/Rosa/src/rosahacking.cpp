#include "core.h"
#include "rosahacking.h"
#include "wbworld.h"
#include "configmanager.h"
#include "rosaframework.h"
#include "inputsystem.h"
#include "mathcore.h"
#include "rosagame.h"
#include "wbentity.h"
#include "mesh.h"
#include "irenderer.h"
#include "ivertexdeclaration.h"
#include "ivertexbuffer.h"
#include "iindexbuffer.h"
#include "texturemanager.h"
#include "mathfunc.h"
#include "segment2d.h"
#include "collisioninfo2d.h"
#include "wbaction.h"
#include "wbactionfactory.h"
#include "wbactionstack.h"
#include "Achievements/iachievementmanager.h"
#include "Components/wbcompstatmod.h"
#include "vector2.h"
#include "hsv.h"
#include "rosadifficulty.h"

#if BUILD_DEV
#include "keyboard.h"
#endif

RosaHacking::SBrick::SBrick()
:	m_Box()
,	m_Active( false )
,	m_IsBarrier( false )
,	m_Mesh( NULL )
{
}

RosaHacking::SBrick::~SBrick()
{
	SafeDelete( m_Mesh );
}

RosaHacking::SBall::SBall()
:	m_Launched( false )
,	m_Active( false )
,	m_Location()
,	m_Velocity()
,	m_Extents()
,	m_Mesh( NULL )
{
}

RosaHacking::SBall::~SBall()
{
	SafeDelete( m_Mesh );
}

RosaHacking::SPaddle::SPaddle()
:	m_Location()
,	m_Velocity()
,	m_Extents()
,	m_Mesh( NULL )
{
}

RosaHacking::SPaddle::~SPaddle()
{
	SafeDelete( m_Mesh );
}

RosaHacking::RosaHacking()
:	m_Ball()
,	m_Paddle()
,	m_Bricks()
,	m_PaddleSpeed( 0.0f )
,	m_BallSpeed( 0.0f )
,	m_BallOffset( 0.0f )
,	m_PaddleOrigin( 0.0f )
,	m_GameEndDelay( 0.0f )
,	m_BrickColor()
,	m_BarrierColor()
,	m_PaddleColor()
,	m_BallColor()
,	m_IsHacking( false )
,	m_InputContext()
,	m_Material()
,	m_Texture( NULL )
,	m_ScreenDimensions()
,	m_UVAdjustment( 0.0f )
,	m_PaddleSound()
,	m_BrickSound()
,	m_SuccessActions()
,	m_FailureActions()
,	m_GameEndEventUID( 0 )
{
	m_Bricks.SetDeflate( false );

	InitializeFromDefinition( "RosaHacking" );

	RegisterForEvents();
}

RosaHacking::~RosaHacking()
{
	// I don't unregister for events here because world has already been destroyed. Assumptions!

	WBActionFactory::ClearActionArray( m_SuccessActions );
	WBActionFactory::ClearActionArray( m_FailureActions );
}

void RosaHacking::RegisterForEvents()
{
	STATIC_HASHED_STRING( StartHack );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sStartHack, this, NULL );

	STATIC_HASHED_STRING( StopHack );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sStopHack, this, NULL );

	STATIC_HASHED_STRING( SucceedHack );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sSucceedHack, this, NULL );

	STATIC_HASHED_STRING( FailHack );
	WBWorld::GetInstance()->GetEventManager()->AddObserver( sFailHack, this, NULL );
}

void RosaHacking::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( InputContext );
	m_InputContext = ConfigManager::GetHash( sInputContext, HashedString::NullString, sDefinitionName );

	STATICHASH( Material );
	const SimpleString Material = ConfigManager::GetString( sMaterial, "", sDefinitionName );
	IRenderer* const pRenderer = GetRenderer();
	m_Material.SetDefinition( Material, pRenderer, VD_POSITIONS | VD_UVS );

	STATICHASH( Texture );
	const SimpleString Texture = ConfigManager::GetString( sTexture, "", sDefinitionName );
	m_Texture = pRenderer->GetTextureManager()->GetTexture( Texture.CStr() );
	ASSERT( m_Texture );

	STATICHASH( RTWidth );
	m_ScreenDimensions.x = ConfigManager::GetFloat( sRTWidth, 0.0f, sDefinitionName );

	STATICHASH( RTHeight );
	m_ScreenDimensions.y = ConfigManager::GetFloat( sRTHeight, 0.0f, sDefinitionName );

	STATICHASH( UVAdjustment );
	m_UVAdjustment = ConfigManager::GetFloat( sUVAdjustment, 0.0f, sDefinitionName );

	STATICHASH( PaddleSound );
	m_PaddleSound = ConfigManager::GetString( sPaddleSound, "", sDefinitionName );

	STATICHASH( BrickSound );
	m_BrickSound = ConfigManager::GetString( sBrickSound, "", sDefinitionName );

	STATICHASH( BallOffset );
	m_BallOffset = ConfigManager::GetFloat( sBallOffset, 0.0f, sDefinitionName );

	STATICHASH( PaddleOrigin );
	m_PaddleOrigin = ConfigManager::GetFloat( sPaddleOrigin, 0.0f, sDefinitionName );

	STATICHASH( PaddleExtentsY );
	m_Paddle.m_Extents.y = ConfigManager::GetFloat( sPaddleExtentsY, 0.0f, sDefinitionName );

	STATICHASH( BallExtents );
	const float BallExtents = ConfigManager::GetFloat( sBallExtents, 0.0f, sDefinitionName );
	m_Ball.m_Extents = Vector2( BallExtents, BallExtents );

	STATICHASH( GameEndDelay );
	m_GameEndDelay = ConfigManager::GetInheritedFloat( sGameEndDelay, 0.0f, sDefinitionName );

	{
		Vector BrickColorHSV;

		STATICHASH( BrickColorH );
		BrickColorHSV.x = ConfigManager::GetInheritedFloat( sBrickColorH, 0.0f, sDefinitionName );

		STATICHASH( BrickColorS );
		BrickColorHSV.y = ConfigManager::GetInheritedFloat( sBrickColorS, 0.0f, sDefinitionName );

		STATICHASH( BrickColorV );
		BrickColorHSV.z = ConfigManager::GetInheritedFloat( sBrickColorV, 0.0f, sDefinitionName );

		m_BrickColor = HSV::HSVToRGB( BrickColorHSV );
	}

	{
		Vector BarrierColorHSV;

		STATICHASH( BarrierColorH );
		BarrierColorHSV.x = ConfigManager::GetInheritedFloat( sBarrierColorH, 0.0f, sDefinitionName );

		STATICHASH( BarrierColorS );
		BarrierColorHSV.y = ConfigManager::GetInheritedFloat( sBarrierColorS, 0.0f, sDefinitionName );

		STATICHASH( BarrierColorV );
		BarrierColorHSV.z = ConfigManager::GetInheritedFloat( sBarrierColorV, 0.0f, sDefinitionName );

		m_BarrierColor = HSV::HSVToRGB( BarrierColorHSV );
	}

	{
		Vector PaddleColorHSV;

		STATICHASH( PaddleColorH );
		PaddleColorHSV.x = ConfigManager::GetInheritedFloat( sPaddleColorH, 0.0f, sDefinitionName );

		STATICHASH( PaddleColorS );
		PaddleColorHSV.y = ConfigManager::GetInheritedFloat( sPaddleColorS, 0.0f, sDefinitionName );

		STATICHASH( PaddleColorV );
		PaddleColorHSV.z = ConfigManager::GetInheritedFloat( sPaddleColorV, 0.0f, sDefinitionName );

		m_PaddleColor = HSV::HSVToRGB( PaddleColorHSV );
	}

	{
		Vector BallColorHSV;

		STATICHASH( BallColorH );
		BallColorHSV.x = ConfigManager::GetInheritedFloat( sBallColorH, 0.0f, sDefinitionName );

		STATICHASH( BallColorS );
		BallColorHSV.y = ConfigManager::GetInheritedFloat( sBallColorS, 0.0f, sDefinitionName );

		STATICHASH( BallColorV );
		BallColorHSV.z = ConfigManager::GetInheritedFloat( sBallColorV, 0.0f, sDefinitionName );

		m_BallColor = HSV::HSVToRGB( BallColorHSV );
	}

	WBActionFactory::InitializeActionArray( sDefinitionName, "Success", m_SuccessActions );
	WBActionFactory::InitializeActionArray( sDefinitionName, "Failure", m_FailureActions );
}

void RosaHacking::InitializeBoardFromDefinition( const SimpleString& BoardDef )
{
	MAKEHASH( BoardDef );

	STATICHASH( NumBricksX );
	const uint NumBricksX = ConfigManager::GetInheritedInt( sNumBricksX, 0, sBoardDef );
	DEVASSERT( NumBricksX );

	STATICHASH( NumBricksY );
	const uint NumBricksY = ConfigManager::GetInheritedInt( sNumBricksY, 0, sBoardDef );
	DEVASSERT( NumBricksY );

	STATICHASH( BoardHeight );
	const uint BoardHeight = ConfigManager::GetInheritedInt( sBoardHeight, 0, sBoardDef );
	DEVASSERT( BoardHeight );

	STATICHASH( PaddleSpeed );
	m_PaddleSpeed = ConfigManager::GetInheritedFloat( sPaddleSpeed, 0.0f, sBoardDef );

	STATICHASH( BallSpeed );
	m_BallSpeed = ConfigManager::GetInheritedFloat( sBallSpeed, 0.0f, sBoardDef );

	STATICHASH( PaddleExtentsX );
	m_Paddle.m_Extents.x = ConfigManager::GetFloat( sPaddleExtentsX, 0.0f, sBoardDef );

	const uint NumBricks = NumBricksX * NumBricksY;
	m_Bricks.Clear();	// This frees existing meshes by invoking SBrick dtor
	m_Bricks.Reserve( NumBricks );

	const float BrickWidth	= 1.0f / static_cast<float>( NumBricksX );
	const float BrickHeight	= 1.0f / static_cast<float>( BoardHeight );

	// Initialize bricks
	for( uint BrickY = 0; BrickY < NumBricksY; ++BrickY )
	{
		for( uint BrickX = 0; BrickX < NumBricksX; ++BrickX )
		{
			SBrick& Brick			= m_Bricks.PushBack();

			Brick.m_Box.m_Min.x		= BrickWidth * static_cast<float>( BrickX );
			Brick.m_Box.m_Max.x		= BrickWidth * static_cast<float>( BrickX + 1 );
			Brick.m_Box.m_Min.y		= BrickHeight * static_cast<float>( BrickY );
			Brick.m_Box.m_Max.y		= BrickHeight * static_cast<float>( BrickY + 1 );

			// Brick types are defined row major! Easier conceptually for me to author.
			const SimpleString BrickTypeVar	= SimpleString::PrintF( "BrickType%d,%d", BrickY, BrickX );
			MAKEHASH( BrickTypeVar );
			const HashedString BrickType	= ConfigManager::GetInheritedHash( sBrickTypeVar, HashedString::NullString, sBoardDef );

			STATIC_HASHED_STRING( Null );
			STATIC_HASHED_STRING( Barrier );

			Brick.m_Active		= ( BrickType != sNull );
			Brick.m_IsBarrier	= ( BrickType == sBarrier );

			const uint AtlasIndex			= Brick.m_IsBarrier ? 3 : 0;
			Brick.m_Mesh					= CreateQuad( AtlasIndex );
			Brick.m_Mesh->m_Location		= Brick.m_Box.GetCenter();
			Brick.m_Mesh->m_Scale			= Brick.m_Box.GetExtents();
			Brick.m_Mesh->m_Material		= m_Material;
			Brick.m_Mesh->SetBucket( "Hacking" );
			Brick.m_Mesh->SetTexture( 0, m_Texture );
			Brick.m_Mesh->m_ConstantColor	= Brick.m_IsBarrier ? m_BarrierColor : m_BrickColor;
		}
	}

	// Initialize paddle
	m_Paddle.m_Location	= Vector2( 0.5f, 1.0f - m_Paddle.m_Extents.y );
	m_Paddle.m_Velocity	= Vector2( 0.0f, 0.0f );
	SafeDelete( m_Paddle.m_Mesh );
	m_Paddle.m_Mesh						= CreateQuad( 1 );
	m_Paddle.m_Mesh->m_Location			= m_Paddle.m_Location;
	m_Paddle.m_Mesh->m_Scale			= m_Paddle.m_Extents;
	m_Paddle.m_Mesh->m_Material			= m_Material;
	m_Paddle.m_Mesh->SetBucket( "Hacking" );
	m_Paddle.m_Mesh->SetTexture( 0, m_Texture );
	m_Paddle.m_Mesh->m_ConstantColor	= m_PaddleColor;

	// Initialize ball
	m_Ball.m_Launched	= false;
	m_Ball.m_Active		= true;
	m_Ball.m_Location	= Vector2( 0.5f, 1.0f );
	m_Ball.m_Velocity	= Vector2( 0.0f, 0.0f );
	SafeDelete( m_Ball.m_Mesh );
	m_Ball.m_Mesh					= CreateQuad( 2 );
	m_Ball.m_Mesh->m_Location		= m_Ball.m_Location;
	m_Ball.m_Mesh->m_Scale			= m_Ball.m_Extents;
	m_Ball.m_Mesh->m_Material		= m_Material;
	m_Ball.m_Mesh->SetBucket( "Hacking" );
	m_Ball.m_Mesh->SetTexture( 0, m_Texture );
	m_Ball.m_Mesh->m_ConstantColor	= m_BallColor;
}

Mesh* RosaHacking::CreateQuad( const uint AtlasIndex ) const
{
	XTRACE_FUNCTION;

	static const int	kNumVertices	= 4;
	static const int	kNumIndices		= 6;

	Array<Vector> Positions;
	Positions.Resize( kNumVertices );

	Array<Vector2> UVs;
	UVs.Resize( kNumVertices );

	Array<index_t> Indices;
	Indices.Resize( kNumIndices );

	Positions[0]	= Vector( -1.0f, 1.0f, 0.0f );
	Positions[1]	= Vector( 1.0f, 1.0f, 0.0f );
	Positions[2]	= Vector( -1.0f, -1.0f, 0.0f );
	Positions[3]	= Vector( 1.0f, -1.0f, 0.0f );

#define ATLAS_DIM   2
#define ATLAS_DIM_F 2.0f

	static const float	kAtlasStep	= 1.0f / ATLAS_DIM_F;
	const uint	AtlasIndexX		= AtlasIndex % ATLAS_DIM;
	const uint	AtlasIndexY		= AtlasIndex / ATLAS_DIM;
	const float UVAdjustment	= m_UVAdjustment / ATLAS_DIM_F;
	const float	U0				= ( static_cast<float>( AtlasIndexX ) * kAtlasStep ) + UVAdjustment;
	const float	V0				= ( static_cast<float>( AtlasIndexY ) * kAtlasStep ) + UVAdjustment;
	const float	U1				= ( static_cast<float>( AtlasIndexX + 1 ) * kAtlasStep ) - UVAdjustment;
	const float V1				= ( static_cast<float>( AtlasIndexY + 1 ) * kAtlasStep ) - UVAdjustment;

	UVs[0]			= Vector2( U0, V1 );
	UVs[1]			= Vector2( U1, V1 );
	UVs[2]			= Vector2( U0, V0 );
	UVs[3]			= Vector2( U1, V0 );

	Indices[0]		= 0;
	Indices[1]		= 1;
	Indices[2]		= 2;
	Indices[3]		= 1;
	Indices[4]		= 3;
	Indices[5]		= 2;

	IRenderer* const	pRenderer			= GetRenderer();

	IVertexBuffer*		pVertexBuffer		= pRenderer->CreateVertexBuffer();
	IVertexDeclaration*	pVertexDeclaration	= pRenderer->GetVertexDeclaration( VD_POSITIONS | VD_UVS );
	IIndexBuffer*		pIndexBuffer		= pRenderer->CreateIndexBuffer();
	IVertexBuffer::SInit InitStruct;
	InitStruct.NumVertices	= kNumVertices;
	InitStruct.Positions	= Positions.GetData();
	InitStruct.UVs			= UVs.GetData();
	pVertexBuffer->Init( InitStruct );
	pIndexBuffer->Init( kNumIndices, Indices.GetData() );
	pIndexBuffer->SetPrimitiveType( EPT_TRIANGLELIST );

	Mesh* const pQuadMesh = new Mesh( pVertexBuffer, pVertexDeclaration, pIndexBuffer );

	pQuadMesh->SetAABB( AABB( Vector( -0.5f, -0.5f, 0.0f ), Vector( 0.5f, 0.5f, 0.0f ) ) );
#if BUILD_DEBUG
	pQuadMesh->m_DEBUG_Name = "HackingQuad";
#endif

	return pQuadMesh;
}

IRenderer* RosaHacking::GetRenderer() const
{
	return RosaFramework::GetInstance()->GetRenderer();
}

InputSystem* RosaHacking::GetInputSystem() const
{
	return RosaFramework::GetInstance()->GetInputSystem();
}

/*virtual*/ void RosaHacking::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	STATIC_HASHED_STRING( StartHack );
	STATIC_HASHED_STRING( StopHack );
	STATIC_HASHED_STRING( SucceedHack );
	STATIC_HASHED_STRING( FailHack );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sStartHack )
	{
		STATIC_HASHED_STRING( BoardDef );
		const SimpleString BoardDef = Event.GetString( sBoardDef );

		InitializeBoardFromDefinition( BoardDef );
		BeginHacking();
		TickMinigame( 0.0f );	// Tick game (but not input) once. Otherwise, the board doesn't get a chance to tick before the first render.
	}
	else if( EventName == sStopHack )
	{
		EndHacking();
	}
	else if( EventName == sSucceedHack )
	{
		SucceedHack();
	}
	else if( EventName == sFailHack )
	{
		FailHack();
	}
}

void RosaHacking::Tick( const float DeltaTime )
{
	XTRACE_FUNCTION;

	if( !m_IsHacking )
	{
		return;
	}

	InputSystem* const	pInputSystem	= GetInputSystem();

	STATIC_HASHED_STRING( HackLeft );
	STATIC_HASHED_STRING( HackRight );
	STATIC_HASHED_STRING( HackMoveX );
	STATIC_HASHED_STRING( HackMoveXAlt );
	STATIC_HASHED_STRING( HackUse );
	STATIC_HASHED_STRING( HackUseAlt );
	STATIC_HASHED_STRING( HackUseAlt2 );
	STATIC_HASHED_STRING( HackExit );
	STATIC_HASHED_STRING( HackExitAlt );

	if( pInputSystem->OnRise( sHackUse ) ||
		pInputSystem->OnRise( sHackUseAlt ) ||
		pInputSystem->OnRise( sHackUseAlt2 ) )
	{
		if( m_Ball.m_Launched )
		{
			// Do nothing
		}
		else
		{
			const float		LaunchXFactor	= Math::Random( -1.0f, 1.0f );
			const Vector2	LaunchNormal	= Vector2( LaunchXFactor, -1.0f ).GetNormalized();
			m_Ball.m_Launched = true;
			m_Ball.m_Velocity = m_BallSpeed * LaunchNormal;

			PlayPaddleSound();
		}
	}

	float Movement		= 0.0f;
	if( pInputSystem->IsHigh( sHackRight ) )	{ Movement += 1.0f; }
	if( pInputSystem->IsHigh( sHackLeft ) )		{ Movement -= 1.0f; }
	Movement += pInputSystem->GetPosition( sHackMoveX );
	Movement += pInputSystem->GetPosition( sHackMoveXAlt );
	Movement = Clamp( Movement, -1.0f, 1.0f );
	m_Paddle.m_Velocity.x = Movement * m_PaddleSpeed;

	TickMinigame( DeltaTime );

	if( pInputSystem->OnRise( sHackExit ) || pInputSystem->OnRise( sHackExitAlt ) )
	{
		EndHacking();
	}

#if BUILD_DEV
	// Alt + H succeeds hacking instantly
	Keyboard* const pKeyboard = RosaFramework::GetInstance()->GetKeyboard();
	if( pKeyboard->IsHigh( Keyboard::EB_Virtual_Alt ) && pKeyboard->OnRise( Keyboard::EB_H ) )
	{
		SucceedHack();
	}
#endif // BUILD_DEV
}

void RosaHacking::TickMinigame( const float DeltaTime )
{
	XTRACE_FUNCTION;

	// This can happen if we're autohacking, for instance
	if( !m_IsHacking )
	{
		return;
	}

	m_Paddle.m_Location	+= DeltaTime * m_Paddle.m_Velocity;
	m_Paddle.m_Location.x = Clamp( m_Paddle.m_Location.x, m_Paddle.m_Extents.x, 1.0f - m_Paddle.m_Extents.x );

	if( m_Ball.m_Active )
	{
		if( m_Ball.m_Launched )
		{
			// Do normal ball physics
			Vector2 BallMovement = DeltaTime * m_Ball.m_Velocity;
			MoveBall( m_Ball.m_Location, BallMovement );

			m_Ball.m_Location += BallMovement;
		}
		else
		{
			// Just move ball with paddle
			const float BallOffsetY	= m_Paddle.m_Extents.y + m_Ball.m_Extents.y + m_BallOffset;
			m_Ball.m_Location		= m_Paddle.m_Location;
			m_Ball.m_Location.y		-= BallOffsetY;
		}
	}
}

bool RosaHacking::SweepBallAgainst( const Segment2D& SweepSegment, const Box2D& SweepBounds, const Box2D& EntityBox, const void* const pEntity, const HashedString& Desc, CollisionInfo2D& Info )
{
	// Broad test against sweep region
	if( !EntityBox.Intersects( SweepBounds ) )
	{
		return false;
	}

	CollisionInfo2D CheckInfo;
	CheckInfo.CopyInParametersFrom( Info );
	if( !SweepSegment.Intersects( EntityBox, &CheckInfo ) )
	{
		return false;
	}

	if( Info.m_Collision && CheckInfo.m_HitT >= Info.m_HitT )
	{
		return false;
	}

	Info.CopyOutParametersFrom( CheckInfo );
	Info.m_HitEntity	= const_cast<void*>( pEntity );
	Info.m_HitName		= Desc;

	return true;
}

bool RosaHacking::SweepBall( const Segment2D& SweepSegment, CollisionInfo2D& Info )
{
	CollisionInfo2D MinInfo;

	// Make a bound for the sweep segment; don't add extents because they're added to the other box
	Vector2	SweepMinCorner( Min( SweepSegment.m_Start.x, SweepSegment.m_End.x ), Min( SweepSegment.m_Start.y, SweepSegment.m_End.y ) );
	Vector2	SweepMaxCorner( Max( SweepSegment.m_Start.x, SweepSegment.m_End.x ), Max( SweepSegment.m_Start.y, SweepSegment.m_End.y ) );
	Box2D	SweepBounds( SweepMinCorner, SweepMaxCorner );

	// Sweep against bricks
	FOR_EACH_ARRAY( BrickIter, m_Bricks, SBrick )
	{
		const SBrick& Brick = BrickIter.GetValue();
		if( Brick.m_Active )
		{
			STATIC_HASHED_STRING( Brick );
			Box2D BrickBox = Brick.m_Box;
			BrickBox.ExpandBy( m_Ball.m_Extents );
			SweepBallAgainst( SweepSegment, SweepBounds, BrickBox, &Brick, sBrick, MinInfo );
		}
	}

	// Sweep against board
	{
		STATIC_HASHED_STRING( Wall );
		STATIC_HASHED_STRING( Top );
		STATIC_HASHED_STRING( Bottom );

		static const Vector2	skLeftWallCenter	= Vector2( 0.0f, 0.5f );
		static const Vector2	skRightWallCenter	= Vector2( 1.0f, 0.5f );
		static const Vector2	skTopWallCenter		= Vector2( 0.5f, 0.0f );
		static const Vector2	skBottomWallCenter	= Vector2( 0.5f, 1.0f );
		static const Vector2	skSideWallExtents	= Vector2( 0.0f, 0.5f );
		static const Vector2	skEndWallExtents	= Vector2( 0.5f, 0.0f );

		const Box2D				LeftWallBox			= Box2D::CreateFromCenterAndExtents( skLeftWallCenter, skSideWallExtents + m_Ball.m_Extents );
		SweepBallAgainst( SweepSegment, SweepBounds, LeftWallBox, NULL, sWall, MinInfo );

		const Box2D				RightWallBox		= Box2D::CreateFromCenterAndExtents( skRightWallCenter, skSideWallExtents + m_Ball.m_Extents );
		SweepBallAgainst( SweepSegment, SweepBounds, RightWallBox, NULL, sWall, MinInfo );

		const Box2D				TopWallBox			= Box2D::CreateFromCenterAndExtents( skTopWallCenter, skEndWallExtents + m_Ball.m_Extents );
		SweepBallAgainst( SweepSegment, SweepBounds, TopWallBox, NULL, sTop, MinInfo );

		const Box2D				BottomWallBox		= Box2D::CreateFromCenterAndExtents( skBottomWallCenter, skEndWallExtents + m_Ball.m_Extents );
		SweepBallAgainst( SweepSegment, SweepBounds, BottomWallBox, NULL, sBottom, MinInfo );
	}

	Info.CopyOutParametersFrom( MinInfo );
	return Info.m_Collision;
}

void RosaHacking::MoveBall( const Vector2& StartLocation, Vector2& InOutMovement )
{
	bool CurrentCollision	= false;

	do
	{
		CurrentCollision = false;

		CollisionInfo2D Info;
		Segment2D SweepSegment( StartLocation, StartLocation + InOutMovement );
		if( SweepBall( SweepSegment, Info ) )
		{
			// Collision response
			CurrentCollision	= true;

			if( Info.m_EHitNormal == CollisionInfo2D::EHN_Left ||
				Info.m_EHitNormal == CollisionInfo2D::EHN_Right )
			{
				InOutMovement.x		= 0.0f;
				m_Ball.m_Velocity.x	= -m_Ball.m_Velocity.x;
			}
			else if(
				Info.m_EHitNormal == CollisionInfo2D::EHN_Up ||
				Info.m_EHitNormal == CollisionInfo2D::EHN_Down )
			{
				InOutMovement.y		= 0.0f;
				m_Ball.m_Velocity.y	= -m_Ball.m_Velocity.y;
			}
			else
			{
				// Complete collision. Bail out. Shouldn't happen except with paddle.
				WARN;
				InOutMovement.x	= 0.0f;
				InOutMovement.y	= 0.0f;
			}

			STATIC_HASHED_STRING( Brick );
			if( Info.m_HitName == sBrick )
			{
				// Deactivate hit brick
				DEVASSERT( Info.m_HitEntity );
				SBrick* const pBrick	= static_cast<SBrick*>( Info.m_HitEntity );
				if( !pBrick->m_IsBarrier )
				{
					pBrick->m_Active		= false;
				}

				PlayBrickSound();
			}

			// Subtract a ball or end hack when we hit the bottom of the board
			STATIC_HASHED_STRING( Bottom );
			if( Info.m_HitName == sBottom )
			{
				m_Ball.m_Active = false;
				WB_MAKE_EVENT( FailHack, NULL );
				m_GameEndEventUID = WB_QUEUE_EVENT_DELAY( WBWorld::GetInstance()->GetEventManager(), FailHack, NULL, m_GameEndDelay );
			}

			// Complete hacking when we hit the top
			STATIC_HASHED_STRING( Top );
			if( Info.m_HitName == sTop )
			{
				m_Ball.m_Active = false;
				WB_MAKE_EVENT( SucceedHack, NULL );
				m_GameEndEventUID = WB_QUEUE_EVENT_DELAY( WBWorld::GetInstance()->GetEventManager(), SucceedHack, NULL, m_GameEndDelay );
			}
		}
	}
	while( InOutMovement.LengthSquared() > 0.0f && CurrentCollision );

	// Finally, separately sweep against paddle (which we *don't* collide against).
	// This prevents the paddle being used to create a complete collision and push
	// the ball off the board.
	if( InOutMovement.y > 0.0f )
	{
		Segment2D SweepSegment( StartLocation, StartLocation + InOutMovement );

		Vector2	SweepMinCorner( Min( SweepSegment.m_Start.x, SweepSegment.m_End.x ), Min( SweepSegment.m_Start.y, SweepSegment.m_End.y ) );
		Vector2	SweepMaxCorner( Max( SweepSegment.m_Start.x, SweepSegment.m_End.x ), Max( SweepSegment.m_Start.y, SweepSegment.m_End.y ) );
		Box2D	SweepBounds( SweepMinCorner, SweepMaxCorner );

		const Box2D PaddleBox = Box2D::CreateFromCenterAndExtents( m_Paddle.m_Location, m_Paddle.m_Extents + m_Ball.m_Extents );

		CollisionInfo2D Info;
		if( SweepBallAgainst( SweepSegment, SweepBounds, PaddleBox, NULL, HashedString::NullString, Info ) )
		{
			InOutMovement.x = 0.0f;
			InOutMovement.y = 0.0f;

			// Reflect up regardless of hit normal, and redirect vector of travel according to where the paddle was hit
			// What I'm doing to determine the vector of travel:
			// Get the direction to the ball from a point behind the paddle; this makes
			// sure the ball will always travel forward, it makes the valid angles of
			// travel scale as the paddle gets longer, and it simplifies the math.
			const Vector2	BehindPaddle	= Vector2( m_Paddle.m_Location.x, m_Paddle.m_Location.y + m_PaddleOrigin );
			const Vector2	Direction		= ( Info.m_Intersection - BehindPaddle ).GetNormalized();
			m_Ball.m_Velocity				= m_BallSpeed * Direction;

			PlayPaddleSound();
		}
	}
}

void RosaHacking::PlayPaddleSound()
{
	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();
	WBEntity* const			pPlayer			= RosaGame::GetPlayer();

	WB_MAKE_EVENT( PlaySoundDef, pPlayer );
	WB_SET_AUTO( PlaySoundDef, Hash, Sound, m_PaddleSound );
	WB_DISPATCH_EVENT( pEventManager, PlaySoundDef, pPlayer );
}

void RosaHacking::PlayBrickSound()
{
	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();
	WBEntity* const			pPlayer			= RosaGame::GetPlayer();

	WB_MAKE_EVENT( PlaySoundDef, pPlayer );
	WB_SET_AUTO( PlaySoundDef, Hash, Sound, m_BrickSound );
	WB_DISPATCH_EVENT( pEventManager, PlaySoundDef, pPlayer );
}

void RosaHacking::Render() const
{
	XTRACE_FUNCTION;

	if( !m_IsHacking )
	{
		return;
	}

	IRenderer* const	pRenderer				= GetRenderer();

	if( m_Ball.m_Active )
	{
		m_Ball.m_Mesh->m_Location = m_Ball.m_Location;
		pRenderer->AddMesh( m_Ball.m_Mesh );
	}

	m_Paddle.m_Mesh->m_Location = m_Paddle.m_Location;
	pRenderer->AddMesh( m_Paddle.m_Mesh );

	FOR_EACH_ARRAY( BrickIter, m_Bricks, SBrick )
	{
		const SBrick& Brick = BrickIter.GetValue();
		if( Brick.m_Active )
		{
			pRenderer->AddMesh( Brick.m_Mesh );
		}
	}
}

void RosaHacking::SucceedHack()
{
	INCREMENT_STAT( "NumHacksCompleted", 1 );

	WBActionFactory::ExecuteActionArray( m_SuccessActions, WBEvent(), NULL );
	EndHacking();
}

void RosaHacking::FailHack()
{
	WBActionFactory::ExecuteActionArray( m_FailureActions, WBEvent(), NULL );
	EndHacking();
}

void RosaHacking::BeginHacking()
{
	if( m_IsHacking )
	{
		return;
	}

	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();
	WBEntity* const			pPlayer			= RosaGame::GetPlayer();

	m_IsHacking = true;

	INCREMENT_STAT( "NumHacksAttempted", 1 );

	// Unqueue any delayed game end events
	// (We don't do this when ending hacking, because the player may manually close the
	// hack screen while waiting for the SucceedHack event to be dispatched.)
	pEventManager->UnqueueEvent( m_GameEndEventUID );

	// Push input context
	GetInputSystem()->PushContext( m_InputContext );

	// Disable frobbing to hide the prompt
	{
		WB_MAKE_EVENT( DisableFrob, NULL );
		WB_DISPATCH_EVENT( pEventManager, DisableFrob, pPlayer );
	}

	// Push the hack UI screen
	{
		STATIC_HASHED_STRING( HackScreen );
		WB_MAKE_EVENT( PushUIScreen, NULL );
		WB_SET_AUTO( PushUIScreen, Hash, Screen, sHackScreen );
		WB_DISPATCH_EVENT( pEventManager, PushUIScreen, NULL );
	}

	// Hide the reticle
	{
		STATIC_HASHED_STRING( HUD );
		STATIC_HASHED_STRING( Crosshair );
		WB_MAKE_EVENT( SetWidgetHidden, NULL );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sCrosshair );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, true );
		WB_DISPATCH_EVENT( pEventManager, SetWidgetHidden, NULL );
	}

	// If we're autohacking or playing in tourist mode, immediately succeed.
	// We still want to do all the hack screen setup so we don't need a
	// separate code path for EndHacking.
	WBCompStatMod* const pStatMod = WB_GETCOMP( pPlayer, StatMod );
	WB_MODIFY_FLOAT( Autohack, 0.0f, pStatMod );
	const bool Autohack = ( WB_MODDED( Autohack ) != 0.0f );
	const bool Tourist = RosaDifficulty::GetGameDifficulty() == 0;
	if( Autohack || Tourist )
	{
		SucceedHack();
		return;
	}
}

void RosaHacking::EndHacking()
{
	if( !m_IsHacking )
	{
		return;
	}

	WBEventManager* const	pEventManager	= WBWorld::GetInstance()->GetEventManager();
	WBEntity* const			pPlayer			= RosaGame::GetPlayer();

	m_IsHacking = false;

	// Pop input context
	GetInputSystem()->PopContext( m_InputContext );

	// Tick input system once more to debounce e.g. frob and crouch inputs
	GetInputSystem()->Tick();

	// Enable frobbing again
	{
		WB_MAKE_EVENT( EnableFrob, NULL );
		WB_DISPATCH_EVENT( pEventManager, EnableFrob, pPlayer );
	}

	// Pop the hack UI screen
	{
		STATIC_HASHED_STRING( HackScreen );
		WB_MAKE_EVENT( RemoveUIScreen, NULL );
		WB_SET_AUTO( RemoveUIScreen, Hash, Screen, sHackScreen );
		WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), RemoveUIScreen, NULL );
	}

	// Show the reticle
	{
		STATIC_HASHED_STRING( HUD );
		STATIC_HASHED_STRING( Crosshair );
		WB_MAKE_EVENT( SetWidgetHidden, NULL );
		WB_SET_AUTO( SetWidgetHidden, Hash, Screen, sHUD );
		WB_SET_AUTO( SetWidgetHidden, Hash, Widget, sCrosshair );
		WB_SET_AUTO( SetWidgetHidden, Bool, Hidden, false );
		WB_DISPATCH_EVENT( pEventManager, SetWidgetHidden, NULL );
	}
}
