#ifndef WBCOMPROSALOCK_H
#define WBCOMPROSALOCK_H

#include "wbrosacomponent.h"
#include "vector.h"
#include "angles.h"
#include "simplestring.h"

class WBCompRosaLock : public WBRosaComponent
{
public:
	WBCompRosaLock();
	virtual ~WBCompRosaLock();

	DEFINE_WBCOMP( RosaLock, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	SimpleString	m_LockDef;

	// For lockpicking
	bool			m_UseCameraOverride;
	Vector			m_CameraTranslation;
	Angles			m_CameraOrientation;
};

#endif // WBCOMPROSALOCK_H
