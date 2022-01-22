#ifndef WBPERANDOMVECTOR_H
#define WBPERANDOMVECTOR_H

#include "wbpe.h"

class WBPERandomVector : public WBPE
{
public:
	WBPERandomVector();
	virtual ~WBPERandomVector();

	DEFINE_WBPE_FACTORY( RandomVector );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

private:
	Vector	m_ValueA;
	Vector	m_ValueB;
};

#endif // WBPERANDOMVECTOR_H
