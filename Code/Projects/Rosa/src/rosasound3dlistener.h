#ifndef ROSASOUND3DLISTENER_H
#define ROSASOUND3DLISTENER_H

#include "sound3dlistener.h"

class RosaWorld;

class RosaSound3DListener : public Sound3DListener
{
public:
	RosaSound3DListener();
	virtual ~RosaSound3DListener();

	virtual void	ModifyAttenuation( ISoundInstance* const pSoundInstance, float& Attenuation ) const;

	void			Initialize();
	void			SetWorld( RosaWorld* const pWorld ) { m_World = pWorld; }

private:
	RosaWorld*	m_World;
	float			m_VerticalScalar;
};

#endif // ROSASOUND3DLISTENER_H
