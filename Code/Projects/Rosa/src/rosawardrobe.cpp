#include "core.h"
#include "rosawardrobe.h"
#include "mathfunc.h"
#include "configmanager.h"
#include "idatastream.h"
#include "hsv.h"

RosaWardrobe::RosaWardrobe()
:	m_Wardrobes()
,	m_Bodies()
,	m_Skins()
,	m_Hairs()
,	m_Eyes()
,	m_Heads()
,	m_Outfits()
,	m_Styles()
,	m_Schemes()
,	m_Pieces()
,	m_Options()
,	m_Accessories()
{
}

RosaWardrobe::~RosaWardrobe()
{
}

RosaWardrobe::SCostume RosaWardrobe::CreateCostume( const HashedString& WardrobeTag, const EBodyGender BodyGender, const bool IsHuman )
{
	SCostume Costume;

	Costume.m_IsValid				= true;
	Costume.m_IsHuman				= IsHuman;

	const SWardrobe&	Wardrobe	= GetWardrobe( WardrobeTag );
	Costume.m_Wardrobe				= WardrobeTag;

	if( BodyGender == EBG_Woman && !Wardrobe.m_FemaleBodies.Empty() )
	{
		Costume.m_Body				= Math::ArrayRandom( Wardrobe.m_FemaleBodies );
	}
	else if( BodyGender == EBG_Man && !Wardrobe.m_MaleBodies.Empty() )
	{
		Costume.m_Body				= Math::ArrayRandom( Wardrobe.m_MaleBodies );
	}
	else
	{
		Costume.m_Body				= Math::ArrayRandom( Wardrobe.m_Bodies );
	}

	const SBody&		Body		= GetBody( Costume.m_Body );
	Costume.m_Scale					= Math::Random( Body.m_ScaleMin, Body.m_ScaleMax );
	Costume.m_Skin					= Math::ArrayRandom( Body.m_Skins );
	Costume.m_Head					= Body.m_Heads.Empty()			? HashedString::NullString : Math::ArrayRandom( Body.m_Heads );

	const SSkin&		Skin		= GetSkin( Costume.m_Skin );
	Costume.m_Hair					= Skin.m_Hairs.Empty()			? HashedString::NullString : Math::ArrayRandom( Skin.m_Hairs );
	Costume.m_Eyes					= Skin.m_Eyes.Empty()			? HashedString::NullString : Math::ArrayRandom( Skin.m_Eyes );

	if( Body.m_Outfits.Empty() )
	{
		// Skip all of that stuff
	}
	else
	{
		const HashedString&	OutfitTag	= Math::ArrayRandom( Body.m_Outfits );

		const SOutfit&		Outfit			= GetOutfit( OutfitTag );
		Costume.m_Style						= Outfit.m_Styles.Empty()		? HashedString::NullString : Math::ArrayRandom( Outfit.m_Styles );
		Costume.m_Accessory					= Outfit.m_Accessories.Empty()	? HashedString::NullString : Math::ArrayRandom( Outfit.m_Accessories );
		Costume.m_Scheme					= Outfit.m_Schemes.Empty()		? HashedString::NullString : Math::ArrayRandom( Outfit.m_Schemes );

		// HACKHACK: Disable the accessory if hairstyle disallows it (because hats)
		// ROSATODO: For Zeta, revisit this so that we can instead hide hair when a helmet is equipped?
		const bool			AllowAccessory	= ConfigManager::GetInheritedBool( Costume.m_Accessory, true, Costume.m_Style );
		Costume.m_Accessory					= AllowAccessory ? Costume.m_Accessory : HashedString::NullString;

		FOR_EACH_ARRAY( PieceIter, Outfit.m_Pieces, HashedString )
		{
			const HashedString&	PieceTag	= PieceIter.GetValue();
			const SPiece&		Piece		= GetPiece( PieceTag );
			const HashedString&	OptionTag	= Math::ArrayRandom( Piece.m_Options );
			Costume.m_Options.PushBack( OptionTag );
		}
	}

	return Costume;
}

