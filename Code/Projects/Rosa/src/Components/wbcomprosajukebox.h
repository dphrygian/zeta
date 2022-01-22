#ifndef WBCOMPROSAJUKEBOX_H
#define WBCOMPROSAJUKEBOX_H

#include "wbrosacomponent.h"
#include "array.h"

class WBCompRosaJukebox : public WBRosaComponent
{
public:
	WBCompRosaJukebox();
	virtual ~WBCompRosaJukebox();

	DEFINE_WBCOMP( RosaJukebox, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			Play();
	void			Next();
	void			Mute();
	void			Unmute();

	void			HandleDeletedSound( const SimpleString& SoundDef );

	Array<SimpleString>	m_TrackSoundDefs;	// Config
	bool				m_DeferPlay;		// Config
	bool				m_Loop;				// Config
	uint				m_TrackIndex;		// Serialized
	bool				m_Muted;			// Serialized
};

#endif // WBCOMPROSAJUKEBOX_H
