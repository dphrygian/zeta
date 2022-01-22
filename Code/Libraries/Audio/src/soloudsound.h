#ifndef SOLOUDSOUND_H
#define SOLOUDSOUND_H

#include "soundcommon.h"
#include "packstream.h"

#include "soloud.h"
#include "soloud_file.h"
#include "soloud_wav.h"
#include "soloud_wavstream.h"

class IDataStream;
class PackStream;
struct SSoundInit;

// Custom extension that allows reading into packed files
// Note that this will not work with in-memory package files
// (e.g., package files downloaded from a cloud service)...
// That's not a problem for now, but it might be worth revisiting
// this in the future to support that case. It should be possible,
// SoLoud doesn't actually use getFilePtr as far as I can see, so
// there's no reason this needs to use a FILE*. It could just be
// a wrapper on a PackStream* or something.
class PackageFileReader : public SoLoud::File
{
public:
	PackageFileReader( const char* const Filename, const uint Offset, const uint Length );
	~PackageFileReader();

	FILE*	m_File;
	uint	m_Offset;
	uint	m_Length;

	virtual int				eof()											{ return feof( m_File ) || pos() >= length(); }
	virtual unsigned int	read(unsigned char *aDst, unsigned int aBytes)	{ return static_cast<uint>( fread( aDst, 1, aBytes, m_File ) ); }	// SoLoud expects fread arguments in this order, not aBytes, 1
	virtual unsigned int	length()										{ return m_Length; }
	virtual void			seek(int aOffset)								{ fseek( m_File, m_Offset + aOffset, SEEK_SET ); }
	virtual unsigned int	pos()											{ return ftell( m_File ) - m_Offset; }
	virtual FILE*			getFilePtr()									{ return m_File; }
};

class SoLoudSound : public SoundCommon
{
public:
	SoLoudSound(
		IAudioSystem* const pSystem,
		const SSoundInit& SoundInit );
	virtual ~SoLoudSound();

	virtual ISoundInstance*		CreateInstance();

	virtual float				GetLength() const;

private:
	void			CreateSample( const IDataStream& Stream, const SSoundInit& SoundInit );
	void			CreateStream( const PackStream& Stream, const SSoundInit& SoundInit );

	SoLoud::AudioSource&	GetSoLoudAudioSource() const;

	SoLoud::Wav*		m_SoLoudWav;

	SoLoud::WavStream*	m_SoLoudWavStream;
	PackageFileReader*	m_PackageFileReader;

	//FMOD::Sound*		m_Sound;
	//FMOD::System*		m_FMODSystem;

	// It's maybe a bit hack to store the FMOD::System* instead of
	// an FMODAudioSystem*, but it's just easier to call directly to
	// it instead of routing everything through the ISoundSystem*,
	// and there's no way I'd be using an FMODSound with any other
	// sound system, so the OO implications are moot.
};

#endif // SOLOUDSOUND_H
