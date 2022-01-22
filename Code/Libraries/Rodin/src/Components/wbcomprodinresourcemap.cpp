#include "core.h"
#include "wbcomprodinresourcemap.h"
#include "irodinresourceuser.h"
#include "wbevent.h"

WBCompRodinResourceMap::WBCompRodinResourceMap()
:	m_ResourceMap()
{
}

WBCompRodinResourceMap::~WBCompRodinResourceMap()
{
	FOR_EACH_MAP( ResourceIter, m_ResourceMap, HashedString, TResourceStack )
	{
		TResourceStack& ResourceStack = ResourceIter.GetValue();
		ResourceStack.Clear();
	}
}

/*virtual*/ void WBCompRodinResourceMap::HandleEvent( const WBEvent& Event )
{
	XTRACE_FUNCTION;

	Super::HandleEvent( Event );

	STATIC_HASHED_STRING( StealResource );

	// HACKHACK to claim and immediately release a resource by a null user
	const HashedString EventName = Event.GetEventName();
	if( EventName == sStealResource )
	{
		STATIC_HASHED_STRING( Resource );
		const HashedString Resource = Event.GetHash( sResource );

		ClaimResource( NULL, Resource, true );
		ReleaseResource( NULL, Resource );
	}
}

bool WBCompRodinResourceMap::ClaimResource( IRodinResourceUser* const pResourceUser, const HashedString& Resource, const bool ForceClaim )
{
	TResourceStack& ResourceStack = m_ResourceMap[ Resource ];

	if( ResourceStack.Empty() )
	{
		// Resource is available, take it
		ResourceStack.PushBack( pResourceUser );
		return true;
	}

	// Resource is in use, check who has it
	IRodinResourceUser* const pCurrentClaimant = ResourceStack.Last();
	if( pCurrentClaimant == pResourceUser )
	{
		// We've already got the resource
		return true;
	}

	// Someone else has the resource
	if( ForceClaim )
	{
		// Steal the resource
		const bool RemainOnStack = pCurrentClaimant->OnResourceStolen( Resource );

		// Get the resource stack again; it might have been destroyed by OnResourceStolen releasing the resource.
		TResourceStack& NewResourceStack = m_ResourceMap[ Resource ];
		if( !RemainOnStack && NewResourceStack.Size() > 0 && NewResourceStack.Last() == pCurrentClaimant )
		{
			// If the former user does not remain on stack but didn't already release the resource, replace it
			NewResourceStack.Last() = pResourceUser;
		}
		else
		{
			NewResourceStack.PushBack( pResourceUser );
		}

		return true;
	}

	// We can't get the resource
	return false;
}

void WBCompRodinResourceMap::ReleaseResource( IRodinResourceUser* const pResourceUser, const HashedString& Resource )
{
	TResourceMap::Iterator ResourceIter = m_ResourceMap.Search( Resource );

	if( ResourceIter.IsNull() )
	{
		return;
	}

	TResourceStack& ResourceStack = ResourceIter.GetValue();
	ASSERT( ResourceStack.Size() > 0 );

	// Make sure we actually have the resource
	if( ResourceStack.Last() != pResourceUser )
	{
		return;
	}

	// Remove user
	ResourceStack.PopBack();

	if( ResourceStack.Empty() )
	{
		// Free the array if this was the last user on the stack
		m_ResourceMap.Remove( ResourceIter );
	}
	else
	{
		// Else, notify the next user that it has the resource again
		IRodinResourceUser* const pCurrentClaimant = ResourceStack.Last();
		ASSERT( pCurrentClaimant );
		pCurrentClaimant->OnResourceReturned( Resource );
	}
}
