#ifndef ROSAWARDROBE_H
#define ROSAWARDROBE_H

#include "simplestring.h"
#include "hashedstring.h"
#include "map.h"
#include "vector.h"
#include "vector4.h"
#include "angles.h"

// 24 Nov 2019: I walked away from generalizing this with data-driven schemas
// because it was going to turn into a massive task of building a domain-
// specific language just for wardrobe variations. Instead, I can possibly
// generalize the hard-coded schema by collapsing stuff like hairstyles and
// accessories into any number of static/skinned mesh groups.

// A Wardrobe is a possibility space for a character's appearance.
// A Costume is a singular selection from a Wardrobe.
// Wardrobes are assembled from a set of nested elements.
// Wardrobe
// |-Bodies
//   |-Gender			Just used for mapping back to campaign actors
//   |-Skins			Colors (not randomized)
//     |-Hairs			Hair colors (not randomized); parented to skin so hair and skin can be coordinated
//     |-Eyes			Eyes colors (not randomized); same as hair colors
//   |-Heads
//   |-Outfits
//     |-Styles			Hair styles; parented to outfit so it can be coordinated with fashion
//     |-Schemes		Color schemes (not randomized)
//     |-Accessories	Static attachments (1 per costume)
//     |-Pieces			Skinned attachment slots (* per costume)
//       |-Options		Skinned attachment (1 per slot)

class IDataStream;

class RosaWardrobe
{
public:
	RosaWardrobe();
	~RosaWardrobe();

	// A costume is a singular selection from a wardrobe.
	struct SCostume
	{
		SCostume()
		:	m_IsValid( false )
		,	m_IsHuman( false )
		,	m_Scale( 0.0f )
		,	m_Wardrobe()
		,	m_Body()
		,	m_Skin()
		,	m_Hair()
		,	m_Eyes()
		,	m_Head()
		,	m_Style()
		,	m_Accessory()
		,	m_Scheme()
		,	m_Options()
		{
		}

		bool				m_IsValid;
		bool				m_IsHuman;
		float				m_Scale;
		HashedString		m_Wardrobe;
		HashedString		m_Body;
		HashedString		m_Skin;
		HashedString		m_Hair;
		HashedString		m_Eyes;
		HashedString		m_Head;
		HashedString		m_Style;
		HashedString		m_Accessory;
		HashedString		m_Scheme;

		// ROSATODO: I could also store the outfit and pieces if I need to refer to them again;
		// but once the outfit is rolled, all I really need is the options.
		Array<HashedString>	m_Options;
	};

	struct SWardrobe
	{
		SWardrobe()
		:	m_Bodies()
		,	m_MaleBodies()
		,	m_FemaleBodies()
		,	m_CastsShadows( false )
		,	m_CastsSelfShadows( false )
		{
		}

		// A wardrobe is a broad palette of options for a type of character.
		Array<HashedString>	m_Bodies;	// All bodies regardless of gender (or for nongendered creatures)
		Array<HashedString>	m_MaleBodies;
		Array<HashedString>	m_FemaleBodies;
		bool				m_CastsShadows;	// Not a shipping option, just testing Taboo in old Vamp content; if I reuse the wardrobe/costume stuff, I may want this option per mesh or per section
		bool				m_CastsSelfShadows;
	};

	enum EBodyGender
	{
		EBG_None,	// or "any"
		EBG_Woman,
		EBG_Man,
	};

	struct SBody
	{
		SBody()
		:	m_Mesh()
		,	m_Gender( EBG_None )
		,	m_HumanVoice()
		,	m_VampireVoice()
		,	m_ScaleMin( 0.0f )
		,	m_ScaleMax( 0.0f )
		,	m_Skins()
		,	m_Heads()
		,	m_Outfits()
		{
		}

		// Mesh name, and whatever other properties of a mesh component are *actually relevant*
		// (i.e., I'll hard-code anything that I can safely assume 100% of the time)
		SimpleString		m_Mesh;
		EBodyGender			m_Gender;
		SimpleString		m_HumanVoice;
		SimpleString		m_VampireVoice;
		float				m_ScaleMin;
		float				m_ScaleMax;
		Array<HashedString>	m_Skins;
		Array<HashedString>	m_Heads;
		Array<HashedString>	m_Outfits;
	};

