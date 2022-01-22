#include "core.h"
#include "wbcomprosahands.h"
#include "wbeventmanager.h"
#include "hashedstring.h"
#include "wbcomprosainventory.h"
#include "wbcomprosaitem.h"
#include "idatastream.h"
#include "inputsystem.h"
#include "configmanager.h"

WBCompRosaHands::WBCompRosaHands()
:	m_HideHandsRefs( 0 )
,	m_HidingNoWeapon( false )
,	m_ModalEntity()
,	m_BraceletLeftBone()
,	m_BraceletLeftOffset()
,	m_BraceletRightBone()
,	m_BraceletRightOffset()
{
	STATIC_HASHED_STRING( OnWorldLoaded );
	GetEventManager()->AddObserver( sOnWorldLoaded, this );

	STATIC_HASHED_STRING( OnToggledLeftyMode );
	GetEventManager()->AddObserver( sOnToggledLeftyMode, this );
}

WBCompRosaHands::~WBCompRosaHands()
{
	WBEventManager* const pEventManager = GetEventManager();
	if( pEventManager )
	{
		STATIC_HASHED_STRING( OnWorldLoaded );
		pEventManager->RemoveObserver( sOnWorldLoaded, this );

		STATIC_HASHED_STRING( OnToggledLeftyMode );
		pEventManager->RemoveObserver( sOnToggledLeftyMode, this );
	}
}

/*virtual*/ void WBCompRosaHands::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( BraceletLeftBone );
	m_BraceletLeftBone = ConfigManager::GetInheritedHash( sBraceletLeftBone, HashedString::NullString, sDefinitionName );

	STATICHASH( BraceletLeftOffsetX );
	m_BraceletLeftOffset.x = ConfigManager::GetInheritedFloat( sBraceletLeftOffsetX, 0.0f, sDefinitionName );

	STATICHASH( BraceletLeftOffsetY );
	m_BraceletLeftOffset.y = ConfigManager::GetInheritedFloat( sBraceletLeftOffsetY, 0.0f, sDefinitionName );

	STATICHASH( BraceletLeftOffsetZ );
	m_BraceletLeftOffset.z = ConfigManager::GetInheritedFloat( sBraceletLeftOffsetZ, 0.0f, sDefinitionName );

	STATICHASH( BraceletRightBone );
	m_BraceletRightBone = ConfigManager::GetInheritedHash( sBraceletRightBone, HashedString::NullString, sDefinitionName );

	STATICHASH( BraceletRightOffsetX );
	m_BraceletRightOffset.x = ConfigManager::GetInheritedFloat( sBraceletRightOffsetX, 0.0f, sDefinitionName );

	STATICHASH( BraceletRightOffsetY );
	m_BraceletRightOffset.y = ConfigManager::GetInheritedFloat( sBraceletRightOffsetY, 0.0f, sDefinitionName );

	STATICHASH( BraceletRightOffsetZ );
	m_BraceletRightOffset.z = ConfigManager::GetInheritedFloat( sBraceletRightOffsetZ, 0.0f, sDefinitionName );
}

