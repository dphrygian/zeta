#ifndef WBCOMPROSAUPGRADES_H
#define WBCOMPROSAUPGRADES_H

#include "wbrosacomponent.h"
#include "map.h"
#include "hashedstring.h"
#include "simplestring.h"

enum EUpgradeStatus
{
	EUS_None,
	EUS_Locked,
	EUS_Unlocked,
	EUS_Purchased,
};

struct SUpgrade
{
	SUpgrade()
	:	m_Hash()
	,	m_Name()
	,	m_Icon()
	,	m_Status( EUS_None )
	,	m_StatModEvent()
	,	m_Prereq()
	,	m_PostEvent()
	,	m_Track()
	,	m_Cost( 0 )
	{
	}

	HashedString	m_Hash;
	SimpleString	m_Name;
	SimpleString	m_Icon;
	EUpgradeStatus	m_Status;
	HashedString	m_StatModEvent;
	HashedString	m_Prereq;
	HashedString	m_PostEvent;
	HashedString	m_Track;
	uint			m_Cost;
};

typedef Map<HashedString, SUpgrade> TUpgradeMap;

class WBCompRosaUpgrades : public WBRosaComponent
{
public:
	WBCompRosaUpgrades();
	virtual ~WBCompRosaUpgrades();

	DEFINE_WBCOMP( RosaUpgrades, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	const TUpgradeMap&	GetUpgrades() const { return m_Upgrades; }
	const SUpgrade&		GetUpgrade( const HashedString& Upgrade ) const { return m_Upgrades[ Upgrade ]; }
	const SUpgrade*		SafeGetUpgrade( const HashedString& Upgrade ) const;

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			TriggerStatMods() const;
	void			TryPurchase( const HashedString& UpgradeTag );
	void			Give( const HashedString& UpgradeTag );
	void			Give( SUpgrade& Upgrade );

	void			PushPersistence() const;
	void			PullPersistence();

	TUpgradeMap		m_Upgrades;	// Serialized
};

#endif // WBCOMPROSAUPGRADES_H
