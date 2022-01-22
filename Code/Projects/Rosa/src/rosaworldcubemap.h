#ifndef ROSAWORLDCUBEMAP_H
#define ROSAWORLDCUBEMAP_H

#include "cubemapcommon.h"
#include "rosairradiance.h"

class RosaWorldCubemap : public CubemapCommon
{
public:
	RosaWorldCubemap();
	virtual ~RosaWorldCubemap();

	virtual void*			GetHandle()			{ return m_APICubemap ? m_APICubemap->GetHandle() : NULL; }
	virtual SSamplerState*	GetSamplerState()	{ return m_APICubemap ? m_APICubemap->GetSamplerState() : NULL; }

	const SCubemapData&	GetData() const						{ return m_Data; }
	SCubemapData&		GetData()							{ return m_Data; }
	const STextureData&	GetData( const uint Index ) const	{ return m_Data.m_Textures[ Index ]; }
	STextureData&		GetData( const uint Index )			{ return m_Data.m_Textures[ Index ]; }

	// Non-const so we can access and modify
	SVoxelIrradiance&	GetIrradiance()						{ return m_Irradiance; }
	void				SetIrradiance( const SVoxelIrradiance& Irradiance ) { m_Irradiance = Irradiance; }	// This should work fine, it's a simple struct

	void				SetAPICubemap( ITexture* const pCubemap );

private:
	virtual void		LoadDDS( const IDataStream& Stream, STextureData& OutTextureData );
	virtual void		CreateCubemap( const SCubemapData& CubemapData );

	SCubemapData		m_Data;

	// HACKHACK: Also store irradiance, so we can easily grab it from the cubemap during rendering
	SVoxelIrradiance	m_Irradiance;

	// HACKHACK: Wrap an API-specific cubemap; we're essentially "decorating" this now
	ITexture*			m_APICubemap;
};

#endif // ROSAWORLDCUBEMAP_H
