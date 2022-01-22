#ifndef WBCOMPROSASENSOR_H
#define WBCOMPROSASENSOR_H

#include "wbrosacomponent.h"

class WBCompRosaSensor : public WBRosaComponent
{
public:
	WBCompRosaSensor();
	virtual ~WBCompRosaSensor();

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

protected:
	bool	m_Paused;	// Serialized
};

#endif // WBCOMPROSASENSOR_H