/*virtual*/ void WBCompRosaHands::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( OnSpawned );
	STATIC_HASHED_STRING( OnWorldLoaded );
	STATIC_HASHED_STRING( OnItemEquipped );
	STATIC_HASHED_STRING( OnItemCycled );
	STATIC_HASHED_STRING( ForceShowHands );
	STATIC_HASHED_STRING( ShowHands );
	STATIC_HASHED_STRING( HideHands );
	STATIC_HASHED_STRING( BeginModalHands );
	STATIC_HASHED_STRING( EndModalHands );
	STATIC_HASHED_STRING( PlayHandAnim );
	STATIC_HASHED_STRING( SetHandAnimBlend );
	STATIC_HASHED_STRING( SetHandMeshes );
	STATIC_HASHED_STRING( OnToggledLeftyMode );

	// Marshaled to weapon
	STATIC_HASHED_STRING( SpendAmmo );
	STATIC_HASHED_STRING( TryReload );
	STATIC_HASHED_STRING( TryCycleMagazine );
	STATIC_HASHED_STRING( OnAmmoBagUpdated );
	STATIC_HASHED_STRING( UseWeapon );
	STATIC_HASHED_STRING( TryShove );
	STATIC_HASHED_STRING( TryAim );
	STATIC_HASHED_STRING( TryUnAim );
	STATIC_HASHED_STRING( TryCycleToSlot );
	STATIC_HASHED_STRING( FlushMagazines );

	const HashedString EventName = Event.GetEventName();
	if( EventName == sOnSpawned )
	{
		if( NULL == GetItemInHands() )
		{
			m_HidingNoWeapon = true;
			IncrementHideHandsRefs();
		}
	}
	else if( EventName == sOnWorldLoaded )
	{
		// HACKHACK, in case game is saved while modal
		if( IsModal() )
		{
			EndModalHands();
		}

		// Notify weapon
		WB_MAKE_EVENT( OnWeaponEquipped, GetEntity() );
		WB_DISPATCH_EVENT( GetEventManager(), OnWeaponEquipped, GetItemInHands() );

		SetBraceletAttachment( true );
		SetBraceletAttachment( false );

		RestoreHandAnimations();
	}
	else if( EventName == sOnItemEquipped )
	{
		STATIC_HASHED_STRING( Item );
		WBEntity* const pItem = Event.GetEntity( sItem );
		ASSERT( pItem );

		if( pItem == GetHands() && m_HidingNoWeapon )
		{
			// HACKHACK: Fix up hiding hands after travel
			HideHands( EHF_Primary );
		}
		else if( pItem == GetLeftBracelet() )
		{
			SetBraceletAttachment( true );
			RestoreHandAnimations();
		}
		else if( pItem == GetRightBracelet() )
		{
			SetBraceletAttachment( false );
			RestoreHandAnimations();
		}
	}
	else if( EventName == sOnItemCycled )
	{
		STATIC_HASHED_STRING( Item );
		WBEntity* const pItem = Event.GetEntity( sItem );
		ASSERT( pItem );

		if( pItem == GetItemInHands() )
		{
			// Notify weapon to show ammo
			WB_MAKE_EVENT( OnWeaponEquipped, GetEntity() );
			WB_DISPATCH_EVENT( GetEventManager(), OnWeaponEquipped, GetItemInHands() );

			if( !IsModal() )
			{
				AddAnimationsToHands( pItem );
			}

			if( m_HidingNoWeapon )
			{
				m_HidingNoWeapon = false;
				DecrementHideHandsRefs();
			}
		}
	}
	else if( EventName == sForceShowHands )
	{
		if( m_HideHandsRefs > 0 )
		{
			m_HideHandsRefs = 1;
			DecrementHideHandsRefs();
		}
	}
	else if( EventName == sShowHands )
	{
		DecrementHideHandsRefs();
	}
	else if( EventName == sHideHands )
	{
		IncrementHideHandsRefs();
	}
	else if( EventName == sBeginModalHands )
	{
		STATIC_HASHED_STRING( Item );
		const HashedString Item = Event.GetHash( sItem );
		WBEntity* const pModalItem = GetInventory()->GetItem( Item );

		BeginModalHands( pModalItem );
	}
	else if( EventName == sEndModalHands )
	{
		EndModalHands();
	}
	else if( EventName == sPlayHandAnim )
	{
		WBEntity* const pDefaultEntity = IsModal() ? m_ModalEntity.Get() : GetItemInHands();

		STATIC_HASHED_STRING( AnimatingEntity );
		WBEntity* const pAnimatingEntity = Event.GetEntity( sAnimatingEntity, pDefaultEntity );

		STATIC_HASHED_STRING( AnimationName );
		const HashedString AnimationName = Event.GetHash( sAnimationName );

		STATIC_HASHED_STRING( Loop );
		const bool Loop = Event.GetBool( sLoop );

		STATIC_HASHED_STRING( PlayRate );
		const float PlayRate = Event.GetFloat( sPlayRate );

		STATIC_HASHED_STRING( BlendTime );
		const float BlendTime = Event.GetFloat( sBlendTime );

		if( pAnimatingEntity == m_ModalEntity.Get() ||
			pAnimatingEntity == GetItemInHands() )
		{
			PlayAnimation( pAnimatingEntity, AnimationName, Loop, PlayRate, BlendTime );
		}
#if BUILD_DEV
		else
		{
			// Catch cases where the wrong thing is requesting hand anim
			WARN;
		}
#endif
	}
	else if( EventName == sSetHandAnimBlend )
	{
		WBEntity* const pDefaultEntity = IsModal() ? m_ModalEntity.Get() : GetItemInHands();

		STATIC_HASHED_STRING( AnimatingEntity );
		WBEntity* const pAnimatingEntity = Event.GetEntity( sAnimatingEntity, pDefaultEntity );

		STATIC_HASHED_STRING( AnimationNameA );
		const HashedString AnimationNameA = Event.GetHash( sAnimationNameA );

		STATIC_HASHED_STRING( AnimationNameB );
		const HashedString AnimationNameB = Event.GetHash( sAnimationNameB );

		STATIC_HASHED_STRING( BlendAlpha );
		const float BlendAlpha = Event.GetFloat( sBlendAlpha );

		if( pAnimatingEntity == m_ModalEntity.Get() ||
			pAnimatingEntity == GetItemInHands() )
		{
			SetAnimationBlend( pAnimatingEntity, AnimationNameA, AnimationNameB, BlendAlpha );
		}
#if BUILD_DEV
		else
		{
			// Catch cases where the wrong thing is requesting hand anim
			WARN;
		}
#endif
	}
	else if( EventName == sSetHandMeshes )
	{
		STATIC_HASHED_STRING( HandsMesh );
		const SimpleString HandsMesh = Event.GetString( sHandsMesh );

		SetHandMeshes( HandsMesh );
		RestoreHandAnimations();
	}
	else if( EventName == sOnToggledLeftyMode )
	{
		SetBraceletAttachment( true );
		SetBraceletAttachment( false );
	}
	else if(
		EventName == sSpendAmmo			||
		EventName == sTryReload			||
		EventName == sTryCycleMagazine	||
		EventName == sOnAmmoBagUpdated	||
		EventName == sUseWeapon			||
		EventName == sTryShove			||
		EventName == sTryAim			||
		EventName == sTryUnAim			||
		EventName == sTryCycleToSlot	||
		EventName == sFlushMagazines )
	{
		// Marshal these events to the selected weapon
		WBEntity* const pWeapon = GetItemInHands();
		if( pWeapon )
		{
			GetEventManager()->DispatchEvent( Event, pWeapon );
		}
	}
}

