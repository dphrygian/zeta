#include "core.h"
#include "targetmanager.h"
#include "3d.h"
#include "irenderer.h"
#include "irendertarget.h"
#include "configmanager.h"

TargetManager::TargetManager( IRenderer* const pRenderer )
:	m_Renderer( pRenderer )
,	m_RenderTargets()
{
	DEVASSERT( pRenderer );
	m_Renderer->SetTargetManager( this );
}

TargetManager::~TargetManager()
{
	ReleaseTargets();
}

void TargetManager::CreateTargets( const uint DisplayWidth, const uint DisplayHeight )
{
	Unused( DisplayWidth );
	Unused( DisplayHeight );

	ReleaseTargets();

	// Original RT, i.e. the backbuffer
	m_RenderTargets.Insert( "Original", m_Renderer->GetDefaultRenderTarget() );
}

void TargetManager::ReleaseTargets()
{
	m_Renderer->FreeRenderTargets();

	FOR_EACH_MAP( RenderTargetIter, m_RenderTargets, HashedString, IRenderTarget* )
	{
		const HashedString& Tag			= RenderTargetIter.GetKey();
		IRenderTarget* pRenderTarget	= RenderTargetIter.GetValue();

		// Don't free the original render target! The renderer owns that.
		if( Tag == "Original" )
		{
			continue;
		}

		SafeDelete( pRenderTarget );
	}
}

IRenderTarget* TargetManager::GetRenderTarget( const HashedString& Tag ) const
{
	const Map<HashedString, IRenderTarget*>::Iterator RenderTargetIter = m_RenderTargets.Search( Tag );
	ASSERT( RenderTargetIter.IsValid() );
	return ( RenderTargetIter.IsValid() ) ? RenderTargetIter.GetValue() : NULL;
}
