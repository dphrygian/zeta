#ifndef ROSAMESH_H
#define ROSAMESH_H

#include "mesh.h"
#include "wbentity.h"

class RosaMesh : public Mesh
{
public:
	RosaMesh();
	virtual ~RosaMesh();

	void				SetEntity( WBEntity* const pEntity ) { m_Entity = pEntity; }
	WBEntity*			GetEntity() const { return m_Entity; }

	void				SetSection( const HashedString& Section ) { m_Section = Section; }
	const HashedString&	GetSection() const { return m_Section; }

private:
	WBEntity*		m_Entity;
	HashedString	m_Section;	// For config colors, and any other similar purposes I may ever need
};

#endif // ROSAMESH_H
