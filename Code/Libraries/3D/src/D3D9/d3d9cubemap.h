#ifndef D3D9CUBEMAP_H
#define D3D9CUBEMAP_H

#include "cubemapcommon.h"
#include "d3d9texture.h"

struct IDirect3DDevice9;
struct IDirect3DCubeTexture9;

class D3D9Cubemap : public CubemapCommon
{
public:
	D3D9Cubemap( IDirect3DDevice9* D3DDevice );
	D3D9Cubemap( IDirect3DDevice9* D3DDevice, IDirect3DCubeTexture9* Texture );
	virtual ~D3D9Cubemap();

	virtual void*			GetHandle();
	virtual SSamplerState*	GetSamplerState() { return NULL; }
	virtual void			CreateCubemap( const SCubemapData& CubemapData );

private:
	// Pass through to texture classes
	virtual void	LoadDDS( const IDataStream& Stream, STextureData& OutTextureData ) { D3D9Texture::StaticLoadDDS( Stream, OutTextureData ); }

	IDirect3DCubeTexture9*	m_CubeTexture;
	IDirect3DDevice9*		m_D3DDevice;
};

#endif // D3D9CUBEMAP_H
