#include "core.h"
#include "rodinbtnoderosachecktrace.h"
#include "configmanager.h"
#include "collisioninfo.h"
#include "rosaframework.h"
#include "rosaworld.h"

RodinBTNodeRosaCheckTrace::RodinBTNodeRosaCheckTrace()
:	m_StartPE()
,	m_EndPE()
,	m_ExtentsPE()
,	m_TargetPE()
{
}

RodinBTNodeRosaCheckTrace::~RodinBTNodeRosaCheckTrace()
{
}

void RodinBTNodeRosaCheckTrace::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	MAKEHASH( DefinitionName );

	STATICHASH( StartPE );
	m_StartPE.InitializeFromDefinition( ConfigManager::GetString( sStartPE, "", sDefinitionName ) );

	STATICHASH( EndPE );
	m_EndPE.InitializeFromDefinition( ConfigManager::GetString( sEndPE, "", sDefinitionName ) );

	STATICHASH( ExtentsPE );
	m_ExtentsPE.InitializeFromDefinition( ConfigManager::GetString( sExtentsPE, "", sDefinitionName ) );

	STATICHASH( TargetPE );
	m_TargetPE.InitializeFromDefinition( ConfigManager::GetString( sTargetPE, "", sDefinitionName ) );
}

RodinBTNode::ETickStatus RodinBTNodeRosaCheckTrace::Tick( const float DeltaTime )
{
	PROFILE_FUNCTION;

	Unused( DeltaTime );

	WBParamEvaluator::SPEContext Context;
	Context.m_Entity = GetEntity();

	m_StartPE.Evaluate( Context );
	m_EndPE.Evaluate( Context );
	m_ExtentsPE.Evaluate( Context );
	m_TargetPE.Evaluate( Context );

	const Vector	Start	= m_StartPE.GetVector();
	const Vector	End		= m_EndPE.GetVector();
	const Vector	Extents	= m_ExtentsPE.GetVector();
	WBEntity* const	pTarget	= m_TargetPE.GetEntity();

	CollisionInfo Info;
	Info.m_In_CollideWorld		= true;
	Info.m_In_CollideEntities	= true;
	Info.m_In_CollidingEntity	= GetEntity();
	Info.m_In_UserFlags			= EECF_Trace;

	RosaWorld* const pWorld		= RosaFramework::GetInstance()->GetWorld();
	if( pWorld->SweepCheck( Start, End, Extents, Info ) )
	{
		if( pTarget != NULL && pTarget == Info.m_Out_HitEntity )
		{
			// We hit our target, that's fine.
			return ETS_Success;
		}
		else
		{
			// We hit geometry or another entity
			return ETS_Fail;
		}
	}
	else
	{
		// We hit nothing.
		return ETS_Success;
	}
}
