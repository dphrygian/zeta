#ifndef WBCOMPROSACHARACTERCONFIG_H
#define WBCOMPROSACHARACTERCONFIG_H

#include "wbrosacomponent.h"
#include "matrix.h"
#include "map.h"
#include "hashedstring.h"
#include "rosawardrobe.h"
#include "rosacampaign.h"

class Vector4;

class WBCompRosaCharacterConfig : public WBRosaComponent
{
public:
	WBCompRosaCharacterConfig();
	virtual ~WBCompRosaCharacterConfig();

	DEFINE_WBCOMP( RosaCharacterConfig, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	const Matrix&	GetCharacterColors( const HashedString& Section ) const { DEBUGASSERT( m_CharacterColors.Search( Section ).IsValid() ); return m_CharacterColors[ Section ]; }

	void			SetCharacterColor( const HashedString& Section, const uint Row, const Vector4& ColorRGB );

	bool							HasCostume() const	{ return m_Costume.m_IsValid; }
	const RosaWardrobe::SCostume&	GetCostume() const	{ return m_Costume; }

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			InitializeConfigSection( const HashedString& DefinitionName );
	void			InitializeFromCostume();
	void			PushCostumeToPEMap();

	void			InitializeCharacterColor( const HashedString& Section, const Vector4& PrimaryHSV, const Vector4& SecondaryHSV, const Vector4& AccentHSV );

	typedef Map<HashedString, Matrix> TConfigMap;
	TConfigMap	m_CharacterColors;		// Serialized; defined as HSV and automatically prepared in linear RGB space

	RosaWardrobe::SCostume	m_Costume;	// Config/serialized
};

#endif // WBCOMPROSACHARACTERCONFIG_H
