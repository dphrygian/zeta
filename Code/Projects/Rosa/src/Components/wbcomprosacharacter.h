#ifndef WBCOMPROSACHARACTER_H
#define WBCOMPROSACHARACTER_H

#include "wbrosacomponent.h"
#include "array.h"
#include "simplestring.h"
#include "vector.h"
#include "vector4.h"

// This manages player customization; AI customization
// and rendering is in RosaCharacterConfig. Urk.

class WBCompRosaCharacter : public WBRosaComponent
{
public:
	WBCompRosaCharacter();
	virtual ~WBCompRosaCharacter();

	DEFINE_WBCOMP( RosaCharacter, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	// ZETATODO: Reimplement this sort of thing if I let the player change gender
	//SimpleString	GetCurrentVoiceSet() const { return GetCurrentHeadOption().m_VoiceSet; }

	const Array<Vector>&	GetSkinPresets() const	{ return m_SkinPresets; }
	const Array<Vector4>&	GetNailsPresets() const	{ return m_NailsPresets; }
	Vector					GetCurrentSkin() const	{ return m_CurrentSkin; }
	Vector4					GetCurrentNails() const	{ return m_CurrentNails; }

	void					SetCurrentSkin( const Vector&	SkinHSV )	{ m_CurrentSkin		= SkinHSV; }
	void					SetCurrentNails( const Vector4&	NailsHSVA )	{ m_CurrentNails	= NailsHSVA; }
	void					PushSkinAndNailsToConfig() const;
	void					PushSkinAndNailsToUI() const;	// Implicitly calls PushSkinAndNailsToConfig by updating sliders

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	struct SHeadOption
	{
		SimpleString	m_VoiceSet;
	};

	struct SBodyOption
	{
		SimpleString	m_HandsMesh;
	};

	void				SelectCharacterPreset( const int SkinPresetIndex, const int NailsPresetIndex );
	void				PushCharacterOptions() const;

	void				PushPersistence() const;
	void				PullPersistence();

	Array<SHeadOption>	m_HeadOptions;
	Array<SBodyOption>	m_BodyOptions;
	Array<Vector>		m_SkinPresets;	// Colors as HSV
	Array<Vector4>		m_NailsPresets;	// Colors as HSVA

	Vector				m_CurrentSkin;
	Vector4				m_CurrentNails;
};

#endif // WBCOMPROSACHARACTER_H
