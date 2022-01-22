#ifndef WBCOMPROSATHINKER_H
#define WBCOMPROSATHINKER_H

#include "wbrosacomponent.h"

class WBCompRosaThinker : public WBRosaComponent
{
public:
	WBCompRosaThinker();
	virtual ~WBCompRosaThinker();

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

protected:
	bool	m_Paused;	// Serialized
};

#endif // WBCOMPROSATHINKER_H
