#ifndef WBCOMPROSAMEDKIT_H
#define WBCOMPROSAMEDKIT_H

#include "wbrosacomponent.h"

class WBCompRosaMedkit : public WBRosaComponent
{
public:
	WBCompRosaMedkit();
	virtual ~WBCompRosaMedkit();

	DEFINE_WBCOMP( RosaMedkit, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );
	virtual void	AddContextToEvent( WBEvent& Event ) const;

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	uint			GetBandages() const		{ return m_Bandages; }
	uint			GetMaxBandages() const;
	bool			HasBandages() { return m_Bandages > 0; }
	void			AddBandages( const uint Bandages, const bool ShowLogMessage );
	void			TryUseBandage();

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			PublishToHUD() const;
	void			PushPersistence() const;
	void			PullPersistence();

	uint			m_Bandages;		// Serialized/persistent
	uint			m_MaxBandages;	// Config (can now be statmodded instead of being serialized/persistent)
};

#endif // WBCOMPROSAMEDKIT_H
