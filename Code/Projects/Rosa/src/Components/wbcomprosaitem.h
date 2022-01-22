#ifndef WBCOMPROSAITEM_H
#define WBCOMPROSAITEM_H

#include "wbrosacomponent.h"

class WBCompRosaItem : public WBRosaComponent
{
public:
	WBCompRosaItem();
	virtual ~WBCompRosaItem();

	DEFINE_WBCOMP( RosaItem, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	HashedString	GetSlot() const { return m_Slot; }
	void			SetSlot( const HashedString& NewSlot ) { m_Slot = NewSlot; }

	bool			IsPersistent() const { return m_Persistent; }
	bool			KeepHands() const { return m_KeepHands; }

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	HashedString	m_Slot;			// Config/serialized
	bool			m_Persistent;	// Config; does this item travel to other levels via persistence?
	bool			m_KeepHands;	// Config; when used as a modal item, should we keep or replace the usual hands
};

#endif // WBCOMPROSAITEM_H