const RosaWardrobe::SWardrobe& RosaWardrobe::GetWardrobe( const HashedString& Tag )
{
	Map<HashedString, SWardrobe>::Iterator Iter = m_Wardrobes.Search( Tag );
	if( Iter.IsValid() )
	{
		return Iter.GetValue();
	}

	SWardrobe& Wardrobe = m_Wardrobes[ Tag ];

	STATICHASH( NumBodies );
	const uint NumBodies = ConfigManager::GetInheritedInt( sNumBodies, 0, Tag );
	for( uint Index = 0; Index < NumBodies; ++Index )
	{
		Wardrobe.m_Bodies.PushBack( ConfigManager::GetInheritedSequenceHash( "Body%d", Index, HashedString::NullString, Tag ) );
	}

	STATICHASH( NumMaleBodies );
	const uint NumMaleBodies = ConfigManager::GetInheritedInt( sNumMaleBodies, 0, Tag );
	for( uint Index = 0; Index < NumMaleBodies; ++Index )
	{
		const HashedString Body = ConfigManager::GetInheritedSequenceHash( "MaleBody%d", Index, HashedString::NullString, Tag );
		Wardrobe.m_Bodies.PushBack( Body );
		Wardrobe.m_MaleBodies.PushBack( Body );
	}

	STATICHASH( NumFemaleBodies );
	const uint NumFemaleBodies = ConfigManager::GetInheritedInt( sNumFemaleBodies, 0, Tag );
	for( uint Index = 0; Index < NumFemaleBodies; ++Index )
	{
		const HashedString Body = ConfigManager::GetInheritedSequenceHash( "FemaleBody%d", Index, HashedString::NullString, Tag );
		Wardrobe.m_Bodies.PushBack( Body );
		Wardrobe.m_FemaleBodies.PushBack( Body );
	}

	STATICHASH( CastsShadows );
	Wardrobe.m_CastsShadows = ConfigManager::GetInheritedBool( sCastsShadows, true, Tag );

	STATICHASH( CastsSelfShadows );
	Wardrobe.m_CastsSelfShadows = ConfigManager::GetInheritedBool( sCastsSelfShadows, true, Tag );

	return Wardrobe;
}

RosaWardrobe::EBodyGender RosaWardrobe::GetBodyGender( const HashedString& BodyGender ) const
{
	STATIC_HASHED_STRING( Woman );
	if( BodyGender == sWoman ) { return EBG_Woman; }

	STATIC_HASHED_STRING( Man );
	if( BodyGender == sMan ) { return EBG_Man; }

	// This case is not invalid; some enemy types may not be
	// gendered (whereas currently all actors are gendered).
	return EBG_None;
}

const RosaWardrobe::SBody& RosaWardrobe::GetBody( const HashedString& Tag )
{
	Map<HashedString, SBody>::Iterator BodyIter = m_Bodies.Search( Tag );
	if( BodyIter.IsValid() )
	{
		return BodyIter.GetValue();
	}

	SBody& Body = m_Bodies[ Tag ];

	STATICHASH( Mesh );
	Body.m_Mesh = ConfigManager::GetInheritedString( sMesh, "", Tag );

	STATICHASH( Gender );
	Body.m_Gender = GetBodyGender( ConfigManager::GetInheritedHash( sGender, HashedString::NullString, Tag ) );

	STATICHASH( Voice );
	const SimpleString DefaultVoice = ConfigManager::GetInheritedString( sVoice, "", Tag );

	STATICHASH( HumanVoice );
	Body.m_HumanVoice = ConfigManager::GetInheritedString( sHumanVoice, DefaultVoice.CStr(), Tag );

	STATICHASH( VampireVoice );
	Body.m_VampireVoice = ConfigManager::GetInheritedString( sVampireVoice, DefaultVoice.CStr(), Tag );

	STATICHASH( Scale );
	const float Scale = ConfigManager::GetInheritedFloat( sScale, 1.0f, Tag );

	STATICHASH( ScaleMin );
	Body.m_ScaleMin = ConfigManager::GetInheritedFloat( sScaleMin, Scale, Tag );

	STATICHASH( ScaleMax );
	Body.m_ScaleMax = ConfigManager::GetInheritedFloat( sScaleMax, Scale, Tag );

	DEVASSERT( Body.m_ScaleMin <= Body.m_ScaleMax );

	STATICHASH( NumSkins );
	const uint NumSkins = ConfigManager::GetInheritedInt( sNumSkins, 0, Tag );
	for( uint Index = 0; Index < NumSkins; ++Index )
	{
		Body.m_Skins.PushBack( ConfigManager::GetInheritedSequenceHash( "Skin%d", Index, HashedString::NullString, Tag ) );
	}

	STATICHASH( NumHeads );
	const uint NumHeads = ConfigManager::GetInheritedInt( sNumHeads, 0, Tag );
	for( uint Index = 0; Index < NumHeads; ++Index )
	{
		Body.m_Heads.PushBack( ConfigManager::GetInheritedSequenceHash( "Head%d", Index, HashedString::NullString, Tag ) );
	}

	// ZETATODO: Clean up the stuff I no longer want here, make it easier to build creature variations.

	STATICHASH( NumOutfits );
	const uint NumOutfits = ConfigManager::GetInheritedInt( sNumOutfits, 0, Tag );
	for( uint Index = 0; Index < NumOutfits; ++Index )
	{
		Body.m_Outfits.PushBack( ConfigManager::GetInheritedSequenceHash( "Outfit%d", Index, HashedString::NullString, Tag ) );
	}

	return Body;
}

