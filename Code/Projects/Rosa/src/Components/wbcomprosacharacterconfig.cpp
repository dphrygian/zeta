#include "core.h"
#include "wbcomprosacharacterconfig.h"
#include "Components/wbcomppemap.h"
#include "configmanager.h"
#include "idatastream.h"
#include "vector.h"
#include "vector4.h"
#include "hsv.h"
#include "mathfunc.h"
#include "mathcore.h"
#include "rosagame.h"
#include "rosacampaign.h"

WBCompRosaCharacterConfig::WBCompRosaCharacterConfig()
:	m_CharacterColors()
,	m_Costume()
{
}

WBCompRosaCharacterConfig::~WBCompRosaCharacterConfig()
{
}

/*virtual*/ void WBCompRosaCharacterConfig::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Wardrobe );
	const HashedString Wardrobe = ConfigManager::GetInheritedHash( sWardrobe, HashedString::NullString, sDefinitionName );
	if( Wardrobe )
	{
		STATICHASH( IsHuman );
		const bool IsHuman = ConfigManager::GetInheritedBool( sIsHuman, false, sDefinitionName );

		// We shouldn't ever care about gender here, because this is strictly randomized.
		m_Costume = GetGame()->GetWardrobe()->CreateCostume( Wardrobe, RosaWardrobe::EBG_None, IsHuman );
		InitializeFromCostume();
	}

	// We can either put a single section in the component definition directly (usually unnamed)...
	InitializeConfigSection( sDefinitionName );

	// ...or specify multiple subsections (usually named).
	STATICHASH( NumSections );
	const uint NumSections = ConfigManager::GetInheritedInt( sNumSections, 0, sDefinitionName );
	for( uint SectionIndex = 0; SectionIndex < NumSections; ++SectionIndex )
	{
		const HashedString SectionDefinitionName = ConfigManager::GetInheritedSequenceHash( "Section%d", SectionIndex, HashedString::NullString, sDefinitionName );
		InitializeConfigSection( SectionDefinitionName );
	}
}

void WBCompRosaCharacterConfig::PushCostumeToPEMap()
{
	WBCompPEMap* const			pPEMap		= WB_GETCOMP( GetEntity(), PEMap );
	if( !pPEMap )
	{
		return;
	}

	RosaGame* const				pGame		= GetGame();
	DEVASSERT( pGame );

	RosaWardrobe* const			pWardrobe	= pGame->GetWardrobe();
	DEVASSERT( pWardrobe );

	const RosaWardrobe::SBody&	Body		= pWardrobe->GetBody( m_Costume.m_Body );
	pPEMap->AddPEMap( m_Costume.m_IsHuman ? Body.m_HumanVoice : Body.m_VampireVoice );
}

void WBCompRosaCharacterConfig::InitializeFromCostume()
{
	RosaGame* const				pGame		= GetGame();
	DEVASSERT( pGame );

	RosaWardrobe* const			pWardrobe	= pGame->GetWardrobe();
	DEVASSERT( pWardrobe );

	STATIC_HASHED_STRING( Skin );
	const RosaWardrobe::SSkin&	Skin		= pWardrobe->GetSkin( m_Costume.m_Skin );
	const RosaWardrobe::SHair&	Hair		= pWardrobe->GetHair( m_Costume.m_Hair );
	const RosaWardrobe::SEyes&	Eyes		= pWardrobe->GetEyes( m_Costume.m_Eyes );
	InitializeCharacterColor( sSkin, m_Costume.m_IsHuman ? Skin.m_HumanColorHSV : Skin.m_VampireColorHSV, Hair.m_ColorHSV, Eyes.m_ColorHSV );

	STATIC_HASHED_STRING( Clothes );
	const RosaWardrobe::SScheme&	Scheme	= pWardrobe->GetScheme( m_Costume.m_Scheme );
	InitializeCharacterColor( sClothes, Scheme.m_PrimaryColorHSV, Scheme.m_SecondaryColorHSV, Scheme.m_AccentColorHSV );

	PushCostumeToPEMap();
}

