#ifndef WBCOMPROSAPATROLPATH_H
#define WBCOMPROSAPATROLPATH_H

#include "wbrosacomponent.h"
#include "array.h"
#include "wbeventmanager.h"
#include "vector.h"
#include "rosaworldgen.h"

class WBCompRosaPatrolPath : public WBRosaComponent
{
public:
	struct SPatrolNode
	{
		SPatrolNode()
		:	m_Location()
		,	m_Orientation()
		{
		}

		Vector	m_Location;
		Angles	m_Orientation;
	};

	WBCompRosaPatrolPath();
	virtual ~WBCompRosaPatrolPath();

	DEFINE_WBCOMP( RosaPatrolPath, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	void			SetPatrolNodes( const Array<WBEntityRef>& LinkedEntities );

	bool			HasPatrolNodes() { return !m_PatrolNodes.Empty(); }
	Vector			GetCurrentNodeLocation() const		{ return m_PatrolNodes[ m_CurrentNode ].m_Location; }
	Angles			GetCurrentNodeOrientation() const	{ return m_PatrolNodes[ m_CurrentNode ].m_Orientation; }

	void			SetFirstNode() { m_CurrentNode = 0; }
	void			SetNearestNode( const Vector& Location );
	void			SetNextNode();

private:
	Array<SPatrolNode>	m_PatrolNodes;
	uint				m_CurrentNode;
};

#endif // WBCOMPROSAPATROLPATH_H
