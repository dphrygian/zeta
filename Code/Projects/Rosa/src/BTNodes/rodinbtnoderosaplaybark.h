#ifndef RODINBTNODEROSAPLAYBARK_H
#define RODINBTNODEROSAPLAYBARK_H

#include "rodinbtnode.h"
#include "wbparamevaluator.h"

class RodinBTNodeRosaPlayBark : public RodinBTNode
{
public:
	RodinBTNodeRosaPlayBark();
	virtual ~RodinBTNodeRosaPlayBark();

	DEFINE_RODINBTNODE( RosaPlayBark, RodinBTNode );

	virtual void		InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual ETickStatus	Tick( const float DeltaTime );

private:
	SimpleString		m_SoundDef;		// Config
	WBParamEvaluator	m_SoundDefPE;	// Config
	HashedString		m_Category;		// Config
};

#endif // RODINBTNODEROSAPLAYBARK_H
