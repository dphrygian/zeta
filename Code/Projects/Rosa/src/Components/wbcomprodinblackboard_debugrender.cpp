#include "core.h"
#include "Components/wbcomprodinblackboard.h"
#include "rosaframework.h"
#include "irenderer.h"
#include "view.h"
#include "display.h"
#include "wbcomprosatransform.h"
#include "wbentity.h"
#include "fontmanager.h"
#include "reversehash.h"

#if BUILD_DEV
/*virtual*/ void WBCompRodinBlackboard::DebugRender( const bool GroupedRender ) const
{
	Super::DebugRender( GroupedRender );

	RosaFramework*				pFramework	= RosaFramework::GetInstance();
	IRenderer* const			pRenderer	= pFramework->GetRenderer();
	View* const					pView		= pFramework->GetMainView();
	Display* const				pDisplay	= pFramework->GetDisplay();
	WBCompRosaTransform* const	pTransform	= WBComponent::GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	const Vector				Location	= pTransform->GetLocation();

	const WBEvent::TParameterMap& ParameterMap = m_BlackboardEntries.GetParameters();
	FOR_EACH_MAP( ParamIter, ParameterMap, HashedString, WBEvent::SParameter )
	{
		const HashedString&			NameHash	= ParamIter.GetKey();
		const WBEvent::SParameter&	Parameter	= ParamIter.GetValue();

		const SimpleString			Name		= ReverseHash::ReversedHash( NameHash );
		const SimpleString			Value		= Parameter.CoerceString();

		pRenderer->DEBUGPrint( SimpleString::PrintF( "%s%s: %s", DebugRenderLineFeed().CStr(), Name.CStr(), Value.CStr() ), Location, pView, pDisplay, DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 192, 255, 128 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );
	}
}
#endif
