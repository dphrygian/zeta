#ifndef WBCOMPLABEL_H
#define WBCOMPLABEL_H

#include "wbcomponent.h"
#include "array.h"

class WBCompLabel : public WBComponent
{
public:
	WBCompLabel();
	virtual ~WBCompLabel();

	DEFINE_WBCOMP( Label, WBComponent );

	virtual bool	BelongsInComponentArray() { return true; }
	virtual int		GetTickOrder() { return ETO_NoTick; }

	HashedString	GetLabel() { return m_Label; }

	static WBEntity*	GetEntityByLabel( const HashedString& Label );
	static void			GetEntitiesByLabel( const HashedString& Label, Array<WBEntity*>& OutEntities );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	HashedString	m_Label;	// Config
};

#endif // WBCOMPLABEL_H