void WBCompRosaHands::IncrementHideHandsRefs()
{
	if( ++m_HideHandsRefs == 1 )
	{
		HideHands( EHF_Primary );
	}
}

void WBCompRosaHands::DecrementHideHandsRefs()
{
	if( m_HideHandsRefs > 0 )
	{
		if( --m_HideHandsRefs == 0 )
		{
			if( !m_HidingNoWeapon )
			{
				ShowHands( EHF_Primary );
			}
		}
	}
}

void WBCompRosaHands::HideHands( const uint Flags ) const
{
	WB_MAKE_EVENT( Hide, GetEntity() );

	if( Flags & EHF_Hands )
	{
		WB_DISPATCH_EVENT( GetEventManager(), Hide, GetHands() );
	}

	if( Flags & EHF_Item )
	{
		WB_DISPATCH_EVENT( GetEventManager(), Hide, GetItemInHands() );
	}

	if( Flags & EHF_Bracelets )
	{
		WB_DISPATCH_EVENT( GetEventManager(), Hide, GetLeftBracelet() );
		WB_DISPATCH_EVENT( GetEventManager(), Hide, GetRightBracelet() );
	}

	if( Flags & EHF_ModalItem )
	{
		WB_DISPATCH_EVENT( GetEventManager(), Hide, m_ModalEntity.Get() );
	}
}

void WBCompRosaHands::ShowHands( const uint Flags ) const
{
	WB_MAKE_EVENT( Show, GetEntity() );

	if( Flags & EHF_Hands )
	{
		WB_DISPATCH_EVENT( GetEventManager(), Show, GetHands() );
	}

	if( Flags & EHF_Item )
	{
		WB_DISPATCH_EVENT( GetEventManager(), Show, GetItemInHands() );
	}

	if( Flags & EHF_Bracelets )
	{
		WB_DISPATCH_EVENT( GetEventManager(), Show, GetLeftBracelet() );
		WB_DISPATCH_EVENT( GetEventManager(), Show, GetRightBracelet() );
	}

	if( Flags & EHF_ModalItem )
	{
		WB_DISPATCH_EVENT( GetEventManager(), Show, m_ModalEntity.Get() );
	}
}

