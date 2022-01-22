#ifndef WBPEROSACAMPAIGNMODIFY_H
#define WBPEROSACAMPAIGNMODIFY_H

#include "PEs/wbpeunaryop.h"

class WBPERosaCampaignModify : public WBPEUnaryOp
{
public:
	WBPERosaCampaignModify();
	virtual ~WBPERosaCampaignModify();

	DEFINE_WBPE_FACTORY( RosaCampaignModify );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

protected:
	HashedString	m_Name;
	bool			m_Override;
	bool			m_Force;	// For hub, etc.
};

#endif // WBPEROSACAMPAIGNMODIFY_H
