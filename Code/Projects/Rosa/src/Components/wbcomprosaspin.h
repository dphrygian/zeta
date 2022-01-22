#ifndef WBCOMPROSASPIN_H
#define WBCOMPROSASPIN_H

#include "wbrosacomponent.h"
#include "vector.h"

class WBCompRosaSpin : public WBRosaComponent
{
public:
	WBCompRosaSpin();
	virtual ~WBCompRosaSpin();

	DEFINE_WBCOMP( RosaSpin, WBRosaComponent );

	virtual void	Tick( const float DeltaTime );
	virtual int		GetTickOrder() { return ETO_TickFirst; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	Vector	m_Axis;				// Config
	float	m_Velocity;			// Config (in degrees)/serialized
	float	m_DampingScalar;	// Config
};

#endif // WBCOMPROSASPIN_H