const RosaWardrobe::SSkin& RosaWardrobe::GetSkin( const HashedString& Tag )
{
	Map<HashedString, SSkin>::Iterator SkinIter = m_Skins.Search( Tag );
	if( SkinIter.IsValid() )
	{
		return SkinIter.GetValue();
	}

	SSkin& Skin = m_Skins[ Tag ];

	const Vector DefaultColorHSV = HSV::GetConfigHSV( "Color", Tag, Vector() );

	// ROSANOTE: Human alpha defaults to 0, which is the skin bit (see character shader)
	Skin.m_HumanColorHSV = HSV::GetConfigHSVA( "HumanColor", Tag, Vector4( DefaultColorHSV, 0.0f ) );
	// ROSANOTE: Vampire alpha defaults to 1, which is the no-skin bit (see character shader)
	Skin.m_VampireColorHSV = HSV::GetConfigHSVA( "VampireColor", Tag, Vector4( DefaultColorHSV, 1.0f ) );

	STATICHASH( NumHairs );
	const uint NumHairs = ConfigManager::GetInheritedInt( sNumHairs, 0, Tag );
	for( uint Index = 0; Index < NumHairs; ++Index )
	{
		Skin.m_Hairs.PushBack( ConfigManager::GetInheritedSequenceHash( "Hair%d", Index, HashedString::NullString, Tag ) );
	}

	STATICHASH( NumEyes );
	const uint NumEyes = ConfigManager::GetInheritedInt( sNumEyes, 0, Tag );
	for( uint Index = 0; Index < NumEyes; ++Index )
	{
		Skin.m_Eyes.PushBack( ConfigManager::GetInheritedSequenceHash( "Eyes%d", Index, HashedString::NullString, Tag ) );
	}

	return Skin;
}

const RosaWardrobe::SHair& RosaWardrobe::GetHair( const HashedString& Tag )
{
	Map<HashedString, SHair>::Iterator HairIter = m_Hairs.Search( Tag );
	if( HairIter.IsValid() )
	{
		return HairIter.GetValue();
	}

	SHair& Hair = m_Hairs[ Tag ];

	Hair.m_ColorHSV = HSV::GetConfigHSV( "Color", Tag, Vector() );

	return Hair;
}

const RosaWardrobe::SEyes& RosaWardrobe::GetEyes( const HashedString& Tag )
{
	Map<HashedString, SEyes>::Iterator EyesIter = m_Eyes.Search( Tag );
	if( EyesIter.IsValid() )
	{
		return EyesIter.GetValue();
	}

	SEyes& Eyes = m_Eyes[ Tag ];

	Eyes.m_ColorHSV = HSV::GetConfigHSV( "Color", Tag, Vector() );

	return Eyes;
}

const RosaWardrobe::SHead& RosaWardrobe::GetHead( const HashedString& Tag )
{
	Map<HashedString, SHead>::Iterator HeadIter = m_Heads.Search( Tag );
	if( HeadIter.IsValid() )
	{
		return HeadIter.GetValue();
	}

	SHead& Head = m_Heads[ Tag ];

	STATICHASH( Mesh );
	const SimpleString DefaultMesh = ConfigManager::GetInheritedString( sMesh, "", Tag );

	STATICHASH( HumanMesh );
	Head.m_HumanMesh = ConfigManager::GetInheritedString( sHumanMesh, DefaultMesh.CStr(), Tag );

	STATICHASH( VampireMesh );
	Head.m_VampireMesh = ConfigManager::GetInheritedString( sVampireMesh, DefaultMesh.CStr(), Tag );

	return Head;
}