	struct SSkin
	{
		SSkin()
		:	m_HumanColorHSV()
		,	m_VampireColorHSV()
		,	m_Hairs()
		,	m_Eyes()
		{
		}

		Vector4				m_HumanColorHSV;
		Vector4				m_VampireColorHSV;
		Array<HashedString>	m_Hairs;
		Array<HashedString>	m_Eyes;
	};

	struct SHair
	{
		SHair()
		:	m_ColorHSV()
		{
		}

		Vector	m_ColorHSV;
	};

	struct SEyes
	{
		SEyes()
		:	m_ColorHSV()
		{
		}

		Vector	m_ColorHSV;
	};

	struct SHead
	{
		SHead()
		:	m_HumanMesh()
		,	m_VampireMesh()
		{
		}

		SimpleString	m_HumanMesh;
		SimpleString	m_VampireMesh;
	};

	struct SOutfit
	{
		SOutfit()
		:	m_Styles()
		,	m_Schemes()
		,	m_Pieces()
		,	m_Accessories()
		{
		}

		Array<HashedString>	m_Styles;
		Array<HashedString>	m_Schemes;
		Array<HashedString>	m_Pieces;
		Array<HashedString>	m_Accessories;
	};

	struct SStyle
	{
		SStyle()
		:	m_Mesh()
		{
		}

		SimpleString	m_Mesh;
	};

	struct SScheme
	{
		SScheme()
		:	m_PrimaryColorHSV()
		,	m_SecondaryColorHSV()
		,	m_AccentColorHSV()
		{
		}

		Vector	m_PrimaryColorHSV;
		Vector	m_SecondaryColorHSV;
		Vector	m_AccentColorHSV;
	};

	struct SPiece
	{
		SPiece()
		:	m_Options()
		{
		}

		Array<HashedString>	m_Options;
	};

	struct SOption
	{
		SOption()
		:	m_Mesh()
		{
		}

		SimpleString	m_Mesh;
	};

	struct SAccessory
	{
		SAccessory()
		:	m_Mesh()
		,	m_Bone()
		,	m_LocationOffset()
		,	m_OrientationOffset()
		{
		}

		SimpleString	m_Mesh;
		HashedString	m_Bone;
		Vector			m_LocationOffset;
		Angles			m_OrientationOffset;
	};

	// This is the primary interface for rolling a character. It initializes
	// the wardrobe and all its elements if they do not yet exist.
	SCostume			CreateCostume( const HashedString& WardrobeTag, const EBodyGender BodyGender, const bool IsHuman );

	uint				GetCostumeSerializationSize( const SCostume& Costume );
	void				SaveCostume( const SCostume& Costume, const IDataStream& Stream );
	void				LoadCostume( SCostume& Costume, const IDataStream& Stream );

	// These initialize the element and all their elements if they do not yet exist.
	const SWardrobe&	GetWardrobe( const HashedString& Tag );
	const SBody&		GetBody( const HashedString& Tag );
	const SSkin&		GetSkin( const HashedString& Tag );
	const SHair&		GetHair( const HashedString& Tag );
	const SEyes&		GetEyes( const HashedString& Tag );
	const SHead&		GetHead( const HashedString& Tag );
	const SOutfit&		GetOutfit( const HashedString& Tag );
	const SStyle&		GetStyle( const HashedString& Tag );
	const SScheme&		GetScheme( const HashedString& Tag );
	const SPiece&		GetPiece( const HashedString& Tag );
	const SOption&		GetOption( const HashedString& Tag );
	const SAccessory&	GetAccessory( const HashedString& Tag );

private:
	// Helper for getting enum from def
	EBodyGender			GetBodyGender( const HashedString& BodyGender ) const;

	Map<HashedString, SWardrobe>	m_Wardrobes;
	Map<HashedString, SBody>		m_Bodies;
	Map<HashedString, SSkin>		m_Skins;
	Map<HashedString, SHair>		m_Hairs;
	Map<HashedString, SEyes>		m_Eyes;
	Map<HashedString, SHead>		m_Heads;
	Map<HashedString, SOutfit>		m_Outfits;
	Map<HashedString, SStyle>		m_Styles;
	Map<HashedString, SScheme>		m_Schemes;
	Map<HashedString, SPiece>		m_Pieces;
	Map<HashedString, SOption>		m_Options;
	Map<HashedString, SAccessory>	m_Accessories;
};

#endif // ROSAWARDROBE_H
