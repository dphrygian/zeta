#ifndef WBCOMPROSAKEYRING_H
#define WBCOMPROSAKEYRING_H

#include "wbrosacomponent.h"
#include "wbeventmanager.h"
#include "set.h"
#include "hashedstring.h"
#include "array.h"

class WBCompRosaKeyRing : public WBRosaComponent
{
public:
	WBCompRosaKeyRing();
	virtual ~WBCompRosaKeyRing();

	DEFINE_WBCOMP( RosaKeyRing, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );
	virtual void	AddContextToEvent( WBEvent& Event ) const;

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	uint			GetKeys() const { return m_Keys; }
	bool			HasKeys() { return m_Keys > 0; }
	void			AddKeys( const uint Keys, const bool ShowLogMessage );
	void			RemoveKey();

	bool			HasKeycard( const Array<HashedString>& Keycards, HashedString* const pOutUsedKeycard = NULL );
	bool			HasKeycard( const HashedString& Keycard );
	void			AddKeycard( const HashedString& Keycard, const bool ShowLogMessage );
	void			RemoveKeycard( const HashedString& Keycard, const bool ShowLogMessage );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			PublishToHUD() const;
	void			PushPersistence() const;
	void			PullPersistence();

	uint				m_Keys;
	Set<HashedString>	m_Keycards;	// Config/serialized/persistent

	bool				m_ShowKeycardUsedLog;	// Config
};

#endif // WBCOMPROSAKEYRING_H
