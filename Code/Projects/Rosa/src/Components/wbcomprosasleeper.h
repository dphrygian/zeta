#ifndef WBCOMPROSASLEEPER_H
#define WBCOMPROSASLEEPER_H

#include "wbrosacomponent.h"
#include "vector.h"

class WBCompRosaSleeper : public WBRosaComponent
{
public:
	WBCompRosaSleeper();
	virtual ~WBCompRosaSleeper();

	DEFINE_WBCOMP( RosaSleeper, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void	Wake();
	void	HandleNoise( const Vector& NoiseLocation, const float NoiseRadius, const float NoiseCertaintyScalar );

	bool	m_IsAwake;			// Config/Serialized
	float	m_NoiseThreshold;	// Config
	bool	m_OnlyHearPlayer;	// Config
	bool	m_OnlyHearHostiles;	// Config
};

#endif // WBCOMPROSASLEEPER_H
