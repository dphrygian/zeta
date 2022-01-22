#ifndef D3D9VERTEXBUFFER_H
#define D3D9VERTEXBUFFER_H

#include "3d.h"
#include "ivertexbuffer.h"
#include "list.h"

struct IDirect3DVertexBuffer9;
struct IDirect3DDevice9;

class D3D9VertexBuffer : public IVertexBuffer
{
protected:
	virtual ~D3D9VertexBuffer();

public:
	D3D9VertexBuffer( IDirect3DDevice9* D3DDevice );

	virtual void		Init( const SInit& InitStruct );

	virtual void*		GetPositions();
	virtual void*		GetColors();
	virtual void*		GetFloatColors();
	virtual void*		GetUVs();
	virtual void*		GetNormals();
	virtual void*		GetNormalsB();
	virtual void*		GetTangents();
	virtual void*		GetBoneIndices();
	virtual void*		GetBoneWeights();

	virtual uint		GetNumVertices();
	virtual void		SetNumVertices( uint NumVertices );

	virtual int			AddReference();
	virtual int			Release();

	virtual void		DeviceRelease();
	virtual void		DeviceReset();
	virtual void		RegisterDeviceResetCallback( const SDeviceResetCallback& Callback );

	virtual void*		Lock( EVertexElements VertexType );
	virtual void		Unlock( EVertexElements VertexType );

private:
	int								m_RefCount;
	uint							m_NumVertices;
	IDirect3DVertexBuffer9*			m_Positions;
	IDirect3DVertexBuffer9*			m_Colors;
	IDirect3DVertexBuffer9*			m_FloatColors;
	IDirect3DVertexBuffer9*			m_UVs;
	IDirect3DVertexBuffer9*			m_Normals;
	IDirect3DVertexBuffer9*			m_NormalsB;
	IDirect3DVertexBuffer9*			m_Tangents;
	IDirect3DVertexBuffer9*			m_BoneIndices;
	IDirect3DVertexBuffer9*			m_BoneWeights;
	IDirect3DDevice9*				m_D3DDevice;

	bool							m_Dynamic;

	IDirect3DVertexBuffer9*			InternalGetBuffer( EVertexElements VertexType );

	List< SDeviceResetCallback >	m_DeviceResetCallbacks;
};

#endif // D3D9VERTEXBUFFER_H
