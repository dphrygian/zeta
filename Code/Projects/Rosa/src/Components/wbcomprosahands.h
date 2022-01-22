#ifndef WBCOMPROSAHANDS_H
#define WBCOMPROSAHANDS_H

#include "wbrosacomponent.h"
#include "hashedstring.h"
#include "vector.h"
#include "wbentityref.h"

class WBCompRosaInventory;

class WBCompRosaHands : public WBRosaComponent
{
public:
	WBCompRosaHands();
	virtual ~WBCompRosaHands();

	DEFINE_WBCOMP( RosaHands, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	bool			IsHidden() const	{ return m_HideHandsRefs > 0; }
	bool			IsModal() const		{ return m_ModalEntity.Get() != NULL; }

	WBEntity*		GetHands() const;
	WBEntity*		GetExamineHands() const;
	WBEntity*		GetItemInHands() const;
	WBEntity*		GetLeftBracelet() const;
	WBEntity*		GetRightBracelet() const;

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	enum EHandFlags
	{
		EHF_None		= 0x00,
		EHF_Hands		= 0x01,
		EHF_Item		= 0x02,
		EHF_Bracelets	= 0x04,
		EHF_ModalItem	= 0x08,

		EHF_Primary				= EHF_Hands | EHF_Item | EHF_Bracelets,
		EHF_Primary_NoBracelets	= EHF_Hands | EHF_Item,
	};

	void	IncrementHideHandsRefs();
	void	DecrementHideHandsRefs();
	void	HideHands( const uint Flags ) const;
	void	ShowHands( const uint Flags ) const;

	void	BeginModalHands( WBEntity* const pItem );
	void	EndModalHands();
	void	SetBraceletsTransform( WBEntity* const pEntity );
	void	SetBraceletAttachment( const bool LeftBracelet );

	void	PlayAnimation( WBEntity* const pAnimatingEntity, const HashedString& AnimationName, const bool Loop, const float PlayRate, const float BlendTime ) const;
	void	SetAnimationBlend( WBEntity* const pAnimatingEntity, const HashedString& AnimationNameA, const HashedString& AnimationNameB, const float BlendAlpha ) const;
	void	AddAnimationsToHands( WBEntity* const pItem ) const;
	void	SetHandMeshes( const SimpleString& HandsMesh ) const;
	void	RestoreHandAnimations() const;

	WBCompRosaInventory*	GetInventory() const;

	int				m_HideHandsRefs;	// Serialized, refcount for hiding hands
	bool			m_HidingNoWeapon;	// Serialized, extra state for hiding hands because we have no weapon
	WBEntityRef		m_ModalEntity;		// Serialized, entity for current "modal hands"

	// Config; for assigning mesh attachment offsets when a bracelet is equipped
	HashedString	m_BraceletLeftBone;
	Vector			m_BraceletLeftOffset;
	HashedString	m_BraceletRightBone;
	Vector			m_BraceletRightOffset;
};

#endif // WBCOMPROSAHANDS_H
