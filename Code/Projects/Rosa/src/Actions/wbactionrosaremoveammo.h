#ifndef WBACTIONROSAREMOVEAMMO_H
#define WBACTIONROSAREMOVEAMMO_H

#include "wbaction.h"
#include "hashedstring.h"
#include "wbparamevaluator.h"

class WBActionRosaRemoveAmmo : public WBAction
{
public:
	WBActionRosaRemoveAmmo();
	virtual ~WBActionRosaRemoveAmmo();

	DEFINE_WBACTION_FACTORY( RosaRemoveAmmo );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	virtual void	Execute();

private:
	// If true, ignore Type and remove ammo from the item in hands instead of the ammo bag
	bool				m_Spend;

	HashedString		m_Type;
	WBParamEvaluator	m_TypePE;
	uint				m_Count;
	WBParamEvaluator	m_CountPE;
};

#endif // WBACTIONROSAREMOVEAMMO_H
