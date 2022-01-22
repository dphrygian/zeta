#ifndef WBCOMPROSAHEADSHOT_H
#define WBCOMPROSAHEADSHOT_H

#include "wbrosacomponent.h"
#include "map.h"

class WBCompRosaHeadshot : public WBRosaComponent
{
public:
	WBCompRosaHeadshot();
	virtual ~WBCompRosaHeadshot();

	DEFINE_WBCOMP( RosaHeadshot, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	float			GetHeadshotMod( const HashedString& BoneName ) const;

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	typedef Map<HashedString, float> THeadshotMap;
	THeadshotMap	m_Headshots;	// Config, map bone name to mod
};

#endif // WBCOMPROSAHEADSHOT_H
