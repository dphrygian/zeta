#ifndef WBPEGETCONFIGVAR_H
#define WBPEGETCONFIGVAR_H

#include "wbpe.h"
#include "wbparamevaluator.h"

class WBPEGetConfigVar : public WBPE
{
public:
	WBPEGetConfigVar();
	virtual ~WBPEGetConfigVar();

	DEFINE_WBPE_FACTORY( GetConfigVar );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

private:
	HashedString				m_VarContext;
	mutable WBParamEvaluator	m_VarContextPE;
	HashedString				m_VarName;
	mutable WBParamEvaluator	m_VarNamePE;
	mutable WBParamEvaluator	m_DefaultPE;
};

#endif // WBPEGETVARIABLE_H
