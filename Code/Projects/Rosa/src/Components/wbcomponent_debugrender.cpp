#include "core.h"
#include "wbcomponent.h"
#include "rosaframework.h"
#include "irenderer.h"
#include "view.h"
#include "display.h"
#include "wbcomprosatransform.h"
#include "wbentity.h"
#include "fontmanager.h"
#include "vector4.h"

#if BUILD_DEV
/*virtual*/ void WBComponent::DebugRender( const bool GroupedRender ) const
{
	if( GroupedRender )
	{
		// No need to print header when all components are rendering, it's just noise.
		return;
	}

	RosaFramework* const		pFramework	= RosaFramework::GetInstance();
	IRenderer* const			pRenderer	= pFramework->GetRenderer();
	View* const					pView		= pFramework->GetMainView();
	Display* const				pDisplay	= pFramework->GetDisplay();

	WBCompRosaTransform* const	pTransform	= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	DEVASSERT( pTransform );
	const Vector				Location	= pTransform->GetLocation();

	pRenderer->DEBUGPrint( SimpleString::PrintF( "%s%s", DebugRenderLineFeed().CStr(), GetReadableName() ), Location, pView, pDisplay, DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 255, 255, 255 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );
}
#endif
