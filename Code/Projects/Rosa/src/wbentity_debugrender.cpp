#include "core.h"
#include "wbentity.h"
#include "rosaframework.h"
#include "irenderer.h"
#include "view.h"
#include "display.h"
#include "Components/wbcomprosatransform.h"
#include "fontmanager.h"
#include "vector4.h"

#if BUILD_DEV
/*virtual*/ void WBEntity::DebugRender_Internal() const
{
	RosaFramework*				pFramework	= RosaFramework::GetInstance();
	IRenderer* const			pRenderer	= pFramework->GetRenderer();
	View* const					pView		= pFramework->GetMainView();
	Display* const				pDisplay	= pFramework->GetDisplay();
	WBCompRosaTransform* const	pTransform	= GetTransformComponent<WBCompRosaTransform>();
	const Vector				Location	= pTransform->GetLocation();

	pRenderer->DEBUGPrint(
		SimpleString::PrintF( "%s%s", DebugRenderLineFeed().CStr(), GetUniqueName().CStr() ),
		Location,
		pView,
		pDisplay,
		DEFAULT_FONT_TAG,
		ARGB_TO_COLOR( 255, 255, 255, 255 ),
		ARGB_TO_COLOR( 255, 0, 0, 0 )
	);
}
#endif