void WBCompRosaHands::PlayAnimation( WBEntity* const pAnimatingEntity, const HashedString& AnimationName, const bool Loop, const float PlayRate, const float BlendTime ) const
{
	if( !pAnimatingEntity )
	{
		return;
	}

	WB_MAKE_EVENT( PlayAnim, GetEntity() );
	WB_SET_AUTO( PlayAnim, Hash,	AnimationName,	AnimationName );
	WB_SET_AUTO( PlayAnim, Bool,	Loop,			Loop );
	WB_SET_AUTO( PlayAnim, Float,	PlayRate,		PlayRate );
	WB_SET_AUTO( PlayAnim, Float,	BlendTime,		BlendTime );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), PlayAnim, pAnimatingEntity );
}

void WBCompRosaHands::SetAnimationBlend( WBEntity* const pAnimatingEntity, const HashedString& AnimationNameA, const HashedString& AnimationNameB, const float BlendAlpha ) const
{
	if( !pAnimatingEntity )
	{
		return;
	}

	WB_MAKE_EVENT( SetAnimBlend, GetEntity() );
	WB_SET_AUTO( SetAnimBlend, Hash,	AnimationNameA,	AnimationNameA );
	WB_SET_AUTO( SetAnimBlend, Hash,	AnimationNameB,	AnimationNameB );
	WB_SET_AUTO( SetAnimBlend, Float,	BlendAlpha,		BlendAlpha );
	WB_DISPATCH_EVENT( WBWorld::GetInstance()->GetEventManager(), SetAnimBlend, pAnimatingEntity );
}

void WBCompRosaHands::AddAnimationsToHands( WBEntity* const pItem ) const
{
	if( !pItem )
	{
		return;
	}

	if( pItem == GetHands() )
	{
		return;
	}

	WBEventManager* const pEventManager = WBWorld::GetInstance()->GetEventManager();

	// Add animation to hands
	// NOTE: This also points the hand mesh at the item's animation state so their
	// animations stay in sync; see Mesh::CopyAnimationsFrom if this is ever a problem.
	WB_MAKE_EVENT( CopyAnimations, GetEntity() );
	WB_SET_AUTO( CopyAnimations, Entity, SourceEntity, pItem );
	WB_DISPATCH_EVENT( pEventManager, CopyAnimations, GetHands() );

	// Also add animation to bracelets, if any. They're separate entities, not
	// attachments, so they don't use mesh attachment code and actually need
	// the animation data. But it's shared data so there's no overhead.
	WB_DISPATCH_EVENT( pEventManager, CopyAnimations, GetLeftBracelet() );
	WB_DISPATCH_EVENT( pEventManager, CopyAnimations, GetRightBracelet() );
}

void WBCompRosaHands::SetHandMeshes( const SimpleString& HandsMesh ) const
{
	WB_MAKE_EVENT( SetMesh, GetEntity() );
	WB_SET_AUTO( SetMesh, Hash, Mesh, HandsMesh );
	WB_DISPATCH_EVENT( GetEventManager(), SetMesh, GetHands() );
}

void WBCompRosaHands::BeginModalHands( WBEntity* const pItem )
{
	m_ModalEntity = pItem;

	DEVASSERT( pItem );
	const WBCompRosaItem* const	pItemComponent	= WB_GETCOMP( pItem, RosaItem );
	DEVASSERT( pItemComponent );
	const bool					KeepHands		= pItemComponent->KeepHands();

	if( m_HidingNoWeapon )
	{
		ShowHands( KeepHands ? EHF_Primary : EHF_None );
	}
	else
	{
		HideHands( KeepHands ? EHF_Item : EHF_Primary_NoBracelets );
	}

	ShowHands( EHF_ModalItem );
	AddAnimationsToHands( pItem );
	SetBraceletsTransform( m_ModalEntity.Get() );
}

void WBCompRosaHands::EndModalHands()
{
	DEVASSERT( m_ModalEntity.Get() );
	const WBCompRosaItem* const	pItemComponent	= WB_GETCOMP( m_ModalEntity.Get(), RosaItem );
	DEVASSERT( pItemComponent );
	const bool					KeepHands		= pItemComponent->KeepHands();

	if( m_HidingNoWeapon )
	{
		HideHands( KeepHands ? EHF_Primary : EHF_None );
	}
	else
	{
		ShowHands( KeepHands ? EHF_Item : EHF_Primary_NoBracelets );
	}

	HideHands( EHF_ModalItem );
	RestoreHandAnimations();
	SetBraceletsTransform( GetHands() );

	m_ModalEntity = NULL;
}

