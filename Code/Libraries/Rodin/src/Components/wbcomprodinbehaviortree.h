#ifndef WBCOMPRODINBEHAVIORTREE_H
#define WBCOMPRODINBEHAVIORTREE_H

#include "wbcomponent.h"
#include "array.h"
#include "rodinbtnode.h"
#include "simplestring.h"

// This is the owner of the root node of the BT, as well as the task scheduler.

class WBCompRodinBehaviorTree : public WBComponent
{
public:
	WBCompRodinBehaviorTree();
	virtual ~WBCompRodinBehaviorTree();

	DEFINE_WBCOMP( RodinBehaviorTree, WBComponent );

	virtual void	Tick( const float DeltaTime );
	virtual int		GetTickOrder() { return ETO_TickFirst; }	// Tick before motion. Maybe should tick after sensors and thinkers?

	virtual void	HandleEvent( const WBEvent& Event );

	void			Start( RodinBTNode* pNode, RodinBTNode* pParentNode = NULL );
	void			Stop( RodinBTNode* pNode );
	void			Wake( RodinBTNode* pNode );
	void			Sleep( RodinBTNode* pNode );

	void			Flush();

	SimpleString	GetLookupNode( const HashedString& Key );

#if BUILD_DEV
	virtual void	Report() const;
	virtual bool	HasDebugRender() const { return true; }
	virtual void	DebugRender( const bool GroupedRender ) const;	// Implemented in Rosa, not this lib!
#endif

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			AddLookupNodeSet( const SimpleString& DefinitionName );

	struct SScheduledNode
	{
		SScheduledNode()
		:	m_Node( NULL )
		,	m_ParentNode( NULL )
		{
		}

		RodinBTNode*	m_Node;
		RodinBTNode*	m_ParentNode;
	};

	void					Finish( uint NodeIndex, RodinBTNode::ETickStatus FinishStatus );
	bool					FindScheduledNode( RodinBTNode* pNode, uint& OutIndex );

	// Mainly intending for debugging, only works for scheduled nodes
	RodinBTNode*			GetParentNode( RodinBTNode* const pNode ) const;

	RodinBTNode*			m_RootNode;				// Config/serialized
	Array<SScheduledNode>	m_ScheduledNodes;		// Serialized via m_AllNodes and RodinBTNode::m_BehaviorTreeIndex
	uint					m_TickIterateNodeIndex;	// Transient; this is a member so it can be modified when callbacks adjust the array order.

	bool					m_Paused;				// Serialized

	// This just maps to definition name, since looked-up nodes are still statically instantiated.
	Map<HashedString, SimpleString>	m_LookupNodeMap;	// Transient

#if BUILD_DEV
	bool					m_Verbose;				// Config
	Array<SimpleString>		m_BreakNodes;			// Config
#endif
};

#endif // WBCOMPRODINBEHAVIORTREE_H
