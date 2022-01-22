#ifndef WBCOMPROSASOUND_H
#define WBCOMPROSASOUND_H

#include "wbrosacomponent.h"
#include "array.h"
#include "simplestring.h"
#include "vector.h"

class ISoundInstance;
class BoneArray;

class WBCompRosaSound : public WBRosaComponent
{
public:
	WBCompRosaSound();
	virtual ~WBCompRosaSound();

	DEFINE_WBCOMP( RosaSound, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }
	virtual bool	BelongsInComponentArray() { return true; }	// For suppressing based on nearby sounds

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	bool			HasAttachBone() const { return m_AttachBone != HashedString::NullString; }
	void			SetAttachBoneIndex( const BoneArray* const pBones );

	static void		InstanceDeleteCallback( void* pVoid, ISoundInstance* pInstance );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			OnInstanceDeleted( ISoundInstance* const pInstance );

	struct SPlaySoundDefParams
	{
		SPlaySoundDefParams()
		:	m_SoundDef()
		,	m_Location()
		,	m_VolumeOverride( 0.0f )
		,	m_Attached( false )
		,	m_Muted( false )
		{
		}

		SimpleString	m_SoundDef;
		Vector			m_Location;
		float			m_VolumeOverride;
		bool			m_Attached;
		bool			m_Muted;
	};

	void			StopSound( const SimpleString& SoundDef );
	void			MuteSound( const SimpleString& SoundDef );
	void			UnmuteSound( const SimpleString& SoundDef );
	void			SetSoundVolume( const SimpleString& SoundDef, const float Volume );
	void			StopCategory( const HashedString& Category );
	ISoundInstance*	PlaySoundDef( SPlaySoundDefParams& Params, const uint StartPosition = 0 );

	bool			ShouldSuppress( const SPlaySoundDefParams& Params );

	struct SSoundInstance
	{
		SSoundInstance()
		:	m_SoundInstance( NULL )
		,	m_Attached( false )
		,	m_Muted( false )
		,	m_BaseVolume( 0.0f )
		,	m_SoundDef()
		,	m_SuppressGroup()
		{
		}

		ISoundInstance*	m_SoundInstance;
		bool			m_Attached;
		bool			m_Muted;
		float			m_BaseVolume;
		SimpleString	m_SoundDef;
		HashedString	m_SuppressGroup;
	};

	SimpleString			m_DefaultSound;			// Config
	Array<SSoundInstance>	m_SoundInstances;
	bool					m_SendDeletedEvents;	// Config, mainly for notifying jukebox
	HashedString			m_AttachBone;			// Config
	int						m_AttachBoneIndex;		// Transient
};

#endif // WBCOMPROSASOUND_H