const RosaWardrobe::SOutfit& RosaWardrobe::GetOutfit( const HashedString& Tag )
{
	Map<HashedString, SOutfit>::Iterator Iter = m_Outfits.Search( Tag );
	if( Iter.IsValid() )
	{
		return Iter.GetValue();
	}

	SOutfit& Outfit = m_Outfits[ Tag ];

	STATICHASH( NumStyles );
	const uint NumStyles = ConfigManager::GetInheritedInt( sNumStyles, 0, Tag );
	for( uint Index = 0; Index < NumStyles; ++Index )
	{
		Outfit.m_Styles.PushBack( ConfigManager::GetInheritedSequenceHash( "Style%d", Index, HashedString::NullString, Tag ) );
	}

	STATICHASH( NumSchemes );
	const uint NumSchemes = ConfigManager::GetInheritedInt( sNumSchemes, 0, Tag );
	for( uint Index = 0; Index < NumSchemes; ++Index )
	{
		Outfit.m_Schemes.PushBack( ConfigManager::GetInheritedSequenceHash( "Scheme%d", Index, HashedString::NullString, Tag ) );
	}

	STATICHASH( NumPieces );
	const uint NumPieces = ConfigManager::GetInheritedInt( sNumPieces, 0, Tag );
	for( uint Index = 0; Index < NumPieces; ++Index )
	{
		Outfit.m_Pieces.PushBack( ConfigManager::GetInheritedSequenceHash( "Piece%d", Index, HashedString::NullString, Tag ) );
	}

	STATICHASH( NumAccessories );
	const uint NumAccessories = ConfigManager::GetInheritedInt( sNumAccessories, 0, Tag );
	for( uint Index = 0; Index < NumAccessories; ++Index )
	{
		Outfit.m_Accessories.PushBack( ConfigManager::GetInheritedSequenceHash( "Accessory%d", Index, HashedString::NullString, Tag ) );
	}

	return Outfit;
}

const RosaWardrobe::SStyle& RosaWardrobe::GetStyle( const HashedString& Tag )
{
	Map<HashedString, SStyle>::Iterator StyleIter = m_Styles.Search( Tag );
	if( StyleIter.IsValid() )
	{
		return StyleIter.GetValue();
	}

	SStyle& Style = m_Styles[ Tag ];

	STATICHASH( Mesh );
	Style.m_Mesh = ConfigManager::GetInheritedString( sMesh, "", Tag );

	return Style;
}

const RosaWardrobe::SScheme& RosaWardrobe::GetScheme( const HashedString& Tag )
{
	Map<HashedString, SScheme>::Iterator SchemeIter = m_Schemes.Search( Tag );
	if( SchemeIter.IsValid() )
	{
		return SchemeIter.GetValue();
	}

	SScheme& Scheme = m_Schemes[ Tag ];

	Scheme.m_PrimaryColorHSV	= HSV::GetConfigHSV( "PrimaryColor", Tag, Vector() );
	Scheme.m_SecondaryColorHSV	= HSV::GetConfigHSV( "SecondaryColor", Tag, Vector() );
	Scheme.m_AccentColorHSV		= HSV::GetConfigHSV( "AccentColor", Tag, Vector() );

	return Scheme;
}

const RosaWardrobe::SPiece& RosaWardrobe::GetPiece( const HashedString& Tag )
{
	Map<HashedString, SPiece>::Iterator Iter = m_Pieces.Search( Tag );
	if( Iter.IsValid() )
	{
		return Iter.GetValue();
	}

	SPiece& Piece = m_Pieces[ Tag ];

	STATICHASH( NumOptions );
	const uint NumOptions = ConfigManager::GetInheritedInt( sNumOptions, 0, Tag );
	for( uint Index = 0; Index < NumOptions; ++Index )
	{
		Piece.m_Options.PushBack( ConfigManager::GetInheritedSequenceHash( "Option%d", Index, HashedString::NullString, Tag ) );
	}

	return Piece;
}

const RosaWardrobe::SOption& RosaWardrobe::GetOption( const HashedString& Tag )
{
	Map<HashedString, SOption>::Iterator OptionIter = m_Options.Search( Tag );
	if( OptionIter.IsValid() )
	{
		return OptionIter.GetValue();
	}

	SOption& Option = m_Options[ Tag ];

	STATICHASH( Mesh );
	Option.m_Mesh = ConfigManager::GetInheritedString( sMesh, "", Tag );

	return Option;
}

