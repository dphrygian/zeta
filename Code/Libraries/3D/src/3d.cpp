#include "core.h"
#include "3d.h"
#if BUILD_WINDOWS_NO_SDL
#include "D3D9/d3d9renderer.h"
#endif
#include "GL2/gl2renderer.h"

#if BUILD_WINDOWS_NO_SDL
IRenderer* CreateD3D9Renderer( HWND hWnd, Display* const pDisplay )
{
	return new D3D9Renderer( hWnd, pDisplay );
}
#endif

IRenderer* CreateGL2Renderer( Window* const pWindow, Display* const pDisplay )
{
	return new GL2Renderer( pWindow, pDisplay );
}
