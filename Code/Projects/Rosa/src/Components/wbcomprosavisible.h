#ifndef WBCOMPROSAVISIBLE_H
#define WBCOMPROSAVISIBLE_H

#include "wbrosacomponent.h"
#include "vector.h"
#include "rosaworld.h"

class WBCompRosaVisible : public WBRosaComponent
{
public:
	WBCompRosaVisible();
	virtual ~WBCompRosaVisible();

	DEFINE_WBCOMP( RosaVisible, WBRosaComponent );

	virtual bool	BelongsInComponentArray() { return true; }

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );
	virtual void	AddContextToEvent( WBEvent& Event ) const;

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	bool			IsVisible() const { return m_Visible; }
	void			SetVisible( const bool Visible ) { m_Visible = Visible; }
	Vector			GetVisibleLocation( const bool IsAlreadyVisible ) const;
	Angles			GetVisibleOrientation() const;

	int				GetVisionPriority() const { return m_VisionPriority; }
	float			GetVisibleCertainty() const;

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	bool			m_Visible;
	int				m_VisionPriority;	// Config; ends up being used as combat target priority
};

#endif // WBCOMPROSAVISIBLE_H