void WBCompRosaCharacterConfig::InitializeConfigSection( const HashedString& DefinitionName )
{
	Vector PrimaryColorHSVMin;
	Vector PrimaryColorHSVMax;
	Vector SecondaryColorHSVMin;
	Vector SecondaryColorHSVMax;
	Vector AccentColorHSVMin;
	Vector AccentColorHSVMax;

	// These colors are defined in non-linear sRGB space, so they
	// correspond to the values that would be in textures.

	STATICHASH( Section );
	const HashedString Section = ConfigManager::GetInheritedHash( sSection, HashedString::NullString, DefinitionName );

	STATICHASH( PrimaryColorH );
	PrimaryColorHSVMin.x = ConfigManager::GetInheritedFloat( sPrimaryColorH, 0.0f, DefinitionName );

	STATICHASH( PrimaryColorHRange );
	PrimaryColorHSVMax.x = PrimaryColorHSVMin.x + ConfigManager::GetInheritedFloat( sPrimaryColorHRange, 0.0f, DefinitionName );

	STATICHASH( PrimaryColorS );
	PrimaryColorHSVMin.y = ConfigManager::GetInheritedFloat( sPrimaryColorS, 0.0f, DefinitionName );

	STATICHASH( PrimaryColorSRange );
	PrimaryColorHSVMax.y = PrimaryColorHSVMin.y + ConfigManager::GetInheritedFloat( sPrimaryColorSRange, 0.0f, DefinitionName );

	STATICHASH( PrimaryColorV );
	PrimaryColorHSVMin.z = ConfigManager::GetInheritedFloat( sPrimaryColorV, 0.0f, DefinitionName );

	STATICHASH( PrimaryColorVRange );
	PrimaryColorHSVMax.z = PrimaryColorHSVMin.z + ConfigManager::GetInheritedFloat( sPrimaryColorVRange, 0.0f, DefinitionName );

	STATICHASH( SecondaryColorH );
	SecondaryColorHSVMin.x = ConfigManager::GetInheritedFloat( sSecondaryColorH, 0.0f, DefinitionName );

	STATICHASH( SecondaryColorHRange );
	SecondaryColorHSVMax.x = SecondaryColorHSVMin.x + ConfigManager::GetInheritedFloat( sSecondaryColorHRange, 0.0f, DefinitionName );

	STATICHASH( SecondaryColorS );
	SecondaryColorHSVMin.y = ConfigManager::GetInheritedFloat( sSecondaryColorS, 0.0f, DefinitionName );

	STATICHASH( SecondaryColorSRange );
	SecondaryColorHSVMax.y = SecondaryColorHSVMin.y + ConfigManager::GetInheritedFloat( sSecondaryColorSRange, 0.0f, DefinitionName );

	STATICHASH( SecondaryColorV );
	SecondaryColorHSVMin.z = ConfigManager::GetInheritedFloat( sSecondaryColorV, 0.0f, DefinitionName );

	STATICHASH( SecondaryColorVRange );
	SecondaryColorHSVMax.z = SecondaryColorHSVMin.z + ConfigManager::GetInheritedFloat( sSecondaryColorVRange, 0.0f, DefinitionName );

	STATICHASH( AccentColorH );
	AccentColorHSVMin.x = ConfigManager::GetInheritedFloat( sAccentColorH, 0.0f, DefinitionName );

	STATICHASH( AccentColorHRange );
	AccentColorHSVMax.x = AccentColorHSVMin.x + ConfigManager::GetInheritedFloat( sAccentColorHRange, 0.0f, DefinitionName );

	STATICHASH( AccentColorS );
	AccentColorHSVMin.y = ConfigManager::GetInheritedFloat( sAccentColorS, 0.0f, DefinitionName );

	STATICHASH( AccentColorSRange );
	AccentColorHSVMax.y = AccentColorHSVMin.y + ConfigManager::GetInheritedFloat( sAccentColorSRange, 0.0f, DefinitionName );

	STATICHASH( AccentColorV );
	AccentColorHSVMin.z = ConfigManager::GetInheritedFloat( sAccentColorV, 0.0f, DefinitionName );

	STATICHASH( AccentColorVRange );
	AccentColorHSVMax.z = AccentColorHSVMin.z + ConfigManager::GetInheritedFloat( sAccentColorVRange, 0.0f, DefinitionName );

	InitializeCharacterColor(
		Section,
		Math::Random( PrimaryColorHSVMin,	PrimaryColorHSVMax ),
		Math::Random( SecondaryColorHSVMin,	SecondaryColorHSVMax ),
		Math::Random( AccentColorHSVMin,	AccentColorHSVMax ) );
}

