#include "core.h"
#include "Components/wbcomprodinbehaviortree.h"
#include "rosaframework.h"
#include "irenderer.h"
#include "view.h"
#include "display.h"
#include "wbcomprosatransform.h"
#include "wbentity.h"
#include "fontmanager.h"
#include "vector4.h"

#if BUILD_DEV
/*virtual*/ void WBCompRodinBehaviorTree::DebugRender( const bool GroupedRender ) const
{
	Super::DebugRender( GroupedRender );

	RosaFramework*				pFramework	= RosaFramework::GetInstance();
	IRenderer* const			pRenderer	= pFramework->GetRenderer();
	View* const					pView		= pFramework->GetMainView();
	Display* const				pDisplay	= pFramework->GetDisplay();
	WBCompRosaTransform* const	pTransform	= GetEntity()->GetTransformComponent<WBCompRosaTransform>();
	const Vector				Location	= pTransform->GetLocation();

	if( m_ScheduledNodes.Size() > 0 )
	{
		const SimpleString		TreeString	= SimpleString::PrintF( "%sWBCompRodinBehaviorTree : %s", DebugRenderLineFeed().CStr(), GetEntity()->GetUniqueName().CStr() );
		pRenderer->DEBUGPrint( TreeString, Location, pView, pDisplay, DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 192, 255, 128 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );
		FOR_EACH_ARRAY( NodeIter, m_ScheduledNodes, SScheduledNode )
		{
			const SScheduledNode&	Node		= NodeIter.GetValue();

			// Skip drawing if an ancestor is collapsed
			bool IsAncestorCollapsed = false;
			for( RodinBTNode* pParentNode = Node.m_ParentNode; pParentNode != NULL; pParentNode = GetParentNode( pParentNode ) )
			{
				if( pParentNode->m_DEV_CollapseDebug )
				{
					IsAncestorCollapsed = true;
					break;
				}
			}
			if( IsAncestorCollapsed )
			{
				continue;
			}

			SimpleString NodeString = DebugRenderLineFeed();
			NodeString += "  -";
			FOR_EACH_INDEX( NodeDepth, Node.m_Node->m_DEV_Depth )
			{
				NodeString += "-";
			}
			NodeString += " ";
			NodeString += Node.m_Node->GetDebugName();

			if( Node.m_Node->m_DEV_CollapseDebug )
			{
				NodeString += " (COLLAPSED)";
			}

			pRenderer->DEBUGPrint( NodeString, Location, pView, pDisplay, DEFAULT_FONT_TAG, ARGB_TO_COLOR( 255, 192, 255, 128 ), ARGB_TO_COLOR( 255, 0, 0, 0 ) );
		}
	}
}
#endif