void WBCompRosaHands::SetBraceletsTransform( WBEntity* const pEntity )
{
	WB_MAKE_EVENT( CopyOwnerOffset, GetEntity() );
	WB_SET_AUTO( CopyOwnerOffset, Entity, Source, pEntity );
	WB_DISPATCH_EVENT( GetEventManager(), CopyOwnerOffset, GetLeftBracelet() );
	WB_DISPATCH_EVENT( GetEventManager(), CopyOwnerOffset, GetRightBracelet() );
}

void WBCompRosaHands::SetBraceletAttachment( const bool LeftBracelet )
{
	STATICHASH( LeftyMode );
	const bool LeftyMode = ConfigManager::GetBool( sLeftyMode );

	// LM LB | Use:
	// F  F  | R
	// F  T  | L
	// T  F  | L
	// T  T  | R
	const bool UseLeft = ( LeftyMode != LeftBracelet );

	STATIC_HASHED_STRING( Bracelet );
	WB_MAKE_EVENT( SetMeshAttachmentTransform, NULL );
	WB_SET_AUTO( SetMeshAttachmentTransform, Hash, Tag, sBracelet );
	WB_SET_AUTO( SetMeshAttachmentTransform, Hash, Bone, UseLeft ? m_BraceletLeftBone : m_BraceletRightBone );
	WB_SET_AUTO( SetMeshAttachmentTransform, Vector, Offset, UseLeft ? m_BraceletLeftOffset : m_BraceletRightOffset );
	WB_DISPATCH_EVENT( GetEventManager(), SetMeshAttachmentTransform, LeftBracelet ? GetLeftBracelet() : GetRightBracelet() );
}

void WBCompRosaHands::RestoreHandAnimations() const
{
	// Restore hand animations from whatever is equipped.
	AddAnimationsToHands( GetItemInHands() );
}

WBCompRosaInventory* WBCompRosaHands::GetInventory() const
{
	WBCompRosaInventory* const pInventory = WB_GETCOMP( GetEntity(), RosaInventory );
	ASSERT( pInventory );
	return pInventory;
}

WBEntity* WBCompRosaHands::GetHands() const
{
	STATIC_HASHED_STRING( Hands );
	return GetInventory()->GetItem( sHands );
}

WBEntity* WBCompRosaHands::GetExamineHands() const
{
	STATIC_HASHED_STRING( Hands_Examine );
	return GetInventory()->GetItem( sHands_Examine );
}

WBEntity* WBCompRosaHands::GetItemInHands() const
{
	return GetInventory()->GetCycleItem();
}

WBEntity* WBCompRosaHands::GetLeftBracelet() const
{
	STATIC_HASHED_STRING( BraceletLeft );
	return GetInventory()->GetItem( sBraceletLeft );
}

WBEntity* WBCompRosaHands::GetRightBracelet() const
{
	STATIC_HASHED_STRING( BraceletRight );
	return GetInventory()->GetItem( sBraceletRight );
}

#define VERSION_EMPTY			0
#define VERSION_HIDEHANDSREFS	1
#define VERSION_HIDINGNOWEAPON	3
#define VERSION_ISMODAL			4
#define VERSION_CURRENT			4

uint WBCompRosaHands::GetSerializationSize()
{
	uint Size = 0;

	Size += 4;						// Version
	Size += 4;						// m_HideHandsRefs
	Size += 1;						// m_HidingNoWeapon
	Size += sizeof( WBEntityRef );	// m_ModalEntity

	return Size;
}

void WBCompRosaHands::Save( const IDataStream& Stream )
{
	Stream.WriteUInt32( VERSION_CURRENT );

	Stream.WriteInt32( m_HideHandsRefs );
	Stream.WriteBool( m_HidingNoWeapon );
	Stream.Write<WBEntityRef>( m_ModalEntity );
}

void WBCompRosaHands::Load( const IDataStream& Stream )
{
	XTRACE_FUNCTION;

	const uint Version = Stream.ReadUInt32();

	if( Version >= VERSION_HIDEHANDSREFS )
	{
		m_HideHandsRefs = Stream.ReadInt32();
	}

	if( Version >= VERSION_HIDINGNOWEAPON )
	{
		m_HidingNoWeapon = Stream.ReadBool();
	}

	if( Version >= VERSION_ISMODAL )
	{
		m_ModalEntity = Stream.Read<WBEntityRef>();
	}
}
