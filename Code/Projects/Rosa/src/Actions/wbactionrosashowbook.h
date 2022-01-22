#ifndef WBACTIONROSASHOWBOOK_H
#define WBACTIONROSASHOWBOOK_H

#include "wbaction.h"
#include "simplestring.h"
#include "wbparamevaluator.h"

class WBActionRosaShowBook : public WBAction
{
public:
	WBActionRosaShowBook();
	virtual ~WBActionRosaShowBook();

	DEFINE_WBACTION_FACTORY( RosaShowBook );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

	// Static function so it can be reused by other systems (see WBCompRosaReadable)
	static void		ShowBookScreen( const SimpleString& BookString, const bool IsDynamic, const HashedString BookScreenOverride );

private:
	SimpleString		m_BookString;
	WBParamEvaluator	m_BookStringPE;
	bool				m_IsDynamic;
	HashedString		m_BookScreen;
};

#endif // WBACTIONROSASHOWBOOK_H
