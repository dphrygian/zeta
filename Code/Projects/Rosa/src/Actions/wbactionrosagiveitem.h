#ifndef WBACTIONROSAGIVEITEM_H
#define WBACTIONROSAGIVEITEM_H

#include "wbaction.h"
#include "simplestring.h"
#include "wbparamevaluator.h"

class WBActionRosaGiveItem : public WBAction
{
public:
	WBActionRosaGiveItem();
	virtual ~WBActionRosaGiveItem();

	DEFINE_WBACTION_FACTORY( RosaGiveItem );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Execute();

private:
	void			GiveItemTo( const SimpleString& ItemDef,  WBEntity* const pEntity ) const;

	SimpleString		m_ItemDef;
	WBParamEvaluator	m_ItemDefPE;
	WBParamEvaluator	m_GiveToPE;
};

#endif // WBACTIONROSAGIVEITEM_H
