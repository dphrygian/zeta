#ifndef WBCOMPROSAHITBOX_H
#define WBCOMPROSAHITBOX_H

#include "wbrosacomponent.h"
#include "hashedstring.h"
#include "segment.h"
#include "aabb.h"

struct SHitboxBone
{
	SHitboxBone()
	:	m_BoneIndex( 0 )
	,	m_BoneName()
	,	m_BoneSegment()
	,	m_BoneWidth( 0.0f )
	,	m_BoneWidthSq( 0.0f )
	,	m_BoneLengthScalar( 0.0f )
	{
	}

	int				m_BoneIndex;
	HashedString	m_BoneName;
	Segment			m_BoneSegment;
	float			m_BoneWidth;
	float			m_BoneWidthSq;
	float			m_BoneLengthScalar;
};

class WBCompRosaHitbox : public WBRosaComponent
{
public:
	WBCompRosaHitbox();

	DEFINE_WBCOMP( RosaHitbox, WBRosaComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	virtual void	HandleEvent( const WBEvent& Event );

#if BUILD_DEV
	virtual bool	HasDebugRender() const { return true; }
	virtual void	DebugRender( const bool GroupedRender ) const;
#endif

	bool						HasHitbox() const		{ return m_HitboxBones.Size() > 0; }
	void						UpdateHitbox();
	const AABB&					GetHitboxBounds() const	{ return m_HitboxBounds; }
	const Array<SHitboxBone>&	GetHitboxBones() const	{ return m_HitboxBones; }

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	bool				m_DirtyHitbox;	// Transient
	AABB				m_HitboxBounds;	// Transient
	Array<SHitboxBone>	m_HitboxBones;	// Config
};

#endif // WBCOMPROSAHITBOX_H
