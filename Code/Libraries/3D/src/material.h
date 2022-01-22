#ifndef MATERIAL_H
#define MATERIAL_H

#include "3d.h"
#include "vector4.h"
#include "simplestring.h"
#include "array.h"
#include "vector2.h"
#include "vector.h"
#include "renderstates.h"
#include "shaderdataprovider.h"
#include "hashedstring.h"

class ITexture;
class IShaderProgram;
class IVertexDeclaration;

#define DEFAULT_MATERIAL				"Material_Min"
#define DEFAULT_MATERIAL_NODEPTHTEST	"Material_Min_NoDepthTest"
#define DEFAULT_MATERIAL_HUD			"Material_MinHUD"

#define MAT_NONE			0x00000000
#define MAT_PRESCRIBED		0x00000001	// For buckets that can only accept meshes when prescribed
#define MAT_ALPHA			0x00000002
#define MAT_HUD				0x00000004
#define MAT_WORLD			0x00000008	// i.e., not HUD
#define MAT_ANIMATED		0x00000010
#define MAT_FOREGROUND		0x00000020
#define MAT_INWORLDHUD		0x00000040
#define MAT_DYNAMIC			0x00000080
#define MAT_SHADOW			0x00000100	// shadow-caster, rendered again for every shadow-casting light
#define MAT_DECALS			0x00000200	// accepts decals (automatically true for world geo, this is for entity meshes)
#define MAT_FOG				0x00000400	// renders in both main scene and as a mask on watercolor edges
#define MAT_FOGLIGHTS		0x00000800	// renders in both main scene and as a mask on watercolor edges
#define MAT_MINIMAP			0x00001000
#define MAT_MINIMAPMARKER	0x00002000

#define MAT_DEBUG_WORLD		0x10000000
#define MAT_DEBUG_HUD		0x20000000

#if BUILD_DEV
#define MAT_DEBUG_ALWAYS	0x40000000	// Always draw this, even if frustum-culled, etc.
#endif

#define MAT_ALWAYS			0x80000000	// Always draw this, even if frustum-culled, etc.

#define MAT_ALL				0xffffffff

class Material
{
public:
	Material();
	~Material();

	void	SetDefinition( const SimpleString& DefinitionName, IRenderer* const pRenderer, const uint VertexSignature );
	void	SetSamplerDefinition( const uint SamplerStage, const SimpleString& SamplerDefinitionName );
	void	SetTexture( const uint SamplerStage, ITexture* const pTexture );

	HashedString			GetName() const { return m_Name; }
	IShaderProgram*			GetShaderProgram() const { return m_ShaderProgram; }
	ITexture*				GetTexture( const uint SamplerStage ) const;
	ShaderDataProvider*		GetSDP() const { return m_SDP; }
	const SRenderOps&		GetRenderOps() const { return m_RenderOps; }
	const SRenderState&		GetRenderState() const { return m_RenderState; }
	const SSamplerState&	GetSamplerState( const uint SamplerStage ) const;
	uint					GetNumSamplers() const { return m_NumSamplers; }

#if BUILD_DEV
	uint					GetExpectedVD() const { return m_ExpectedVD; }
#endif

	bool					SupportsTexture( const uint SamplerStage ) const;
	bool					SupportsAlphaBlend() const;

	uint					GetFlags() const { return m_Flags; }
	void					SetFlags( const uint Flags, const uint Mask = MAT_ALL );
	void					SetFlag( const uint Flag, const bool Set );

	const HashedString&		GetBucket() const { return m_Bucket; }
	void					SetBucket( const HashedString& PrescribedBucket ) { m_Bucket = PrescribedBucket; }

	uint					GetLightCubeMask() const { return m_LightCubeMask; }
	void					SetLightCubeMask( const uint LightCubeMask ) { m_LightCubeMask = LightCubeMask; }

	void					SetRenderTarget( class IRenderTarget* const pRenderTarget ) { m_RenderOps.m_RenderTarget = pRenderTarget; }

	void					CopySamplerStatesFrom( const Material& OtherMaterial );

	void					Tick( const float DeltaTime ) { GetSDP()->Tick( DeltaTime ); }

private:
	HashedString		m_Name;
	IShaderProgram*		m_ShaderProgram;
	ShaderDataProvider*	m_SDP;
	uint				m_Flags;	// TODO: Move to a data-driven/project-specific system for declaring flags
	HashedString		m_Bucket;	// Prescribed bucket to minimize material flags
	uint				m_LightCubeMask;	// HACKHACK for shadow lights

	SRenderOps			m_RenderOps;
	SRenderState		m_RenderState;
	SSamplerState		m_SamplerStates[ MAX_TEXTURE_STAGES ];
	uint				m_NumSamplers;

#if BUILD_DEV
	uint				m_ExpectedVD;
#endif
};

#endif // MATERIAL_H
