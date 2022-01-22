#include "core.h"
#include "Components/wbcompstatmod.h"
#include "rosaframework.h"
#include "irenderer.h"
#include "view.h"
#include "display.h"
#include "wbcomprosatransform.h"
#include "wbentity.h"
#include "fontmanager.h"
#include "vector4.h"
#include "reversehash.h"

#if BUILD_DEV
/*virtual*/ void WBCompStatMod::DebugRender( const bool GroupedRender ) const
{
	Super::DebugRender( GroupedRender );

	RosaFramework* const		pFramework	= RosaFramework::GetInstance();
	IRenderer* const			pRenderer	= pFramework->GetRenderer();
	View* const					pView		= pFramework->GetMainView();
	Display* const				pDisplay	= pFramework->GetDisplay();

	WBCompRosaTransform* const	pTransform	= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );
	const Vector				Location	= pTransform->GetLocation();

	pRenderer->DEBUGPrint(
		SimpleString::PrintF(
			"%sActive StatMod events (refcounts):",
			DebugRenderLineFeed().CStr()
		),
		Location,
		pView,
		pDisplay,
		DEFAULT_FONT_TAG,
		ARGB_TO_COLOR( 255, 255, 255, 0 ),
		ARGB_TO_COLOR( 255, 0, 0, 0 ) );

	FOR_EACH_SET( ActiveEventIter, m_ActiveEvents, HashedString )
	{
		const HashedString&						ActiveEvent				= ActiveEventIter.GetValue();
		//const Map<HashedString, uint>::Iterator	ActiveEventRefCountIter	= m_ActiveEventRefCounts.Search( ActiveEvent );
		uint* const								pRefCount				= m_ActiveEventRefCounts.SearchPointer( ActiveEvent );	// Trying the Unreal style instead of the above
		pRenderer->DEBUGPrint(
			SimpleString::PrintF(
				"%s  %s (%d)",
				DebugRenderLineFeed().CStr(),
				ReverseHash::ReversedHash( ActiveEvent ).CStr(),
				//( ActiveEventRefCountIter.IsValid() ? ActiveEventRefCountIter.GetValue() : 0 )
				( pRefCount ? *pRefCount : 0 )																					// Trying the Unreal style instead of the above
			),
			Location,
			pView,
			pDisplay,
			DEFAULT_FONT_TAG,
			ARGB_TO_COLOR( 255, 255, 255, 0 ),
			ARGB_TO_COLOR( 255, 0, 0, 0 ) );
	}
}
#endif
