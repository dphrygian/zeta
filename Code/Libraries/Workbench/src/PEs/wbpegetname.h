#ifndef WBPEGETNAME_H
#define WBPEGETNAME_H

#include "wbpeunaryop.h"

class WBPEGetName : public WBPEUnaryOp
{
public:
	WBPEGetName();
	virtual ~WBPEGetName();

	DEFINE_WBPE_FACTORY( GetName );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

private:
	bool	m_UniqueName;
};

#endif // WBPEGETNAME_H
