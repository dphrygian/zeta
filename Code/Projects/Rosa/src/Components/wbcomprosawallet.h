#ifndef WBCOMPROSAWALLET_H
#define WBCOMPROSAWALLET_H

#include "wbrosacomponent.h"
#include "wbeventmanager.h"

class WBCompRosaWallet : public WBRosaComponent
{
public:
	WBCompRosaWallet();
	virtual ~WBCompRosaWallet();

	DEFINE_WBCOMP( RosaWallet, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );
	virtual void	AddContextToEvent( WBEvent& Event ) const;

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	uint			GetLimit() const { return m_Limit; }
	uint			GetMoney() const { return m_Money; }
	bool			HasMoney( const uint Money ) { return m_Money >= Money; }
	void			AddMoney( const uint Money, const bool ShowLogMessage );
	void			RemoveMoney( const uint Money, const bool ShowLogMessage );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			PublishToHUD() const;
	void			PushPersistence() const;
	void			PullPersistence();

	uint	m_Money;
	uint	m_Limit;
};

#endif // WBCOMPROSAWALLET_H
