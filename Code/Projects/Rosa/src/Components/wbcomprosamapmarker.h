#ifndef WBCOMPROSAMAPMARKER_H
#define WBCOMPROSAMAPMARKER_H

#include "wbrosacomponent.h"

class Mesh;

class WBCompRosaMapMarker : public WBRosaComponent
{
public:
	WBCompRosaMapMarker();
	virtual ~WBCompRosaMapMarker();

	DEFINE_WBCOMP( RosaMapMarker, WBRosaComponent );

	virtual void	Tick( const float DeltaTime );
	virtual int		GetTickOrder() { return ETO_TickDefault; }	// Should tick after transform

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	virtual bool	IsRenderable() { return true; }
	virtual void	Render();

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	void	CreateMesh( const SimpleString& Texture );

	bool	m_Orient;			// Config; if true, renders with orientation in world; if false, renders upright on screen
	bool	m_Hidden;			// Config/serialized
	bool	m_AlwaysShow;		// Config, show this even if minimap markers are disabled in config
	bool	m_ConfigEnabled;	// Transient, mirrors the config option so it doesn't have to be queried every frame for every marker
	float	m_Size;				// Config, screen size relative to minimap (or world size relative to view extents, same thing)
	Mesh*	m_Mesh;
};

#endif // WBCOMPROSAMAPMARKER_H
