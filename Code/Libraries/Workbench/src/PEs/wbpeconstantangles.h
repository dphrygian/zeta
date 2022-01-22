#ifndef WBPECONSTANTANGLES_H
#define WBPECONSTANTANGLES_H

#include "wbpe.h"
#include "angles.h"

class WBPEConstantAngles : public WBPE
{
public:
	WBPEConstantAngles();
	virtual ~WBPEConstantAngles();

	DEFINE_WBPE_FACTORY( ConstantAngles );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Evaluate( const WBParamEvaluator::SPEContext& Context, WBParamEvaluator::SEvaluatedParam& EvaluatedParam ) const;

private:
	Angles	m_Value;
};

#endif // WBPECONSTANTANGLES_H
