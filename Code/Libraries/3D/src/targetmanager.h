#ifndef TARGETMANAGER_H
#define TARGETMANAGER_H

#include "map.h"
#include "hashedstring.h"

class IRenderer;
class IRenderTarget;

class TargetManager
{
public:
	TargetManager( IRenderer* const pRenderer );
	virtual ~TargetManager();

	virtual void	CreateTargets( const uint DisplayWidth, const uint DisplayHeight );
	void			ReleaseTargets();

	IRenderTarget*	GetRenderTarget( const HashedString& Tag ) const;

protected:
	IRenderer*							m_Renderer;
	Map<HashedString, IRenderTarget*>	m_RenderTargets;
};

#endif // TARGETMANAGER_H
