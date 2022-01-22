#ifndef UIWIDGETFRAME_H
#define UIWIDGETFRAME_H

#include "uiwidget.h"
#include "vector2.h"
#include "simplestring.h"

class ITexture;
class Mesh;

class UIWidgetFrame : public UIWidget
{
public:
	UIWidgetFrame();
	virtual ~UIWidgetFrame();

	DEFINE_UIWIDGET_FACTORY( Frame );

	virtual void	Render( bool HasFocus );
	virtual void	UpdateRender();
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	Mesh*			CreateMesh() const;

	ITexture*		m_Texture;
	float			m_Border;
	Mesh*			m_Mesh;
	SimpleString	m_Material;
};

#endif // UIWIDGETFRAME_H
