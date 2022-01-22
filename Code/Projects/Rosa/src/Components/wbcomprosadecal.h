#ifndef WBCOMPROSADECAL_H
#define WBCOMPROSADECAL_H

#include "wbrosacomponent.h"
#include "matrix.h"

class WBCompRosaDecal : public WBRosaComponent
{
public:
	WBCompRosaDecal();
	virtual ~WBCompRosaDecal();

	DEFINE_WBCOMP( RosaDecal, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

	virtual uint	GetSerializationSize();
	virtual void	Save( const IDataStream& Stream );
	virtual void	Load( const IDataStream& Stream );

	bool			IsFinite() const		{ return m_Lifetime > 0.0f; }
	float			GetAlpha() const;
	const Matrix&	GetNormalBasis() const	{ return m_NormalBasis; }

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	float	m_Lifetime;					// Config
	float	m_FadeOutTime;				// Config
	float	m_InvFadeOutTime;			// Config
	float	m_ExpireTime;				// Serialized
	bool	m_PrescribedNormalBasis;	// Serialized; use a specific normal basis instead of pulling from the object's orientation
	Matrix	m_NormalBasis;				// Serialized; used to orient normals to a surface when decal is projected at a different angle
};

#endif // WBCOMPROSADECAL_H
