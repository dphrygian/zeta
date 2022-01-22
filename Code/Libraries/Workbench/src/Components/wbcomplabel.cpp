#include "core.h"
#include "wbcomplabel.h"
#include "configmanager.h"
#include "wbentity.h"
#include "wbcomponentarrays.h"

WBCompLabel::WBCompLabel()
:	m_Label()
{
}

WBCompLabel::~WBCompLabel()
{
}

/*virtual*/ void WBCompLabel::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Label );
	m_Label = ConfigManager::GetInheritedHash( sLabel, GetEntity()->GetName(), sDefinitionName );
}

/*static*/ WBEntity* WBCompLabel::GetEntityByLabel( const HashedString& Label )
{
	const Array<WBCompLabel*>* pLabelComponents = WBComponentArrays::GetComponents<WBCompLabel>();
	if( !pLabelComponents )
	{
		return NULL;
	}

	const uint NumLabelComponents = pLabelComponents->Size();
	for( uint LabelComponentIndex = 0; LabelComponentIndex < NumLabelComponents; ++LabelComponentIndex )
	{
		WBCompLabel* const pLabelComponent = ( *pLabelComponents )[ LabelComponentIndex ];
		ASSERT( pLabelComponent );

		if( pLabelComponent->GetLabel() != Label )
		{
			continue;
		}

		WBEntity* const pLabelEntity = pLabelComponent->GetEntity();
		ASSERT( pLabelEntity );

		if( pLabelEntity->IsDestroyed() )
		{
			continue;
		}

		return pLabelEntity;
	}

	return NULL;
}

/*static*/ void WBCompLabel::GetEntitiesByLabel( const HashedString& Label, Array<WBEntity*>& OutEntities )
{
	const Array<WBCompLabel*>* pLabelComponents = WBComponentArrays::GetComponents<WBCompLabel>();
	if( !pLabelComponents )
	{
		return;
	}

	const uint NumLabelComponents = pLabelComponents->Size();
	for( uint LabelComponentIndex = 0; LabelComponentIndex < NumLabelComponents; ++LabelComponentIndex )
	{
		WBCompLabel* const pLabelComponent = ( *pLabelComponents )[ LabelComponentIndex ];
		ASSERT( pLabelComponent );

		if( pLabelComponent->GetLabel() != Label )
		{
			continue;
		}

		WBEntity* const pLabelEntity = pLabelComponent->GetEntity();
		ASSERT( pLabelEntity );

		if( pLabelEntity->IsDestroyed() )
		{
			continue;
		}

		OutEntities.PushBack( pLabelEntity );
	}
}
