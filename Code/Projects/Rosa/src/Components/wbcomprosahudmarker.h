#ifndef WBCOMPROSAHUDMARKER_H
#define WBCOMPROSAHUDMARKER_H

#include "wbrosacomponent.h"
#include "simplestring.h"

class Mesh;
class ITexture;
class Font;

class WBCompRosaHUDMarker : public WBRosaComponent
{
public:
	WBCompRosaHUDMarker();
	virtual ~WBCompRosaHUDMarker();

	DEFINE_WBCOMP( RosaHUDMarker, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	virtual bool	IsRenderable() { return true; }
	virtual void	Render();

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void			CreateMesh();
	void			UpdateMesh();

	Mesh*			m_Mesh;				// Transient
	SimpleString	m_Material;			// Config
	ITexture*		m_OccludedImage;	// Config
	ITexture*		m_UnoccludedImage;	// Config
	float			m_Size;				// Config; extents of marker relative to screen height
	float			m_FalloffRadius;	// Config
	float			m_OffsetZ;			// Config

	Mesh*			m_DistanceMesh;		// Transient
	Font*			m_DistanceFont;		// Config
	uint			m_DistanceColor;	// Config
	float			m_DistanceHeight;	// Config

	bool			m_Hidden;			// Config/serialized

	bool			m_ConfigEnabled;	// Transient, mirrors the config option so it doesn't have to be queried every frame for every marker
};

#endif // WBCOMPROSAHUDMARKER_H
