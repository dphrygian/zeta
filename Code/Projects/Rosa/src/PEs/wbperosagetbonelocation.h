#ifndef WBPEROSAGETBONELOCATION_H
#define WBPEROSAGETBONELOCATION_H

#include "wbpe.h"

class WBPERosaGetBoneLocation : public WBPE
{
public:
	WBPERosaGetBoneLocation();
	virtual ~WBPERosaGetBoneLocation();

	DEFINE_WBPE_FACTORY( RosaGetBoneLocation );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

private:
	WBPE*			m_EntityPE;
	HashedString	m_BoneName;
	bool			m_ProjectFromFG;	// If true, project location from foreground view into screen space, then back into worldspace with normal view
};

#endif // WBPEROSAGETBONELOCATION_H
