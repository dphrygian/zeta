#ifndef UIWIDGETIMAGE_H
#define UIWIDGETIMAGE_H

#include "uiwidget.h"
#include "vector2.h"
#include "simplestring.h"
#include "array.h"

class ITexture;
class Mesh;

class UIWidgetImage : public UIWidget
{
public:
	UIWidgetImage();
	UIWidgetImage( const SimpleString& DefinitionName );
	virtual ~UIWidgetImage();

	DEFINE_UIWIDGET_FACTORY( Image );

	virtual void	Tick( const float DeltaTime );
	virtual void	Render( bool HasFocus );
	virtual void	UpdateRender();
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

	void			SetTexture( const char* Filename, const uint Index );
	void			SetTexture( ITexture* const pTexture, const uint Index );

	Array<ITexture*>	m_Textures;
	Mesh*				m_Mesh;
	SimpleString		m_Material;
	uint				m_CircleSides;
};

#endif // UIWIDGETIMAGE_H
