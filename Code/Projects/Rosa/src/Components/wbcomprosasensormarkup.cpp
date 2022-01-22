#include "core.h"
#include "wbcomprosasensormarkup.h"
#include "wbcomprosatransform.h"
#include "Components/wbcomprodinknowledge.h"
#include "configmanager.h"
#include "wbcomponentarrays.h"
#include "wbcomprosamarkup.h"
#include "mathcore.h"

WBCompRosaSensorMarkup::WBCompRosaSensorMarkup()
:	m_Radius( 0.0f )
{
}

WBCompRosaSensorMarkup::~WBCompRosaSensorMarkup()
{
}

/*virtual*/ void WBCompRosaSensorMarkup::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Markup );
	m_Markup = ConfigManager::GetInheritedHash( sMarkup, HashedString::NullString, sDefinitionName );

	STATICHASH( Radius );
	m_Radius = ConfigManager::GetInheritedFloat( sRadius, 0.0f, sDefinitionName );
}

/*virtual*/ void WBCompRosaSensorMarkup::PollTick( const float DeltaTime ) const
{
	Unused( DeltaTime );

	const Array<WBCompRosaMarkup*>* pMarkupComponents = WBComponentArrays::GetComponents<WBCompRosaMarkup>();
	if( !pMarkupComponents )
	{
		return;
	}

	WBEntity* const				pEntity			= GetEntity();
	DEVASSERT( pEntity );

	WBCompRodinKnowledge* const	pKnowledge		= WB_GETCOMP( pEntity, RodinKnowledge );
	ASSERT( pKnowledge );

	WBCompRosaTransform* const pTransform		= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );

	const Vector				CurrentLocation	= pTransform->GetLocation();
	const float					RadiusSq		= Square( m_Radius );

	const uint NumMarkups = pMarkupComponents->Size();
	for( uint MarkupIndex = 0; MarkupIndex < NumMarkups; ++MarkupIndex )
	{
		WBCompRosaMarkup* const	pMarkup					= ( *pMarkupComponents )[ MarkupIndex ];
		ASSERT( pMarkup );

		// Only consider the desired type of markup
		if( pMarkup->GetMarkup() != m_Markup )
		{
			continue;
		}

		WBEntity* const			pMarkupEntity			= pMarkup->GetEntity();
		ASSERT( pMarkupEntity );

		WBCompRosaTransform* const	pMarkupTransform	= pMarkupEntity->GetTransformComponent<WBCompRosaTransform>();

		// Distance check
		const float DistanceSq = ( pMarkupTransform->GetLocation() - CurrentLocation ).LengthSquared();
		if( DistanceSq > RadiusSq )
		{
			continue;
		}

		// Update knowledge with this patrol
		WBCompRodinKnowledge::TKnowledge& Knowledge = pKnowledge->UpdateEntity( pMarkupEntity );

		STATIC_HASHED_STRING( DistanceSq );
		Knowledge.SetFloat( sDistanceSq, DistanceSq );

		STATIC_HASHED_STRING( LastKnownLocation );
		Knowledge.SetVector( sLastKnownLocation, pMarkupTransform->GetLocation() );
		ASSERT( !pMarkupTransform->GetLocation().IsZero() );

		STATIC_HASHED_STRING( KnowledgeType );
		Knowledge.SetHash( sKnowledgeType, m_Markup );
	}
}
