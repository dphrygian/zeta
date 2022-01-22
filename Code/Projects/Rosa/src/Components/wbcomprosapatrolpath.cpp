#include "core.h"
#include "wbcomprosapatrolpath.h"
#include "wbcomprosatransform.h"
#include "idatastream.h"
#include "mathcore.h"

WBCompRosaPatrolPath::WBCompRosaPatrolPath()
:	m_PatrolNodes()
,	m_CurrentNode( 0 )
{
}

WBCompRosaPatrolPath::~WBCompRosaPatrolPath()
{
}

/*virtual*/ void WBCompRosaPatrolPath::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( SetLinkedEntities );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sSetLinkedEntities )
	{
		STATIC_HASHED_STRING( LinkedEntities );
		void* pVoid = Event.GetPointer( sLinkedEntities );
		const Array<WBEntityRef>* const pLinkedEntities = static_cast<const Array<WBEntityRef>*>( pVoid );
		DEVASSERT( pLinkedEntities );

		SetPatrolNodes( *pLinkedEntities );
	}
}

void WBCompRosaPatrolPath::SetPatrolNodes( const Array<WBEntityRef>& LinkedEntities )
{
	ASSERT( m_PatrolNodes.Empty() );
	m_PatrolNodes.Reserve( LinkedEntities.Size() );
	FOR_EACH_ARRAY( LinkedEntityIter, LinkedEntities, WBEntityRef )
	{
		const WBEntityRef&	LinkedEntityRef	= LinkedEntityIter.GetValue();
		WBEntity* const		pLinkedEntity	= LinkedEntityRef.Get();
		DEVASSERT( pLinkedEntity );

		WBCompRosaTransform* const	pLinkedTransform	= pLinkedEntity->GetTransformComponent<WBCompRosaTransform>();
		if( !pLinkedTransform )
		{
			continue;
		}

		SPatrolNode& PatrolNode = m_PatrolNodes.PushBack();

		// TODO: Revisit this, it's old stuff from "loop metadata" era
		PatrolNode.m_Location		= pLinkedTransform->GetLocation();
		PatrolNode.m_Orientation	= pLinkedTransform->GetOrientation();
	}
}

void WBCompRosaPatrolPath::SetNearestNode( const Vector& Location )
{
	uint	NearestNodeIndex	= 0;
	float	NearestNodeDistSq	= FLT_MAX;

	for( uint NodeIndex = 0; NodeIndex < m_PatrolNodes.Size(); ++NodeIndex )
	{
		const SPatrolNode&	Node	= m_PatrolNodes[ NodeIndex ];
		const float			DistSq	= ( Node.m_Location - Location ).LengthSquared();

		if( DistSq < NearestNodeDistSq )
		{
			NearestNodeIndex	= NodeIndex;
			NearestNodeDistSq	= DistSq;
		}
	}

	m_CurrentNode = NearestNodeIndex;
}

void WBCompRosaPatrolPath::SetNextNode()
{
	if( m_PatrolNodes.Empty() )
	{
		// Do nothing; we'll just keep returning to the spawn point
	}
	else
	{
		m_CurrentNode = ( m_CurrentNode + 1 ) % m_PatrolNodes.Size();
	}
}

#define VERSION_EMPTY					0
#define VERSION_PATROLNODES				1
#define VERSION_PATROLNODE_ORIENTATION	2
#define VERSION_CURRENT					2

uint WBCompRosaPatrolPath::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;												// Version
	Size += 4;												// m_PatrolNodes.Size()
	Size += sizeof( SPatrolNode ) * m_PatrolNodes.Size();	// m_PatrolNodes
	Size += 4;												// m_CurrentNode

	return Size;
}

void WBCompRosaPatrolPath::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteUInt32( m_PatrolNodes.Size() );
	Stream.Write( sizeof( SPatrolNode ) * m_PatrolNodes.Size(), m_PatrolNodes.GetData() );

	Stream.WriteUInt32( m_CurrentNode );
}

void WBCompRosaPatrolPath::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_PATROLNODES )
	{
		ASSERT( m_PatrolNodes.Empty() );
		const uint NumPatrolNodes = Stream.ReadUInt32();
		m_PatrolNodes.Resize( NumPatrolNodes );

		if( Version < VERSION_PATROLNODE_ORIENTATION )
		{
			for( uint PatrolNodeIndex = 0; PatrolNodeIndex < NumPatrolNodes; ++PatrolNodeIndex )
			{
				Stream.Read( sizeof( Vector ), &m_PatrolNodes[ PatrolNodeIndex ].m_Location );
			}
		}
		else
		{
			Stream.Read( sizeof( SPatrolNode ) * m_PatrolNodes.Size(), m_PatrolNodes.GetData() );
		}

		m_CurrentNode = Stream.ReadUInt32();
	}
}
