#ifndef WBACTIONROSACONDITIONALSHOWBOOK_H
#define WBACTIONROSACONDITIONALSHOWBOOK_H

#include "wbactionrosashowbook.h"
#include "simplestring.h"

class WBActionRosaConditionalShowBook : public WBActionRosaShowBook
{
public:
	WBActionRosaConditionalShowBook();
	virtual ~WBActionRosaConditionalShowBook();

	DEFINE_WBACTION_FACTORY( RosaConditionalShowBook );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	HashedString	m_PersistenceKey;
};

#endif // WBACTIONROSACONDITIONALSHOWBOOK_H