void WBCompRosaCharacterConfig::InitializeCharacterColor( const HashedString& Section, const Vector4& PrimaryHSV, const Vector4& SecondaryHSV, const Vector4& AccentHSV )
{
	DEVASSERT( m_CharacterColors.Search( Section ).IsNull() );

	const Vector4	PrimaryRGB		= HSV::HSVToRGB_AlphaPass( PrimaryHSV );
	const Vector4	SecondaryRGB	= HSV::HSVToRGB_AlphaPass( SecondaryHSV );
	const Vector4	AccentRGB		= HSV::HSVToRGB_AlphaPass( AccentHSV );

	Matrix&			Colors			= m_CharacterColors.Insert( Section, Matrix( PrimaryRGB, SecondaryRGB, AccentRGB, Vector4() ) ).GetValue();

	// Convert to linear space now (so we don't have to do it for every pixel in the shader)
	for( uint Index = 0; Index < 16; ++Index )
	{
		Colors.v[ Index ] = SRGBToLinear( Colors.v[ Index ] );
	}
}

void WBCompRosaCharacterConfig::SetCharacterColor( const HashedString& Section, const uint Row, const Vector4& ColorRGB )
{
	const TConfigMap::Iterator ColorsIter = m_CharacterColors.Search( Section );
	if( ColorsIter.IsNull() )
	{
		return;
	}

	Matrix& Colors = ColorsIter.GetValue();
	for( uint Index = 0; Index < 4; ++Index )
	{
		Colors.m[ Row ][ Index ] = SRGBToLinear( ColorRGB.v[ Index ] );
	}
}

#define VERSION_EMPTY	0
#define VERSION_BASE	1
#define VERSION_CURRENT	1

uint WBCompRosaCharacterConfig::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;	// Version

	Size += 4;																			// m_CharacterColors.Size()
	Size += ( sizeof( HashedString ) + sizeof( Matrix ) ) * m_CharacterColors.Size();	// m_CharacterColors

	Size += GetGame()->GetWardrobe()->GetCostumeSerializationSize( m_Costume );

	return Size;
}

void WBCompRosaCharacterConfig::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteUInt32( m_CharacterColors.Size() );
	FOR_EACH_MAP( SectionIter, m_CharacterColors, HashedString, Matrix )
	{
		const HashedString&	Section	= SectionIter.GetKey();
		const Matrix&		Colors	= SectionIter.GetValue();

		Stream.WriteHashedString( Section );
		Stream.Write<Matrix>( Colors );
	}

	GetGame()->GetWardrobe()->SaveCostume( m_Costume, Stream );
}

void WBCompRosaCharacterConfig::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();
	Unused( Version );

	{
		const uint NumSections = Stream.ReadUInt32();
		for( uint SectionIndex = 0; SectionIndex < NumSections; ++SectionIndex )
		{
			const HashedString	Section	= Stream.ReadHashedString();
			const Matrix		Colors	= Stream.Read<Matrix>();
			m_CharacterColors.Insert( Section, Colors );
		}
	}

	{
		GetGame()->GetWardrobe()->LoadCostume( m_Costume, Stream );

		// Reinit PE map from costume. Colors will have been serialized and don't need to be reinitialized.
		PushCostumeToPEMap();
	}
}