const RosaWardrobe::SAccessory& RosaWardrobe::GetAccessory( const HashedString& Tag )
{
	Map<HashedString, SAccessory>::Iterator AccessoryIter = m_Accessories.Search( Tag );
	if( AccessoryIter.IsValid() )
	{
		return AccessoryIter.GetValue();
	}

	SAccessory& Accessory = m_Accessories[ Tag ];

	STATICHASH( Mesh );
	Accessory.m_Mesh = ConfigManager::GetInheritedString( sMesh, "", Tag );

	STATICHASH( Bone );
	Accessory.m_Bone = ConfigManager::GetInheritedHash( sBone, HashedString::NullString, Tag );

	STATICHASH( OffsetX );
	Accessory.m_LocationOffset.x = ConfigManager::GetInheritedFloat( sOffsetX, 0.0f, Tag );

	STATICHASH( OffsetY );
	Accessory.m_LocationOffset.y = ConfigManager::GetInheritedFloat( sOffsetY, 0.0f, Tag );

	STATICHASH( OffsetZ );
	Accessory.m_LocationOffset.z = ConfigManager::GetInheritedFloat( sOffsetZ, 0.0f, Tag );

	STATICHASH( OffsetPitch );
	Accessory.m_OrientationOffset.Pitch = ConfigManager::GetInheritedFloat( sOffsetPitch, 0.0f, Tag );

	STATICHASH( OffsetRoll );
	Accessory.m_OrientationOffset.Roll = ConfigManager::GetInheritedFloat( sOffsetRoll, 0.0f, Tag );

	STATICHASH( OffsetYaw );
	Accessory.m_OrientationOffset.Yaw = ConfigManager::GetInheritedFloat( sOffsetYaw, 0.0f, Tag );

	return Accessory;
}

#define VERSION_EMPTY		0
#define VERSION_BASE		1
#define VERSION_ISVALID		2
#define VERSION_ACCESSORY	3
#define VERSION_HAIRSTYLE	4
#define VERSION_SCALE		5
#define VERSION_EYES		6
#define VERSION_ISHUMAN		7
#define VERSION_WARDROBE	8
#define VERSION_CURRENT		8

uint RosaWardrobe::GetCostumeSerializationSize( const SCostume& Costume )
{
	uint Size = 0;

	Size += 4;	// Version

	Size += 1;	// m_IsValid
	Size += 1;	// m_IsHuman
	Size += 4;	// m_Scale

	Size += sizeof( HashedString );	// m_Wardrobe
	Size += sizeof( HashedString );	// m_Body
	Size += sizeof( HashedString );	// m_Skin
	Size += sizeof( HashedString );	// m_Hair
	Size += sizeof( HashedString );	// m_Eyes
	Size += sizeof( HashedString );	// m_Head
	Size += sizeof( HashedString );	// m_Style
	Size += sizeof( HashedString );	// m_Accessory
	Size += sizeof( HashedString );	// m_Scheme

	Size += 4;													// m_Options.Size()
	Size += sizeof( HashedString ) * Costume.m_Options.Size();	// m_Options

	return Size;
}

void RosaWardrobe::SaveCostume( const SCostume& Costume, const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteBool( Costume.m_IsValid );
	Stream.WriteBool( Costume.m_IsHuman );
	Stream.WriteFloat( Costume.m_Scale );

	Stream.Write<HashedString>( Costume.m_Wardrobe );
	Stream.Write<HashedString>( Costume.m_Body );
	Stream.Write<HashedString>( Costume.m_Skin );
	Stream.Write<HashedString>( Costume.m_Hair );
	Stream.Write<HashedString>( Costume.m_Eyes );
	Stream.Write<HashedString>( Costume.m_Head );
	Stream.Write<HashedString>( Costume.m_Style );
	Stream.Write<HashedString>( Costume.m_Accessory );
	Stream.Write<HashedString>( Costume.m_Scheme );

	Stream.WriteArray<HashedString>( Costume.m_Options );
}

void RosaWardrobe::LoadCostume( SCostume& Costume, const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_ISVALID )
	{
		Costume.m_IsValid = Stream.ReadBool();
	}

	if( Version >= VERSION_ISHUMAN )
	{
		Costume.m_IsHuman = Stream.ReadBool();
	}

	Costume.m_Scale = ( Version >= VERSION_SCALE ) ? Stream.ReadFloat() : 1.0f;

	if( Version >= VERSION_BASE )
	{
		if( Version >= VERSION_WARDROBE )
		{
			Costume.m_Wardrobe	= Stream.Read<HashedString>();
		}

		Costume.m_Body			= Stream.Read<HashedString>();
		Costume.m_Skin			= Stream.Read<HashedString>();

		if( Version >= VERSION_HAIRSTYLE )
		{
			Costume.m_Hair		= Stream.Read<HashedString>();
		}

		if( Version >= VERSION_EYES )
		{
			Costume.m_Eyes		= Stream.Read<HashedString>();
		}

		Costume.m_Head			= Stream.Read<HashedString>();

		if( Version >= VERSION_HAIRSTYLE )
		{
			Costume.m_Style		= Stream.Read<HashedString>();
		}

		if( Version >= VERSION_ACCESSORY )
		{
			Costume.m_Accessory	= Stream.Read<HashedString>();
		}

		Costume.m_Scheme		= Stream.Read<HashedString>();

		Stream.ReadArray<HashedString>( Costume.m_Options );
	}
}
